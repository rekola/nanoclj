/* nanoclj 0.2.0
 * Mikael Rekola (mikael.rekola@gmail.com)
 * 
 * This version of nanoclj was branched at 1.42 from TinyScheme:
 *
 * T I N Y S C H E M E    1 . 4 2
 *   Dimitrios Souflis (dsouflis@acm.org)
 *   Based on MiniScheme (original credits follow)
 * (MINISCM)               coded by Atsushi Moriwaki (11/5/1989)
 * (MINISCM)           E-MAIL :  moriwaki@kurims.kurims.kyoto-u.ac.jp
 * (MINISCM) This version has been modified by R.C. Secrist.
 * (MINISCM)
 * (MINISCM) Mini-Scheme is now maintained by Akira KIDA.
 * (MINISCM)
 * (MINISCM) This is a revised and modified version by Akira KIDA.
 * (MINISCM)    current version is 0.85k4 (15 May 1994)
 *
 */

#define _NANOCLJ_SOURCE
#include "nanoclj-private.h"

// #define VECTOR_ARGS
// #define RETAIN_ALLOCS 1

#ifndef CELL_SEGSIZE
#define CELL_SEGSIZE    262144
#endif

#ifndef GC_VERBOSE
#define GC_VERBOSE 0
#endif

#ifdef _WIN32

#if defined(_MSC_VER) && _MSC_VER < 1900
#define snprintf _snprintf  /* Microsoft headers use underscores in some names */
#endif

#else /* _WIN32 */

#include <unistd.h>
#include <pwd.h>
#include <sys/utsname.h>

#endif /* _WIN32 */

#if USE_DL
#include "dynload.h"
#endif

#include <math.h>
#include <limits.h>
#include <stdarg.h>
#include <assert.h>
#include <signal.h>
#include <sys/stat.h>

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

#include <utf8proc.h>

#include "nanoclj_clock.h"
#include "nanoclj_hash.h"
#include "nanoclj_legal.h"
#include "nanoclj_term.h"
#include "nanoclj_utf8.h"

/* Predefined primitive values */
#define kNIL			UINT64_C(18444492273895866368)
#define kTRUE                  UINT64_C(9221401712017801217)
#define kFALSE                 UINT64_C(9221401712017801216)

/* Masks for NaN packing */
#define MASK_SIGN		UINT64_C(0x8000)
#define MASK_EXPONENT		UINT64_C(0x7ff0)
#define MASK_QUIET		UINT64_C(0x0008)
#define MASK_TYPE		UINT64_C(0x0007)

#define MASK_PAYLOAD		0x0000ffffffffffff

/* Signatures for primitive types */
#define SIGNATURE_CELL		(MASK_EXPONENT | MASK_QUIET | MASK_SIGN)
#define SIGNATURE_NAN		(MASK_EXPONENT | MASK_QUIET | 0)
#define SIGNATURE_BOOLEAN	(MASK_EXPONENT | MASK_QUIET | 1)
#define SIGNATURE_LONG		(MASK_EXPONENT | MASK_QUIET | 2)
#define SIGNATURE_CODEPOINT	(MASK_EXPONENT | MASK_QUIET | 3)
#define SIGNATURE_PROC		(MASK_EXPONENT | MASK_QUIET | 4)
#define SIGNATURE_KEYWORD	(MASK_EXPONENT | MASK_QUIET | 5)
#define SIGNATURE_SYMBOL	(MASK_EXPONENT | MASK_QUIET | 6)
// 7: unassigned

/* Parsing tokens */
#define TOK_EOF		(-1)
#define TOK_LPAREN  	0
#define TOK_RPAREN  	1
#define TOK_RCURLY  	2
#define TOK_RSQUARE 	3
#define TOK_OLD_DOT    	4
#define TOK_PRIMITIVE  	5
#define TOK_QUOTE   	6
#define TOK_DEREF   	7
#define TOK_COMMENT 	8
#define TOK_DQUOTE  	9
#define TOK_BQUOTE  	10
#define TOK_COMMA   	11
#define TOK_ATMARK  	12
#define TOK_TAG     	13
#define TOK_CHAR_CONST	14
#define TOK_SHARP_CONST	15
#define TOK_VEC     	16
#define TOK_MAP	    	17
#define TOK_SET	    	18
#define TOK_FN      	19
#define TOK_REGEX   	20
#define TOK_IGNORE  	21
#define TOK_VAR	    	22
#define TOK_DOT		23
#define TOK_META	24

#define BACKQUOTE 	'`'
#define DELIMITERS  	"()[]{}\";\f\t\v\n\r, "
#define NPOS		((size_t)-1)

#define PORT_SAW_EOF	1

#define NANOCLJ_VERSION "0.2.0"

#include <string.h>
#include <stdlib.h>

#ifndef InitFile
#define InitFile "init.clj"
#endif

/*
 *  Basic memory allocation units
 */

#ifndef FIRST_CELLSEGS
#define FIRST_CELLSEGS 3
#endif

enum nanoclj_types {
  T_NIL = 0,
  T_REAL = 1,
  T_BOOLEAN = 2,
  T_LONG = 3,
  T_CODEPOINT = 4,
  T_PROC = 5,
  T_KEYWORD = 6,
  T_SYMBOL = 7,
  /* unassigned = 8 */
  T_LIST = 9,
  T_STRING = 10,
  T_CHAR_ARRAY = 11,
  T_CLOSURE = 12,
  T_RATIO = 13,
  T_FOREIGN_FUNCTION = 14,
  T_READER = 15,
  T_WRITER = 16,
  T_INPUT_STREAM = 17,
  T_OUTPUT_STREAM = 18,
  T_GZIP_INPUT_STREAM = 19,
  T_GZIP_OUTPUT_STREAM = 20,
  T_VECTOR = 21,
  T_MACRO = 22,
  T_LAZYSEQ = 23,
  T_ENVIRONMENT = 24,
  T_CLASS = 25,
  T_MAPENTRY = 26,
  T_ARRAYMAP = 27,
  T_SORTED_SET = 28,
  T_VAR = 29,
  T_FOREIGN_OBJECT = 30,
  T_FOREIGN_METHOD = 31,
  T_BIGINT = 32,
  T_REGEX = 33,
  T_DELAY = 34,
  T_IMAGE = 35,
  T_AUDIO = 36,
  T_FILE = 37,
  T_DATE = 38,
  T_UUID = 39,
  T_QUEUE = 40,
  T_RUNTIME_EXCEPTION = 41,
  T_ARITY_EXCEPTION = 42,
  T_ILLEGAL_ARG_EXCEPTION = 43,
  T_NUM_FMT_EXCEPTION = 44,
  T_ARITHMETIC_EXCEPTION = 45,
  T_CLASS_CAST_EXCEPTION = 46,
  T_ILLEGAL_STATE_EXCEPTION = 47,
  T_TENSOR = 48,
  T_GRAPH = 49,
  T_LAST_SYSTEM_TYPE = 50
};

typedef struct {
  int_fast16_t syntax;
  strview_t ns, name, full_name;
  uint32_t hash;
  nanoclj_val_t ns_sym, name_sym;
  bool is_amp_qualified;
} symbol_t;

typedef struct {
  char * url;
  char * useragent;
  bool http_keepalive;
  int http_max_connections;
  int http_max_redirects;
  int fd;

  func_alloc malloc;
  func_dealloc free;
  func_realloc realloc;
} http_load_t;

#define T_TYPE	         1	/* 00000001 */
#define T_HASHED         2      /* 00000010 */
#define T_SEQUENCE       4	/* 00000100 */
#define T_REALIZED       8	/* 00001000 */
#define T_REVERSE       16      /* 00010000 */
#define T_SMALL         32      /* 00100000 */
#define T_GC_ATOM       64      /* 01000000 */    /* only for gc */
#define CLR_GC_ATOM    191      /* 10111111 */    /* only for gc */
#define MARK           128      /* 10000000 */
#define UNMARK         127      /* 01111111 */

static uint_fast16_t prim_type(nanoclj_val_t value) {
  uint64_t signature = value.as_long >> 48;   
  if (signature == SIGNATURE_CELL) {
    return T_NIL;
  } else if ((signature & (MASK_EXPONENT | MASK_QUIET)) == (MASK_EXPONENT | MASK_QUIET)) {
    return (signature & 7) + 1;
  }
  return T_REAL;  
}

static inline bool is_cell(nanoclj_val_t v) {
  return (v.as_long >> 48) == SIGNATURE_CELL;
}

static inline bool is_primitive(nanoclj_val_t v) {
  return !is_cell(v);
}

static inline bool is_symbol(nanoclj_val_t v) {
  return (v.as_long >> 48) == SIGNATURE_SYMBOL;
}

static inline bool is_keyword(nanoclj_val_t v) {
  return (v.as_long >> 48) == SIGNATURE_KEYWORD;
}

/* returns the type of a, a must not be NULL */
static inline uint_fast16_t _type(nanoclj_cell_t * a) {
  if (a->flags & T_TYPE) return T_CLASS;
  return a->type;
}

static inline nanoclj_cell_t * decode_pointer(nanoclj_val_t value) {
  return (nanoclj_cell_t *)(value.as_long & MASK_PAYLOAD);
}

static inline symbol_t * decode_symbol(nanoclj_val_t value) {
  return (symbol_t *)(value.as_long & MASK_PAYLOAD);
}

static inline bool is_type(nanoclj_val_t value) {
  if (!is_cell(value)) return false;
  return decode_pointer(value)->flags & T_TYPE;
}

static inline uint_fast16_t expand_type(nanoclj_val_t value, uint_fast16_t primitive_type) {
  if (!primitive_type) return _type(decode_pointer(value));
  return primitive_type;
}

static inline uint_fast16_t type(nanoclj_val_t value) {
  uint_fast16_t type = prim_type(value);
  if (!type) {
    nanoclj_cell_t * c = decode_pointer(value);
    return _type(c);
  } else {
    return type;
  }
}

static inline bool is_list(nanoclj_val_t value) {
  if (!is_cell(value)) return false;
  nanoclj_cell_t * c = decode_pointer(value);
  return c && _type(c) == T_LIST;
}

static inline nanoclj_val_t mk_pointer(nanoclj_cell_t * ptr) {
  return (nanoclj_val_t)((SIGNATURE_CELL << 48) | (uint64_t)ptr);
}

static inline nanoclj_val_t mk_nil() {
  return (nanoclj_val_t)kNIL;
}

static inline nanoclj_val_t mk_symbol(nanoclj_t * sc, uint16_t t, strview_t ns, strview_t name, int_fast16_t syntax) {
  /* mk_symbol allocations are leaked intentionally for now */
  char * ns_str = sc->malloc(ns.size);
  memcpy(ns_str, ns.ptr, ns.size);

  char * name_str = sc->malloc(name.size);
  memcpy(name_str, name.ptr, name.size);

  char * full_name_str = sc->malloc((t == T_KEYWORD ? 1 : 0) + ns.size + name.size + 1);
  char * p = full_name_str;
  size_t full_name_size = 0;
  if (t == T_KEYWORD) {
    *p++ = ':';
    full_name_size++;
  }
  if (ns.size) {
    memcpy(p, ns.ptr, ns.size);
    p += ns.size;
    *p++ = '/';
    full_name_size += ns.size + 1;
  }
  memcpy(p, name.ptr, name.size);
  full_name_size += name.size;

  symbol_t * s = sc->malloc(sizeof(symbol_t));
  s->ns = (strview_t){ ns_str, ns.size };
  s->name = (strview_t){ name_str, name.size };
  s->full_name = (strview_t){ full_name_str, full_name_size };
  s->syntax = syntax;
  s->hash = murmur3_hash_qualified_string(ns.ptr, ns.size, name.ptr, name.size);
  s->ns_sym = s->name_sym = mk_nil();

  return (nanoclj_val_t)((t == T_SYMBOL ? (SIGNATURE_SYMBOL << 48) : (SIGNATURE_KEYWORD << 48)) | (uint64_t)s);
}

static inline bool is_nil(nanoclj_val_t v) {
  return v.as_long == kNIL;
}

#define _is_gc_atom(p)            (p->flags & T_GC_ATOM)
#define _car(p)		 	  ((p)->_object._cons.car)
#define _cdr(p)			  ((p)->_object._cons.cdr)
#define _set_car(p, v)         	  _car(p) = v
#define _set_cdr(p, v)         	  _cdr(p) = v
#define car(p)           	  _car(decode_pointer(p))
#define cdr(p)           	  _cdr(decode_pointer(p))
#define set_car(p, v)         	  _car(decode_pointer(p)) = v
#define set_cdr(p, v)         	  _cdr(decode_pointer(p)) = v

#define _so_vector_metadata(p)	  ((p)->_object._small_collection.meta)
#define _cons_metadata(p)	  ((p)->_object._cons.meta)
#define _ff_metadata(p)		  ((p)->_object._ff.meta)
#define _image_metadata(p)	  ((p)->_object._image.meta)
#define _audio_metadata(p)	  ((p)->_object._audio.meta)

#define _is_realized(p)           (p->flags & T_REALIZED)
#define _set_realized(p)          (p->flags |= T_REALIZED)
#define _is_mark(p)               (p->flags & MARK)
#define _setmark(p)               (p->flags |= MARK)
#define _clrmark(p)               (p->flags &= UNMARK)
#define _is_reverse(p)		  (p->flags & T_REVERSE)
#define _is_sequence(p)		  (p->flags & T_SEQUENCE)
#define _set_seq(p)	  	  (p->flags |= T_SEQUENCE)
#define _set_rseq(p)	  	  (p->flags |= T_SEQUENCE | T_REVERSE)
#define _is_small(p)	          (p->flags & T_SMALL)
#define _set_gc_atom(p)           (p->flags |= T_GC_ATOM)
#define _clr_gc_atom(p)           (p->flags &= CLR_GC_ATOM)

#define _offset_unchecked(p)	  ((p)->_object._collection.offset)
#define _size_unchecked(p)	  ((p)->_object._collection.size)
#define _vec_store_unchecked(p)	  ((p)->_object._collection._store._vec)
#define _str_store_unchecked(p)	  ((p)->_object._collection._store._str)
#define _smalldata_unchecked(p)   (&((p)->_object._small_collection.data[0]))

#define _rep_unchecked(p)	  ((p)->_object._port.rep)
#define _port_type_unchecked(p)	  ((p)->_object._port.type)
#define _port_flags_unchecked(p)  ((p)->_object._port.flags)
#define _nesting_unchecked(p)     ((p)->_object._port.nesting)
#define _line_unchecked(p)	  ((p)->_object._port.line)
#define _column_unchecked(p)	  ((p)->_object._port.column)

#define _image_unchecked(p)	  ((p)->_object._image.rep)
#define _audio_unchecked(p)	  ((p)->_object._audio.rep)
#define _tensor_unchecked(p)	  ((p)->_object._tensor.impl)
#define _lvalue_unchecked(p)      ((p)->_object._lvalue)
#define _re_unchecked(p)	  ((p)->_object._re)
#define _ff_unchecked(p)	  ((p)->_object._ff.ptr)
#define _fo_unchecked(p)	  ((p)->_object._fo.ptr)
#define _min_arity_unchecked(p)	  ((p)->_object._ff.min_arity)
#define _max_arity_unchecked(p)	  ((p)->_object._ff.max_arity)

#define _smallstrvalue_unchecked(p)      (&((p)->_object._small_bytearray.data[0]))
#define _sosize_unchecked(p)      ((p)->so_size)

/* macros for cell operations */
#define cell_type(p)      	  (decode_pointer(p)->type)
#define cell_flags(p)      	  (decode_pointer(p)->flags)
#define is_small(p)	 	  (cell_flags(p)&T_SMALL)
#define is_gc_atom(p)             (decode_pointer(p)->flags & T_GC_ATOM)

#define rep_unchecked(p)	  _rep_unchecked(decode_pointer(p))
#define port_type_unchecked(p)	  _port_type_unchecked(decode_pointer(p))
#define port_flags_unchecked(p)	  _port_flags_unchecked(decode_pointer(p))
#define nesting_unchecked(p)      _nesting_unchecked(decode_pointer(p))

#define image_unchecked(p)	  _image_unchecked(decode_pointer(p))
#define audio_unchecked(p)	  _audio_unchecked(decode_pointer(p))

static inline int32_t decode_integer(nanoclj_val_t value) {
  return (uint32_t)value.as_long;
}

static inline bool is_string(nanoclj_val_t p) {
  if (!is_cell(p)) return false;
  nanoclj_cell_t * c = decode_pointer(p);
  return c && _type(c) == T_STRING;
}
static inline bool is_char_array(nanoclj_val_t p) {
  if (!is_cell(p)) return false;
  nanoclj_cell_t * c = decode_pointer(p);
  return c && _type(c) == T_CHAR_ARRAY;
}
static inline bool is_file(nanoclj_val_t p) {
  if (!is_cell(p)) return false;
  nanoclj_cell_t * c = decode_pointer(p);
  return c && _type(c) == T_FILE;
}
static inline bool is_vector(nanoclj_val_t p) {
  if (!is_cell(p)) return false;
  nanoclj_cell_t * c = decode_pointer(p);
  return c && _type(c) == T_VECTOR;
}
static inline bool is_image(nanoclj_val_t p) {
  if (!is_cell(p)) return false;
  nanoclj_cell_t * c = decode_pointer(p);
  return c && _type(c) == T_IMAGE;
}

static inline bool is_seqable_type(uint_fast16_t t) {
  switch (t) {
  case T_NIL:
  case T_LIST:
  case T_LAZYSEQ:
  case T_STRING:
  case T_CHAR_ARRAY:
  case T_FILE:
  case T_VECTOR:
  case T_ARRAYMAP:
  case T_SORTED_SET:
  case T_CLASS:
  case T_MAPENTRY:
  case T_CLOSURE:
  case T_MACRO:
  case T_QUEUE:
    return true;
  }
  return false;
}

static inline bool is_coll_type(uint_fast16_t t) {
  switch (t) {
  case T_NIL: /* Empty list */
  case T_LIST:
  case T_VECTOR:
  case T_MAPENTRY:
  case T_ARRAYMAP:
  case T_SORTED_SET:
  case T_LAZYSEQ:
  case T_QUEUE:
    return true;
  }
  return false;
}

static inline bool is_string_type(uint_fast16_t t) {
  return t == T_STRING || t == T_CHAR_ARRAY || t == T_FILE || t == T_UUID;
}

static inline bool is_vector_type(uint_fast16_t t) {
  return t == T_VECTOR || t == T_ARRAYMAP || t == T_SORTED_SET ||
    t == T_RATIO || t == T_MAPENTRY || t == T_VAR || t == T_QUEUE;
}

static inline nanoclj_val_t vector_elem(nanoclj_cell_t * vec, size_t ielem) {
  if (_is_small(vec)) {
    return _smalldata_unchecked(vec)[ielem];
  } else {
    return _vec_store_unchecked(vec)->data[_offset_unchecked(vec) + ielem];
  }
}

static inline void set_vector_elem(nanoclj_cell_t * vec, size_t ielem, nanoclj_val_t a) {
  if (_is_small(vec)) {
    _smalldata_unchecked(vec)[ielem] = a;
  } else {
    _vec_store_unchecked(vec)->data[_offset_unchecked(vec) + ielem] = a;
  }
}

/* Size of vector or string */
static inline size_t get_size(nanoclj_cell_t * c) {
  return _is_small(c) ? _sosize_unchecked(c) : _size_unchecked(c);
}

static void fill_vector(nanoclj_cell_t * vec, nanoclj_val_t elem) {
  size_t num = get_size(vec);
  for (size_t i = 0; i < num; i++) {
    set_vector_elem(vec, i, elem);
  }
}

static inline nanoclj_cell_t * get_metadata(nanoclj_cell_t * c) {
  switch (_type(c)) {
  case T_VECTOR:
  case T_ARRAYMAP:
  case T_SORTED_SET:
  case T_VAR:
  case T_QUEUE:
    if (_is_small(c)) {
      return _so_vector_metadata(c);
    } else {
      return _vec_store_unchecked(c)->meta;
    }
  case T_LIST:
  case T_CLOSURE:
  case T_MACRO:
  case T_CLASS:
  case T_ENVIRONMENT:
    return _cons_metadata(c);
  case T_FOREIGN_FUNCTION:
    return _ff_metadata(c);
  case T_IMAGE:
    return _image_metadata(c);
  case T_AUDIO:
    return _audio_metadata(c);
  }
  return NULL;
}

static inline void set_metadata(nanoclj_cell_t * c, nanoclj_cell_t * meta) {
  switch (_type(c)) {
  case T_VECTOR:
  case T_ARRAYMAP:
  case T_SORTED_SET:
  case T_VAR:
  case T_QUEUE:
    if (_is_small(c)) {
      _so_vector_metadata(c) = meta;
    } else {
      _vec_store_unchecked(c)->meta = meta;
    }
    break;
  case T_LIST:
  case T_CLOSURE:
  case T_MACRO:
    _cons_metadata(c) = meta;
    break;
  case T_FOREIGN_FUNCTION:
    _ff_metadata(c) = meta;
    break;
  case T_IMAGE:
    _image_metadata(c) = meta;
    break;
  case T_AUDIO:
    _audio_metadata(c) = meta;
    break;
  }
}

static inline bool is_integral_type(uint_fast16_t type) {
  switch (type) {
  case T_BOOLEAN:
  case T_LONG:
  case T_CODEPOINT:
  case T_PROC:
    return true;
  }
  return false;
}

static inline bool is_numeric_type(uint16_t t) {
  switch (t) {
  case T_LONG:
  case T_REAL:
  case T_RATIO:
  case T_BIGINT:
    return true;
  }
  return false;
}

/* Returns true if argument is number */
static inline bool is_number(nanoclj_val_t p) {
  return !is_nil(p) && is_numeric_type(type(p));  
}

static inline char * _strvalue(nanoclj_cell_t * s) {
  if (_is_small(s)) {
    return (char *)_smallstrvalue_unchecked(s);
  } else {
    return (char *)_str_store_unchecked(s)->data + _offset_unchecked(s);
  }
}

static inline long long to_long_w_def(nanoclj_val_t p, long long def) {
  switch (prim_type(p)) {
  case T_LONG:
  case T_PROC:
  case T_CODEPOINT:
    return decode_integer(p);
  case T_REAL:
    return (long long)p.as_double;
  case T_NIL:
    {
      nanoclj_cell_t * c = decode_pointer(p);
      if (c) {
	switch (_type(c)) {
	case T_LONG:
	case T_DATE:
	  return _lvalue_unchecked(c);
	case T_RATIO:
	  return to_long_w_def(vector_elem(c, 0), 0) / to_long_w_def(vector_elem(c, 1), 1);
	case T_VECTOR:
	case T_SORTED_SET:
	case T_ARRAYMAP:
	case T_QUEUE:
	  return get_size(c);
	case T_CLASS:
	  return c->type;
	}
      }
    }
  }
  
  return def;
}

static inline long long to_long(nanoclj_val_t p) {
  return to_long_w_def(p, 0);
}

static inline int32_t to_int(nanoclj_val_t p) {
  switch (prim_type(p)) {
  case T_CODEPOINT:
  case T_PROC:
  case T_LONG:
    return decode_integer(p);
  case T_REAL:
    return (int32_t)p.as_double;
  case T_NIL:
    {
      nanoclj_cell_t * c = decode_pointer(p);
      switch (_type(c)) {
      case T_LONG:
      case T_DATE:
	return _lvalue_unchecked(c);
      case T_RATIO:
	return to_long(vector_elem(c, 0)) / to_long(vector_elem(c, 1));
      case T_VECTOR:
      case T_SORTED_SET:
      case T_ARRAYMAP:
      case T_QUEUE:
	return get_size(c);
      case T_CLASS:
	return c->type;
      }
    }
  }
  return 0;
}

static inline double to_double(nanoclj_val_t p) {
  switch (prim_type(p)) {
  case T_LONG:
    return (double)decode_integer(p);
  case T_REAL:
    return p.as_double;
  case T_NIL: {
    nanoclj_cell_t * c = decode_pointer(p);
    switch (_type(c)) {
    case T_LONG:
    case T_DATE: return (double)_lvalue_unchecked(c);
    case T_RATIO: return to_double(vector_elem(c, 0)) / to_double(vector_elem(c, 1));
    }
  }
  }
  return NAN;
}

static inline void * to_tensor(nanoclj_t * sc, nanoclj_val_t p) {
  switch (prim_type(p)) {
  case T_NIL:{
    nanoclj_cell_t * c = decode_pointer(p);
    switch (_type(c)) {
    case T_TENSOR: return _tensor_unchecked(c);
    }
  }
  }
  return NULL;
}

static inline bool is_readable(nanoclj_cell_t * p) {
  if (!p) return false;
  switch (_type(p)) {
  case T_READER:
  case T_INPUT_STREAM:
  case T_GZIP_INPUT_STREAM: return true;
  }
  return false;
}

static inline bool is_writable(nanoclj_cell_t * p) {
  if (!p) return false;
  switch (_type(p)) {
  case T_WRITER:
  case T_OUTPUT_STREAM: return true;
  }
  return false;
}

static inline bool is_writer(nanoclj_val_t p) {
  if (!is_cell(p)) return false;
  nanoclj_cell_t * c = decode_pointer(p);
  return c && _type(c) == T_WRITER;
}

static inline bool is_mapentry(nanoclj_val_t p) {
  if (!is_cell(p)) return false;
  nanoclj_cell_t * c = decode_pointer(p);
  return c && _type(c) == T_MAPENTRY;
}

static inline bool is_closure(nanoclj_val_t p) {
  if (!is_cell(p)) return false;
  nanoclj_cell_t * c = decode_pointer(p);
  return c && _type(c) == T_CLOSURE;
}
static inline bool is_macro(nanoclj_val_t p) {
  if (!is_cell(p)) return false;
  nanoclj_cell_t * c = decode_pointer(p);
  return c && _type(c) == T_MACRO;
}
static inline nanoclj_val_t closure_code(nanoclj_cell_t * p) {
  return _car(p);
}
static inline nanoclj_cell_t * closure_env(nanoclj_cell_t * p) {
  return decode_pointer(_cdr(p));
}

static inline bool is_environment(nanoclj_val_t p) {
  if (!is_cell(p)) return false;
  nanoclj_cell_t * c = decode_pointer(p);
  return c && (_type(c) == T_ENVIRONMENT || _type(c) == T_CLASS);
}

/* true or false value functions */
/* () is #t in R5RS, but nil is false in clojure */

static inline bool is_false(nanoclj_val_t p) {
  return p.as_long == kFALSE || p.as_long == kNIL;
}

static inline bool is_true(nanoclj_val_t p) {
  return !is_false(p);
}

#define is_mark(p)       (cell_flags(p)&MARK)
#define clrmark(p)       cell_flags(p) &= UNMARK

#define _cadr(p)          car(_cdr(p))
#define _caddr(p)         car(cdr(_cdr(p)))

#define caar(p)          car(car(p))
#define caadr(p)         car(car(cdr(p)))
#define cadr(p)          car(cdr(p))
#define cdar(p)          cdr(car(p))
#define cddr(p)          cdr(cdr(p))
#define cadar(p)         car(cdr(car(p)))
#define caddr(p)         car(cdr(cdr(p)))
#define cdaar(p)         cdr(car(car(p)))
#define cadaar(p)        car(cdr(car(car(p))))
#define cadddr(p)        car(cdr(cdr(cdr(p))))
#define cddddr(p)        cdr(cdr(cdr(cdr(p))))

typedef struct {
  char *name;
  char *doc;
} opcode_info;

static opcode_info dispatch_table[] = {
#define _OP_DEF(A,B,OP) {A,B},
#include "nanoclj-op.h"
  { 0 }
};

static void Eval_Cycle(nanoclj_t * sc, enum nanoclj_opcode op);

static inline strview_t mk_strview(const char * s) {
  return s ? (strview_t){ s, strlen(s) } : (strview_t){ "", 0 };
}

static inline strview_t _to_strview(nanoclj_cell_t * c) {
  switch (_type(c)) {
  case T_NIL:
    return (strview_t){ "()", 2 };
  case T_STRING:
  case T_CHAR_ARRAY:
  case T_FILE:
  case T_UUID:
    if (_is_small(c)) {
      return (strview_t){ (char *)_smallstrvalue_unchecked(c), _sosize_unchecked(c) };
    } else {
      return (strview_t){ (char *)_str_store_unchecked(c)->data + _offset_unchecked(c), _size_unchecked(c) };
    }
  case T_READER:
  case T_WRITER:
    if (_port_type_unchecked(c) == port_string) {
      nanoclj_port_rep_t * pr = _rep_unchecked(c);
      return (strview_t){ (char *)pr->string.data->data, pr->string.data->size };
    }
    break;
  }
  return mk_strview(0);
}

static inline strview_t to_strview(nanoclj_val_t x) {
  switch (prim_type(x)) {
  case T_BOOLEAN:
    return decode_integer(x) == 0 ? (strview_t){ "false", 5 } : (strview_t){ "true", 4 };    
  case T_SYMBOL:
  case T_KEYWORD:
    return decode_symbol(x)->full_name;
  case T_PROC:
    return mk_strview(dispatch_table[decode_integer(x)].name);  
  case T_NIL:{
    nanoclj_cell_t * c = decode_pointer(x);
    if (c) return _to_strview(c);
  }
  }  
  return mk_strview(0);
}

static inline imageview_t _to_imageview(nanoclj_cell_t * c) {
  switch (_type(c)) {
  case T_IMAGE:{
    nanoclj_image_t * img = _image_unchecked(c);
    return (imageview_t){ img->data, img->width, img->height, img->stride, img->format };
  }
  case T_WRITER:
#if NANOCLJ_HAS_CANVAS
    {
      nanoclj_port_rep_t * pr = _rep_unchecked(c);
      switch (_port_type_unchecked(c)) {
      case port_canvas: return canvas_get_imageview(pr->canvas.impl);
      }
    }
#endif
  }
  return (imageview_t){ 0 };
}

static inline imageview_t to_imageview(nanoclj_val_t p) {
  if (is_cell(p)) {
    return _to_imageview(decode_pointer(p));
  } else {
    return (imageview_t){ 0 };
  }
}

static inline nanoclj_color_t to_color(nanoclj_val_t p) {
  switch (type(p)) {
  case T_STRING:{
    strview_t sv = _to_strview(decode_pointer(p));
    int red, green, blue;
    if (sv.size == 7 && sscanf(sv.ptr, "#%2x%2x%2x", &red, &green, &blue) == 3) {
      return mk_color3i(red, green, blue);
    } else if (sv.size == 4 && sscanf(sv.ptr, "#%1x%1x%1x", &red, &green, &blue) == 3) {
      return mk_color3i((red << 4) | red, (green << 4) | green, (blue << 4) | blue);
    }
  }
    break;
    
  case T_LONG:
  case T_REAL:
    return mk_color4d(1.0, 1.0, 1.0, to_double(p));
    
  case T_VECTOR:{
    nanoclj_cell_t * c = decode_pointer(p);
    if (get_size(c) >= 3) {
      return mk_color4d( to_double(vector_elem(c, 0)),
			 to_double(vector_elem(c, 1)),
			 to_double(vector_elem(c, 2)),
			 get_size(c) >= 4 ? to_double(vector_elem(c, 3)) : 1.0);
    }
  }
  }
  
  return mk_color4d(0, 0, 0, 0);
}

static inline int_fast16_t syntaxnum(nanoclj_val_t p) {
  if (is_symbol(p)) {
    return decode_symbol(p)->syntax;
  }
  return 0;
}

/* allocate new cell segment */
static inline int alloc_cellseg(nanoclj_t * sc, int n) {
  nanoclj_shared_context_t * context = sc->context;
  nanoclj_val_t p;

  if (context->last_cell_seg >= context->n_seg_reserved - 1) {
    int n = context->n_seg_reserved ? context->n_seg_reserved * 2 : 12;
    context->n_seg_reserved = n;
    context->alloc_seg = sc->realloc(context->alloc_seg, n * sizeof(nanoclj_cell_t *));
    context->cell_seg = sc->realloc(context->cell_seg, n * sizeof(nanoclj_val_t));
  }
    
  for (int k = 0; k < n; k++) {
    nanoclj_cell_t * cp = sc->malloc(CELL_SEGSIZE * sizeof(nanoclj_cell_t));
    if (!cp) {
      return k;     
    }
    long i = ++context->last_cell_seg;
    context->alloc_seg[i] = cp;    
    /* insert new segment in address order */
    nanoclj_val_t newp = mk_pointer(cp);
    context->cell_seg[i] = newp;
    while (i > 0 && context->cell_seg[i - 1].as_long > context->cell_seg[i].as_long) {
      p = context->cell_seg[i];
      context->cell_seg[i] = context->cell_seg[i - 1];
      context->cell_seg[--i] = p;
    }
    context->fcells += CELL_SEGSIZE;
    nanoclj_val_t last = mk_pointer(decode_pointer(newp) + CELL_SEGSIZE - 1);
    for (p = newp; p.as_long <= last.as_long; p = mk_pointer(decode_pointer(p) + 1)) {
      cell_type(p) = 0;
      cell_flags(p) = 0;
      set_cdr(p, mk_pointer(decode_pointer(p) + 1));
      set_car(p, sc->EMPTY);
    }
    /* insert new cells in address order on free list */
    if (!context->free_cell || decode_pointer(p) < context->free_cell) {
      set_cdr(last, context->free_cell ? mk_pointer(context->free_cell) : sc->EMPTY);
      context->free_cell = decode_pointer(newp);
    } else {
      nanoclj_cell_t * p = context->free_cell;
      while (_cdr(p).as_long != sc->EMPTY.as_long && newp.as_long > _cdr(p).as_long)
        p = decode_pointer(_cdr(p));

      set_cdr(last, _cdr(p));
      _set_cdr(p, newp);
    }
  }

  return n;
}

static void free_bytearray(nanoclj_t * sc, nanoclj_bytearray_t * a) {
  if (a) {
    sc->free(a->data);
    sc->free(a);
  }
}

static void free_valarray(nanoclj_t * sc, nanoclj_valarray_t * a) {
  if (a) {
    sc->free(a->data);
    sc->free(a);
  }
}

static inline int port_close(nanoclj_t * sc, nanoclj_cell_t * p) {
  int r = 0;
  nanoclj_port_rep_t * pr = _rep_unchecked(p);
  switch (_port_type_unchecked(p)) {
  case port_file:
    if (pr->stdio.filename) {
      sc->free(pr->stdio.filename);
    }
    FILE * fh = pr->stdio.file;
    if (fh && (fh != stdout && fh != stderr && fh != stdin)) {
      int fd = fileno(fh);
      struct stat statbuf;
      fstat(fd, &statbuf);
      if (S_ISFIFO(statbuf.st_mode)) {
	r = pclose(fh) >> 8;
      } else {
	fclose(fh);
      }
    }
    break;
  case port_string:
    free_bytearray(sc, pr->string.data);
    break;
  case port_z:
    inflateEnd(&(pr->z.strm));
    break;
#if NANOCLJ_HAS_CANVAS
  case port_canvas:
    finalize_canvas(sc, pr->canvas.impl);
    break;
#endif
  }
  sc->free(pr);
  _port_type_unchecked(p) = port_free;
  _rep_unchecked(p) = NULL;
  return r;
}

static inline void finalize_cell(nanoclj_t * sc, nanoclj_cell_t * a) {
  if (_is_small(a)) {
    return;
  }
  switch (_type(a)) {
  case T_STRING:
  case T_CHAR_ARRAY:
  case T_FILE:
  case T_UUID:{
    nanoclj_bytearray_t * s = _str_store_unchecked(a);
    if (s && --(s->refcnt) == 0) {
      free_bytearray(sc, s);
    }
  }
    break;
  case T_VECTOR:
  case T_ARRAYMAP:
  case T_SORTED_SET:
  case T_QUEUE:{
    nanoclj_valarray_t * s = _vec_store_unchecked(a);
    if (s && --(s->refcnt) == 0) {
      free_valarray(sc, s);
    }
  }
    break;
  case T_READER:
  case T_WRITER:
  case T_INPUT_STREAM:
  case T_OUTPUT_STREAM:
  case T_GZIP_INPUT_STREAM:
  case T_GZIP_OUTPUT_STREAM:
    port_close(sc, a);
    sc->free(_rep_unchecked(a));
    break;
  case T_IMAGE:{
    nanoclj_image_t * image = _image_unchecked(a);
    if (--(image->refcnt)) {
      sc->free(image->data);
      sc->free(image);
    }
  }
    break;
  case T_AUDIO:{
    nanoclj_audio_t * audio = _audio_unchecked(a);
    if (--(audio->refcnt)) {
      sc->free(audio->data);
      sc->free(audio);
    }
  }
    break;
  case T_REGEX:
    pcre2_code_free(_re_unchecked(a));
    break;
  case T_FOREIGN_OBJECT:
    
    break;
  }
}

#include "nanoclj_gc.h"

/* get new cell.  parameter a, b is marked by gc. */

static inline nanoclj_cell_t * get_cell_x(nanoclj_t * sc, uint16_t type_id, uint8_t flags, nanoclj_cell_t * a, nanoclj_cell_t * b, nanoclj_cell_t * c) {
  nanoclj_shared_context_t * context = sc->context;

  if (!context->free_cell) {
    const int min_to_be_recovered = context->last_cell_seg * 8;
    gc(sc, a, b, c);
    if (!context->free_cell || context->fcells < min_to_be_recovered) {
      /* if only a few recovered, get more to avoid fruitless gc's */
      alloc_cellseg(sc, 1);
    }
  }

  nanoclj_cell_t * x = NULL;
  if (context->free_cell) {
    x = context->free_cell;
    context->free_cell = decode_pointer(_cdr(x));
    if (context->free_cell == &(sc->_EMPTY)) {
      context->free_cell = NULL;
    }
    --context->fcells;

    x->type = type_id;
    x->flags = flags;
    x->hasheq = 0;
  } else {
    sc->pending_exception = sc->OutOfMemoryError;
  }
  return x;
}

/* To retain recent allocs before interpreter knows about them -
   Tehom */

static inline void retain(nanoclj_t * sc, nanoclj_cell_t * recent) {
  nanoclj_cell_t * holder = get_cell_x(sc, T_LIST, 0, recent, NULL, NULL);
  if (holder) {
    _set_car(holder, mk_pointer(recent));
    _set_cdr(holder, _car(&sc->sink));
    _cons_metadata(holder) = NULL;
    _set_car(&sc->sink, mk_pointer(holder));
  }
}

static inline void retain_value(nanoclj_t * sc, nanoclj_val_t recent) {
  if (is_cell(recent)) {
    nanoclj_cell_t * c = decode_pointer(recent);
    if (c) retain(sc, c);
  }
}

static inline nanoclj_cell_t * get_cell(nanoclj_t * sc, uint16_t type, uint8_t flags,
					nanoclj_val_t head, nanoclj_cell_t * tail, nanoclj_cell_t * meta) {
  if (!tail) tail = &(sc->_EMPTY);
  nanoclj_cell_t * cell = get_cell_x(sc, type, flags, is_cell(head) ? decode_pointer(head) : NULL, tail, meta);
  if (cell) {
    _set_car(cell, head);
    _set_cdr(cell, mk_pointer(tail));
    _cons_metadata(cell) = meta;

#if RETAIN_ALLOCS
    retain(sc, cell);
#endif
  }
  return cell;
}

/* Creates a foreign function. For infinite arity, set max_arity to -1. */
static inline nanoclj_val_t mk_foreign_func(nanoclj_t * sc, foreign_func f, int min_arity, int max_arity) {
  nanoclj_cell_t * x = get_cell_x(sc, T_FOREIGN_FUNCTION, T_GC_ATOM, NULL, NULL, NULL);
  if (x) {
    _ff_unchecked(x) = f;
    _min_arity_unchecked(x) = min_arity;
    _max_arity_unchecked(x) = max_arity;
    _ff_metadata(x) = NULL;
    
#if RETAIN_ALLOCS
    retain(sc, x);
#endif
  }
  return mk_pointer(x);
}

static inline nanoclj_val_t mk_foreign_object(nanoclj_t * sc, void * o) {
  nanoclj_cell_t * x = get_cell_x(sc, T_FOREIGN_OBJECT, T_GC_ATOM, NULL, NULL, NULL);
  if (x) {
    _fo_unchecked(x) = o;
    _min_arity_unchecked(x) = 0;
    _max_arity_unchecked(x) = -1;
  
#if RETAIN_ALLOCS
    retain(sc, x);
#endif
  }
  return mk_pointer(x);
}

/* Creates a primitive for utf8 codepoint */
static inline nanoclj_val_t mk_codepoint(int c) {
  return (nanoclj_val_t)((SIGNATURE_CODEPOINT << 48) | (uint32_t)c);
}

static inline nanoclj_val_t mk_boolean(bool b) {
  nanoclj_val_t v;
  v.as_long = b ? kTRUE : kFALSE;
  return v;
}

static inline nanoclj_val_t mk_int(int num) {
  return (nanoclj_val_t)((SIGNATURE_LONG << 48) | (uint32_t)num);
}

static inline nanoclj_val_t mk_proc(enum nanoclj_opcode op) {
  return (nanoclj_val_t)((SIGNATURE_PROC << 48) | (uint32_t)op);
}

static inline nanoclj_val_t mk_real(double n) {
  /* Canonize all kinds of NaNs such as -NaN to ##NaN */
  return (nanoclj_val_t)(isnan(n) ? NAN : n);
}

static inline nanoclj_val_t mk_date(nanoclj_t * sc, long long num) {
  nanoclj_cell_t * x = get_cell_x(sc, T_DATE, T_GC_ATOM, NULL, NULL, NULL);
  if (x) {
    _lvalue_unchecked(x) = num;
    
#if RETAIN_ALLOCS
    retain(sc, x);
#endif
  }
  return mk_pointer(x);
}

/* get number atom (integer) */
static inline nanoclj_val_t mk_integer(nanoclj_t * sc, long long num) {
  if (num >= INT_MIN && num <= INT_MAX) {
    return mk_int(num);
  } else {
    nanoclj_cell_t * x = get_cell_x(sc, T_LONG, T_GC_ATOM, NULL, NULL, NULL);
    if (x) {
      _lvalue_unchecked(x) = num;

#if RETAIN_ALLOCS
      retain(sc, x);
#endif
    }
    return mk_pointer(x);
  }
}

static inline nanoclj_val_t mk_image(nanoclj_t * sc, int32_t width, int32_t height,
				     int32_t stride, nanoclj_internal_format_t f,
				     nanoclj_cell_t * meta) {
  nanoclj_cell_t * x = get_cell_x(sc, T_IMAGE, T_GC_ATOM, NULL, NULL, meta);
  if (x) {
    size_t size = get_image_size(width, height, stride, f);
    uint8_t * data = sc->malloc(size);
    if (data) {
      nanoclj_image_t * image = sc->malloc(sizeof(nanoclj_image_t));
      if (!image) {
	sc->free(data);
      } else {
	image->refcnt = 1;
	image->data = data;
	image->width = width;
	image->height = height;
	image->stride = stride;
	image->format = f;

	_image_unchecked(x) = image;
	_image_metadata(x) = meta;
	
#if RETAIN_ALLOCS
	retain(sc, x);
#endif
	return mk_pointer(x);
      }
    }
  }
  sc->pending_exception = sc->OutOfMemoryError;
  return mk_nil();
}

static inline nanoclj_val_t mk_audio(nanoclj_t * sc, size_t frames, int32_t channels, int32_t sample_rate) {
  nanoclj_cell_t * x = get_cell_x(sc, T_AUDIO, T_GC_ATOM, NULL, NULL, NULL);
  if (x) {
    size_t size = frames * channels * sizeof(float);
    float * data = sc->malloc(size);
    if (data) {
      nanoclj_audio_t * audio = sc->malloc(sizeof(nanoclj_audio_t));
      if (!audio) {
	sc->free(data);
      } else {
	audio->refcnt = 1;
	audio->data = data;
	audio->frames = frames;
	audio->channels = channels;
	audio->sample_rate = sample_rate;
	_audio_unchecked(x) = audio;
	
#if RETAIN_ALLOCS
	retain(sc, x);
#endif
	return mk_pointer(x);
      }
    }
  }
  sc->pending_exception = sc->OutOfMemoryError;
  return mk_nil();
}

static inline long long gcd_int64(long long a, long long b) {
  long long temp;
  while (b != 0) {
    temp = a % b;
    a = b;
    b = temp;
  }
  return a;
}

static inline long long normalize(long long num, long long den, long long * output_den) {
  assert(den != 0);
  if (den == 0) {
    *output_den = 0;
    if (num > 0) return 1;
    else if (num < 0) return -1;
    else return 0;
  } else if (num == 0) {
    *output_den = 1;
    return 0;
  } else {
    long long g = gcd_int64(num, den);
    num /= g;
    den /= g;
    /* Ensure that the denominator is positive */
    if (den < 0) {
      num = -num;
      den = -den;
    }
    *output_den = den;
    return num;
  }
}

static inline long long get_ratio(nanoclj_val_t n, long long * den) {
  switch (prim_type(n)) {
  case T_LONG:
    *den = 1;
    return decode_integer(n);
  case T_NIL:{
    nanoclj_cell_t * c = decode_pointer(n);
    switch (_type(c)) {
    case T_LONG:
      *den = 1;
      return _lvalue_unchecked(c);
    case T_RATIO:
      *den = to_long(vector_elem(c, 1));
      return to_long(vector_elem(c, 0));
    }
  }
  }
  *den = 1;
  return 0;
}

/* Returns a null-terminated C string based on string view. */
static inline char * alloc_c_str(nanoclj_t * sc, strview_t sv) {
  char * buffer = sc->malloc(sv.size + 1);
  if (!buffer) {
    sc->pending_exception = sc->OutOfMemoryError;
  } else {
    memcpy(buffer, sv.ptr, sv.size);
    buffer[sv.size] = 0;
  }
  return buffer;
}

/* Creates a string store */
static inline nanoclj_bytearray_t * mk_bytearray(nanoclj_t * sc, size_t len, size_t padding) {
  nanoclj_bytearray_t * s = sc->malloc(sizeof(nanoclj_bytearray_t));
  s->data = sc->malloc((len + padding) * sizeof(char));
  s->size = len;
  s->reserved = len + padding;
  s->refcnt = 1;
  return s;
}

static inline void initialize_string_object(nanoclj_cell_t * s, size_t offset, size_t size, nanoclj_bytearray_t * store) {
  if (store) {
    s->flags &= ~T_SMALL;
    _str_store_unchecked(s) = store;
    _size_unchecked(s) = size;
    _offset_unchecked(s) = offset;
  } else {
    s->flags |= T_SMALL;
    s->so_size = size;
  }  
}

static inline nanoclj_cell_t * _get_string_object(nanoclj_t * sc, int32_t t, size_t offset, size_t size, nanoclj_bytearray_t * store) {
  nanoclj_cell_t * x = get_cell_x(sc, t, T_GC_ATOM, NULL, NULL, NULL);
  if (x) {
    initialize_string_object(x, offset, size, store);
    
#if RETAIN_ALLOCS
    retain(sc, x);
#endif
  }
  return x;
}

static inline nanoclj_cell_t * get_string_object(nanoclj_t * sc, int32_t t, const char *str, size_t len, size_t padding) {
  nanoclj_bytearray_t * s = 0;
  if (len + padding > NANOCLJ_SMALL_STR_SIZE) {
    s = mk_bytearray(sc, len, padding);
  }
  nanoclj_cell_t * x = _get_string_object(sc, t, 0, len, s);
  if (x && str) {
    memcpy(_strvalue(x), str, len);
  }
  return x;
}

/* get new string */
static inline nanoclj_val_t mk_string(nanoclj_t * sc, const char *str) {
  if (str) {
    return mk_pointer(get_string_object(sc, T_STRING, str, strlen(str), 0));
  } else {
    return mk_nil();
  }
}

static inline nanoclj_val_t mk_string_fmt(nanoclj_t * sc, char *fmt, ...) {
  /* First try printing to a small string, and get the actual size */
  nanoclj_cell_t * r = get_string_object(sc, T_STRING, NULL, NANOCLJ_SMALL_STR_SIZE, 0);
  va_list ap;
  va_start(ap, fmt);
  int n = vsnprintf(_strvalue(r), NANOCLJ_SMALL_STR_SIZE, fmt, ap);
  va_end(ap);

  if (n < NANOCLJ_SMALL_STR_SIZE) {
    r->so_size = n; /* Set the small string size */
  } else { /* Reserve 1-byte padding for the 0-marker */
    initialize_string_object(r, 0, n, mk_bytearray(sc, n, 1));
    
    va_start(ap, fmt);
    vsnprintf(_strvalue(r), n + 1, fmt, ap);
    va_end(ap);
  }
  return mk_pointer(r);
}

static inline nanoclj_val_t mk_string_from_sv(nanoclj_t * sc, strview_t sv) {
  return mk_pointer(get_string_object(sc, T_STRING, sv.ptr, sv.size, 0));
}

static inline nanoclj_val_t mk_string_with_bytearray(nanoclj_t * sc, nanoclj_bytearray_t * b) {
  return mk_pointer(_get_string_object(sc, T_STRING, 0, b->size, b));
}

static inline nanoclj_cell_t * mk_exception(nanoclj_t * sc, nanoclj_cell_t * type, const char * msg) {
  return get_cell(sc, type->type, 0, mk_string(sc, msg), NULL, NULL);
}

static inline nanoclj_val_t valarray_peek(nanoclj_valarray_t * vec) {
  if (vec->size) {
    return vec->data[vec->size - 1];
  } else {
    return mk_nil();
  }
}

static inline nanoclj_val_t add_exception_source(nanoclj_t * sc, strview_t msg) {
  int line = 0;
  const char * file = "NO_SOURCE_FILE";
  nanoclj_val_t p0 = valarray_peek(sc->load_stack);
  if (is_cell(p0)) {
    nanoclj_cell_t * p = decode_pointer(p0);
    line = _line_unchecked(p) + 1;
    if (is_readable(p) && _port_type_unchecked(p) == port_file) {
      nanoclj_port_rep_t * pr = _rep_unchecked(p);
      file = pr->stdio.filename;
    }
  }
  return mk_string_fmt(sc, "%.*s (%s:%d)", (int)msg.size, msg.ptr, file, line);
}

static inline nanoclj_cell_t * mk_runtime_exception(nanoclj_t * sc, nanoclj_val_t msg) {
  msg = add_exception_source(sc, to_strview(msg));
  return get_cell(sc, T_RUNTIME_EXCEPTION, 0, msg, NULL, NULL);
}

static inline nanoclj_cell_t * mk_arithmetic_exception(nanoclj_t * sc, const char * msg) {
  return get_cell(sc, T_ARITHMETIC_EXCEPTION, 0, mk_string(sc, msg), NULL, NULL);
}

static inline nanoclj_cell_t * mk_class_cast_exception(nanoclj_t * sc, const char * msg) {
  return get_cell(sc, T_CLASS_CAST_EXCEPTION, 0, mk_string(sc, msg), NULL, NULL);
}

static inline nanoclj_cell_t * mk_illegal_state_exception(nanoclj_t * sc, const char * msg) {
  return get_cell(sc, T_ILLEGAL_STATE_EXCEPTION, 0, mk_string(sc, msg), NULL, NULL);
}

static inline nanoclj_cell_t * mk_arity_exception(nanoclj_t * sc, int n_args, nanoclj_val_t ns, nanoclj_val_t fn) {
  nanoclj_val_t msg0;
  if (is_nil(fn)) {
    msg0 = mk_string_fmt(sc, "Wrong number of args (%d)", n_args);
  } else {
    strview_t sv1 = to_strview(ns), sv2 = to_strview(fn);
    msg0 = mk_string_fmt(sc, "Wrong number of args (%d) passed to %.*s/%.*s", n_args, (int)sv1.size, sv1.ptr, (int)sv2.size, sv2.ptr);
  }
  nanoclj_val_t msg = add_exception_source(sc, to_strview(msg0));
  return get_cell(sc, T_ARITY_EXCEPTION, 0, msg, NULL, NULL);
}

static inline nanoclj_cell_t * mk_illegal_arg_exception(nanoclj_t * sc, const char * msg) {
  return get_cell(sc, T_ILLEGAL_ARG_EXCEPTION, 0, mk_string(sc, msg), NULL, NULL);
}

static inline nanoclj_cell_t * mk_number_format_exception(nanoclj_t * sc, nanoclj_val_t msg) {
  return get_cell(sc, T_NUM_FMT_EXCEPTION, 0, msg, NULL, NULL);
}

static inline int32_t inchar_raw(nanoclj_cell_t * p) {
  nanoclj_port_rep_t * pr = _rep_unchecked(p);
  switch (_port_type_unchecked(p)) {
  case port_file:
    return fgetc(pr->stdio.file);
  case port_string:
    if (pr->string.read_pos == pr->string.data->size) {
      return EOF;
    } else {
      return (unsigned char)pr->string.data->data[pr->string.read_pos++];
    }
  }
  return EOF;
}

static inline int32_t inchar_utf8(nanoclj_cell_t * p) {
  int c = inchar_raw(p);
  if (c < 0) return EOF;
  uint32_t codepoint = c;
  switch (utf8_sequence_length(c)) {
  case 2:
    codepoint = ((codepoint << 6) & 0x7ff) + (inchar_raw(p) & 0x3f);
    break;
  case 3:
    codepoint = ((codepoint << 12) & 0xffff) + ((inchar_raw(p) << 6) & 0xfff);
    codepoint += inchar_raw(p) & 0x3f;
    break;
  case 4:
    codepoint = ((codepoint << 18) & 0x1fffff) + ((inchar_raw(p) << 12) & 0x3ffff);
    codepoint += (inchar_raw(p) << 6) & 0xfff;
    codepoint += inchar_raw(p) & 0x3f;
    break;
  }
  if (_port_flags_unchecked(p) & PORT_SAW_EOF) {
    return EOF;
  } else {
    return codepoint;
  }
}

static inline void update_cursor(int32_t c, nanoclj_cell_t * p, int line_len) {
  if (_port_type_unchecked(p) == port_file) {
    nanoclj_port_rep_t * pr = _rep_unchecked(p);
    switch (c) {
    case 8:
      if (_column_unchecked(p) > 0) {
	_column_unchecked(p)--;
      }
      break;
    case 10:
      _column_unchecked(p) = 0;
      _line_unchecked(p)++;
      break;
    case 13:
      _column_unchecked(p) = 0;
      break;
    default:
      _column_unchecked(p) += utf8proc_charwidth(c);
      if (line_len > 0 && pr->stdio.file == stdout) {
	while (_column_unchecked(p) >= line_len) {
	  _column_unchecked(p) -= line_len;
	  _line_unchecked(p)++;
	}	
      }
    }
  }
}

/* get new codepoint from input file */
static inline int32_t inchar(nanoclj_cell_t * p) {
  if (_port_flags_unchecked(p) & PORT_SAW_EOF) {
    return EOF;
  }
  int32_t c;
  switch (_type(p)) {
  case T_READER: /* Text */
    c = inchar_utf8(p);
    break;
  default:
    c = inchar_raw(p);
  }
  if (c == EOF) {
    /* Instead, set port_saw_EOF */
    _port_flags_unchecked(p) |= PORT_SAW_EOF;
  } else {
    update_cursor(c, p, 0);
  }
  return c;
}

/* back codepoint to input buffer */
static inline void backchar_raw(uint8_t c, nanoclj_cell_t * p) {
  if (c == EOF) return;
  nanoclj_port_rep_t *pr = _rep_unchecked(p);
  switch (_port_type_unchecked(p)) {
  case port_file:
    ungetc(c, pr->stdio.file);
    break;
  case port_string:
    if (pr->string.read_pos > 0) {
      --pr->string.read_pos;
    }
    break;
  }
}

static inline void backchar(int32_t c, nanoclj_cell_t * p) {
  utf8proc_uint8_t buff[4];
  size_t l = utf8proc_encode_char(c, &buff[0]);
  for (size_t i = 0; i < l; i++) {
    backchar_raw(buff[i], p);
  }
}

/* Medium level cell allocation */

/* get new cons cell */
static inline nanoclj_cell_t * cons(nanoclj_t * sc, nanoclj_val_t head, nanoclj_cell_t * tail) {
  return get_cell(sc, T_LIST, 0, head, tail, NULL);
}

/* Creates a vector store */
static inline nanoclj_valarray_t * mk_valarray(nanoclj_t * sc, size_t len, size_t reserve) {
  nanoclj_val_t * data = sc->malloc(reserve * sizeof(nanoclj_val_t));
  if (data) {
    nanoclj_valarray_t * s = sc->malloc(sizeof(nanoclj_valarray_t));
    if (!s) {
      sc->free(data);
    } else if (s) {
      s->data = data;
      s->size = len;
      s->reserved = reserve;
      s->refcnt = 1;
      s->meta = NULL;
      return s;
    }
  }
  sc->pending_exception = sc->OutOfMemoryError;
  return NULL;
}

static inline nanoclj_cell_t * _get_vector_object(nanoclj_t * sc, int_fast16_t t, size_t offset, size_t size, nanoclj_valarray_t * store) {
  nanoclj_cell_t * x = get_cell_x(sc, t, T_GC_ATOM, NULL, NULL, NULL);
  if (x) {
    if (store) {
      x->flags = T_GC_ATOM;
      _size_unchecked(x) = size;
      _vec_store_unchecked(x) = store;
      _offset_unchecked(x) = offset;
    } else {
      x->flags |= T_SMALL;
      x->so_size = size;
    }
    
#if RETAIN_ALLOCS
    retain(sc, x);
#endif
  }
  return x;
}

static inline nanoclj_cell_t * get_vector_object(nanoclj_t * sc, int_fast16_t t, size_t size) {
  nanoclj_valarray_t * store = NULL;
  if (size > NANOCLJ_SMALL_VEC_SIZE) {
    store = mk_valarray(sc, size, size);
    if (!store) return NULL;
  }
  return _get_vector_object(sc, t, 0, size, store);
}

static inline bool resize_vector(nanoclj_t * sc, nanoclj_cell_t * vec, size_t new_size) {
  size_t old_size = get_size(vec);
  if (new_size <= NANOCLJ_SMALL_VEC_SIZE) {
    if (!_is_small(vec)) {
      /* TODO: release the old store */
      vec->flags = T_SMALL | T_GC_ATOM;
    }
    vec->so_size = new_size;    
  } else {
    if (_is_small(vec)) {
      nanoclj_valarray_t * new_store = mk_valarray(sc, new_size, new_size);
      if (!new_store) return false;
      _vec_store_unchecked(vec) = new_store;
      vec->flags = T_GC_ATOM;
    } else {
      /* TODO: resize (and copy if needed) the old store */
    }
    _size_unchecked(vec) = new_size;
    _offset_unchecked(vec) = 0;
  }
  for (; old_size < new_size; old_size++) {
    set_vector_elem(vec, old_size, mk_nil());
  }
  return true;
}

static inline nanoclj_cell_t * mk_vector(nanoclj_t * sc, size_t len) {
  return get_vector_object(sc, T_VECTOR, len);
}

static inline nanoclj_val_t mk_vector_with_valarray(nanoclj_t * sc, nanoclj_valarray_t * a) {
  return mk_pointer(_get_vector_object(sc, T_VECTOR, 0, a->size, a));
}

static inline nanoclj_val_t mk_mapentry(nanoclj_t * sc, nanoclj_val_t key, nanoclj_val_t val) {
  nanoclj_cell_t * vec = get_vector_object(sc, T_MAPENTRY, 2);
  if (vec) {
    set_vector_elem(vec, 0, key);
    set_vector_elem(vec, 1, val);
  }
  return mk_pointer(vec);
}

static inline nanoclj_val_t mk_ratio(nanoclj_t * sc, nanoclj_val_t num, nanoclj_val_t den) {  
  nanoclj_cell_t * vec = get_vector_object(sc, T_RATIO, 2);
  if (vec) {
    set_vector_elem(vec, 0, num);
    set_vector_elem(vec, 1, den);
  }
  return mk_pointer(vec);
}

static inline nanoclj_val_t mk_ratio_long(nanoclj_t * sc, long long num, long long den) {
  return mk_ratio(sc, mk_integer(sc, num), mk_integer(sc, den));
}

static inline uint64_t get_mantissa(nanoclj_val_t a) {
  return (1ULL << 52) + (a.as_long & 0xFFFFFFFFFFFFFULL);
}

static inline int_fast16_t get_exponent(nanoclj_val_t a) {
  return ((a.as_long & (0x7FFULL << 52)) >> 52) - 1023;
}

static inline int_fast8_t number_of_trailing_zeros(uint64_t a) {
  int n = 0;
  while ((a & 1) == 0) {
    n++;
    a >>= 1;
  }
  return n;
}

/* Creates an exact rational representation of a (normalized) double (or overflows) */
static inline nanoclj_val_t mk_ratio_from_double(nanoclj_t * sc, nanoclj_val_t a) {
  if (isnan(a.as_double)) {
    return mk_ratio_long(sc, 0, 0);
  } else if (!isfinite(a.as_double)) {
    return mk_ratio_long(sc, a.as_double < 0 ? -1 : 1, 0);
  }
  long long numerator = get_mantissa(a);
  int_fast16_t exponent = get_exponent(a) - 52;
  while ((numerator & 1) == 0) {
    numerator >>= 1;
    exponent++;
  }
  if (a.as_double < 0) numerator *= -1;

  if (exponent <= -63 || exponent >= 63) {
    return mk_nil(); /* TODO: use BigInt */
  } else if (exponent < 0) {    
    return mk_ratio_long(sc, numerator, 1ULL << -exponent);
  } else {
    return mk_integer(sc, numerator << exponent);
  }
}

static inline nanoclj_val_t mk_regex(nanoclj_t * sc, nanoclj_val_t pattern) {
  nanoclj_cell_t * x = get_cell_x(sc, T_REGEX, T_GC_ATOM, NULL, NULL, NULL);
  if (x) {    
    int errornumber;
    PCRE2_SIZE erroroffset;
    strview_t sv = to_strview(pattern);
    pcre2_code * re = pcre2_compile((PCRE2_SPTR8)sv.ptr,
				    sv.size,
				    PCRE2_UTF | PCRE2_NEVER_BACKSLASH_C,
				    &errornumber,
				    &erroroffset,
				    NULL);
    if (re) {
      _re_unchecked(x) = re;

#if RETAIN_ALLOCS
      retain(sc, x);
#endif
      return mk_pointer(x);
    }
  }
  return mk_nil();
}

static inline dump_stack_frame_t * s_add_frame(nanoclj_t * sc) {
  /* enough room for the next frame? */
  if (sc->dump >= sc->dump_size) {
    if (sc->dump_size == 0) sc->dump_size = 256;
    else {
      sc->dump_size *= 2;
      fprintf(stderr, "reallocing dump stack (%zu)\n", sc->dump_size);
    }	  
    sc->dump_base = sc->realloc(sc->dump_base, sizeof(dump_stack_frame_t) * sc->dump_size);
  }
  return sc->dump_base + sc->dump++;
}

static inline void save_from_C_call(nanoclj_t * sc) { 
  dump_stack_frame_t * next_frame = s_add_frame(sc);
  next_frame->op = 0;
  next_frame->args = sc->args;
  next_frame->envir = sc->envir;
  next_frame->code = sc->code;
}
 
static inline nanoclj_val_t eval(nanoclj_t * sc, nanoclj_cell_t * obj) {
  save_from_C_call(sc);

  sc->args = NULL;
  sc->code = mk_pointer(obj);

  Eval_Cycle(sc, OP_EVAL);
  return sc->value;
}

/* Sequence handling */

static inline nanoclj_cell_t * subvec(nanoclj_t * sc, nanoclj_cell_t * vec, size_t offset, size_t len) {
  if (_is_small(vec)) {
    nanoclj_cell_t * new_vec = get_vector_object(sc, vec->type, len);
    for (size_t i = 0; i < len; i++) {
      set_vector_elem(new_vec, i, vector_elem(vec, offset + i));
    }
    return new_vec;
  } else {
    nanoclj_valarray_t * s = _vec_store_unchecked(vec);
    size_t old_size = _size_unchecked(vec);
    size_t old_offset = _offset_unchecked(vec);
    
    if (offset >= old_size) {
      len = 0;
    } else if (offset + len > old_size) {
      len = old_size - offset;
    }

    s->refcnt++;
    
    return _get_vector_object(sc, vec->type, old_offset + offset, len, s);
  }
}

static inline nanoclj_cell_t * remove_prefix(nanoclj_t * sc, nanoclj_cell_t * coll, size_t n) {
  if (is_string_type(_type(coll))) {
    const char * old_ptr = _strvalue(coll);
    const char * new_ptr = old_ptr;
    const char * end_ptr = old_ptr + get_size(coll);
    while (n > 0 && new_ptr < end_ptr) {
      new_ptr = utf8_next(new_ptr);
      n--;
    }  
    size_t new_size = end_ptr - new_ptr;
    
    if (_is_small(coll)) {
      return get_string_object(sc, coll->type, new_ptr, new_size, 0);
    } else {
      size_t new_offset = new_ptr - old_ptr;
      nanoclj_bytearray_t * s = _str_store_unchecked(coll);
      s->refcnt++;
      return _get_string_object(sc, coll->type, _offset_unchecked(coll) + new_offset, new_size, s);
    }
  } else {
    return subvec(sc, coll, n, get_size(coll) - n);
  }
}

static inline nanoclj_cell_t * remove_suffix(nanoclj_t * sc, nanoclj_cell_t * coll, size_t n) {
  if (is_string_type(_type(coll))) {
    const char * start = _strvalue(coll);
    const char * end = start + get_size(coll);
    const char * new_ptr = end;
    while (n > 0 && new_ptr > start) {
      new_ptr = utf8_prev(new_ptr);
      n--;
    }
    if (_is_small(coll)) {
      return get_string_object(sc, coll->type, start, new_ptr - start, 0);
    } else {
      return coll;
    }
  } else {
    return subvec(sc, coll, 0, get_size(coll) - 1);
  }
}

/* Creates a reverse sequence of a vector or string */
static inline nanoclj_cell_t * rseq(nanoclj_t * sc, nanoclj_cell_t * coll) {
  size_t size = get_size(coll);
  if (!size) {
    return NULL;
  } else if (is_string_type(_type(coll))) {
    if (_is_small(coll)) {
      nanoclj_cell_t * new_str = get_string_object(sc, coll->type, NULL, size, 0);
      _set_seq(new_str); /* small strings are reversed in-place */
      char * old_ptr = _strvalue(coll);
      char * new_ptr = _strvalue(new_str) + size;
      while (size > 0) {
	const char * tmp = utf8_next(old_ptr);
	size_t char_size = tmp - old_ptr;
	memcpy(new_ptr - char_size, old_ptr, char_size);
	old_ptr += char_size;
	new_ptr -= char_size;
	size -= char_size;
      }
      return new_str;
    } else {
      return NULL;
    }
  } else if (_is_small(coll)) {
    nanoclj_cell_t * new_vec = get_vector_object(sc, _type(coll), size);
    for (size_t i = 0; i < size; i++) {
      set_vector_elem(new_vec, size - 1 - i, vector_elem(coll, i));
    }
    _set_seq(new_vec); /* small vectors are reversed in-place */
    return new_vec;
  } else {
    nanoclj_valarray_t * s = _vec_store_unchecked(coll);
    size_t old_size = _size_unchecked(coll);
    size_t old_offset = _offset_unchecked(coll);

    s->refcnt++;
    
    nanoclj_cell_t * new_vec = _get_vector_object(sc, _type(coll), old_offset, old_size, s);
    _set_rseq(new_vec);
    return new_vec;
  }
}

static inline void s_save(nanoclj_t * sc, enum nanoclj_opcode op,
			  nanoclj_cell_t * args, nanoclj_val_t code) {  
  dump_stack_frame_t * next_frame = s_add_frame(sc);
  next_frame->op = op;
  next_frame->args = args;
  next_frame->envir = sc->envir;
  next_frame->code = code;
#ifdef USE_RECUR_REGISTER
  next_frame->recur = sc->recur;
#endif
}

static inline nanoclj_cell_t * deref(nanoclj_t * sc, nanoclj_cell_t * c) {
  if (_is_realized(c)) {
    if (is_nil(_car(c)) && is_nil(_cdr(c))) {
      return NULL;
    } else {
      return c;
    }
  } else {
    /* Should change type to closure here */
    save_from_C_call(sc);
    nanoclj_val_t v = mk_pointer(c);
    s_save(sc, OP_SAVE_FORCED, NULL, v);
    sc->code = v;
    sc->args = NULL;
    Eval_Cycle(sc, OP_APPLY);
    return decode_pointer(sc->value);
  }
}

/* Returns nil or not-empty sequence */
static inline nanoclj_cell_t * seq(nanoclj_t * sc, nanoclj_cell_t * coll) {
  if (!coll) {
    return NULL;
  }
  if (_type(coll) == T_LAZYSEQ) {
    coll = deref(sc, coll);    
  }

  if (_is_sequence(coll)) {
    return coll;
  }

  switch (_type(coll)) {
  case T_NIL:
  case T_LONG:
  case T_DATE:
  case T_MAPENTRY:
  case T_CLOSURE:
  case T_MACRO:
    return NULL;
  case T_STRING:
  case T_CHAR_ARRAY:
  case T_FILE:
  case T_UUID:
  case T_VECTOR:
  case T_ARRAYMAP:
  case T_SORTED_SET:
  case T_QUEUE:
    if (get_size(coll) == 0) {
      return NULL;
    } else {
      nanoclj_cell_t * s = remove_prefix(sc, coll, 0);
      _set_seq(s);
      return s;
    }
    break;
  }

  return coll;
}

static inline bool is_empty(nanoclj_t * sc, nanoclj_cell_t * coll) {
  if (!coll) {
    return true;
  } else if (_is_sequence(coll)) {
    return false;
  } else if (_type(coll) == T_LAZYSEQ) {
    coll = deref(sc, coll);
    if (!coll) return true;    
  }

  switch (_type(coll)) {
  case T_NIL:
    return true;
  case T_STRING:
  case T_CHAR_ARRAY:
  case T_FILE:
  case T_UUID:
  case T_VECTOR:
  case T_ARRAYMAP:
  case T_SORTED_SET:
  case T_MAPENTRY:
  case T_RATIO:
  case T_QUEUE:
    return get_size(coll) == 0;
  }

  return false;
}

static inline nanoclj_cell_t * rest(nanoclj_t * sc, nanoclj_cell_t * coll) {
  if (coll) {
    int typ = _type(coll);
    
    if (typ == T_LAZYSEQ) {
      coll = deref(sc, coll);
      if (!coll) return &(sc->_EMPTY);
    }
    
    switch (typ) {
    case T_NIL:
      break;
    case T_LIST:
    case T_LAZYSEQ:
    case T_ENVIRONMENT:
    case T_CLASS:
      return decode_pointer(_cdr(coll));
    case T_VECTOR:
    case T_ARRAYMAP:
    case T_SORTED_SET:
    case T_STRING:
    case T_CHAR_ARRAY:
    case T_FILE:
    case T_QUEUE:
      if (get_size(coll) >= 2) {
	nanoclj_cell_t * s;
	if (_is_reverse(coll)) {
	  s = remove_suffix(sc, coll, 1);
	  _set_rseq(s);
	} else {
	  s = remove_prefix(sc, coll, 1);
	  _set_seq(s);
	}
	/* In the case of multi-byte utf8 character, the string can be empty */
	return get_size(s) ? s : &(sc->_EMPTY);
      }
      break;
    }
  }
  
  return &(sc->_EMPTY);
}

static inline nanoclj_cell_t * next(nanoclj_t * sc, nanoclj_cell_t * coll) {
  if (!coll) return NULL;
  nanoclj_cell_t * r = rest(sc, coll);
  if (_type(r) == T_LAZYSEQ) {
    r = deref(sc, r);    
  }
  if (r == &(sc->_EMPTY)) {
    return NULL;
  }
  return r;
}

static inline nanoclj_val_t first(nanoclj_t * sc, nanoclj_cell_t * coll) {
  if (coll == NULL) {
    return mk_nil();
  } else if (_type(coll) == T_LAZYSEQ) {
    coll = deref(sc, coll);
    if (!coll) return mk_nil();
  }
  
  switch (_type(coll)) {
  case T_NIL:
    break;
  case T_CLOSURE:
  case T_MACRO:
  case T_ENVIRONMENT:
  case T_LIST:
  case T_LAZYSEQ:
  case T_CLASS:
    return _car(coll);
  case T_VECTOR:
  case T_ARRAYMAP:
  case T_SORTED_SET:
  case T_RATIO:
  case T_MAPENTRY:
  case T_VAR:
  case T_QUEUE:
    if (get_size(coll) > 0) {
      if (_is_reverse(coll)) {
	return vector_elem(coll, get_size(coll) - 1);
      } else {
	return vector_elem(coll, 0);
      }
    }
    break;
  case T_STRING:
  case T_CHAR_ARRAY:
  case T_FILE:
    if (get_size(coll)) {
      if (_is_reverse(coll)) {
	const char * start = _strvalue(coll);
	const char * end = start + get_size(coll);
	const char * last = utf8_prev(end);
	return mk_codepoint(decode_utf8(last));
      } else {
	return mk_codepoint(decode_utf8(_strvalue(coll)));
      }
    }
    break;
  }

  return mk_nil();
}

static inline size_t count(nanoclj_t * sc, nanoclj_cell_t * coll) {
  if (!coll) return 0;
  else if (_type(coll) == T_LAZYSEQ) {
    coll = deref(sc, coll);
    if (!coll) return 0;
  }

  switch (_type(coll)) {
  case T_NIL:
    return 0;

  case T_LIST:
  case T_LAZYSEQ:
  case T_ENVIRONMENT:
  case T_CLASS:
    return 1 + count(sc, next(sc, coll));
    
  case T_STRING:
  case T_CHAR_ARRAY:
  case T_FILE:
  case T_UUID:
    return utf8_num_codepoints(_strvalue(coll), get_size(coll));
    
  case T_VECTOR:
  case T_ARRAYMAP:
  case T_SORTED_SET:
  case T_MAPENTRY:
  case T_RATIO:
  case T_QUEUE:
    return get_size(coll);
  }

  return 0;
}

static nanoclj_val_t second(nanoclj_t * sc, nanoclj_cell_t * a) {
  if (a) {
    int t = _type(a);
    if (is_vector_type(t)) {
      if (get_size(a) >= 2) {
	return vector_elem(a, 1);
      }
    } else if (t != T_NIL) {
      return first(sc, rest(sc, a));
    }
  }
  return mk_nil();
}

static nanoclj_val_t third(nanoclj_t * sc, nanoclj_cell_t * a) {
  if (a) {
    int t = _type(a);
    if (is_vector_type(t)) {
      if (get_size(a) >= 3) {
	return vector_elem(a, 2);
      }
    } else if (t != T_NIL) {
      return first(sc, rest(sc, rest(sc, a)));
    }
  }
  return mk_nil();
}

static nanoclj_val_t fourth(nanoclj_t * sc, nanoclj_cell_t * a) {
  if (a) {
    int t = _type(a);
    if (is_vector_type(t)) {
      if (get_size(a) >= 4) {
	return vector_elem(a, 3);
      }
    } else if (t != T_NIL) {
      return first(sc, rest(sc, rest(sc, rest(sc, a))));
    }
  }
  return mk_nil();
}

static inline bool equals(nanoclj_t * sc, nanoclj_val_t a0, nanoclj_val_t b0) {
  int t_a = prim_type(a0), t_b = prim_type(b0);
  if (t_a == T_REAL && t_b == T_REAL) {
    return a0.as_double == b0.as_double; /* 0.0 == -0.0 */
  } else if (a0.as_long == b0.as_long) {
    return true;
  } else if (t_a || t_b || a0.as_long == kNIL || b0.as_long == kNIL) {
    return false;
  } else {
    nanoclj_cell_t * a = decode_pointer(a0), * b = decode_pointer(b0);
    t_a = _type(a);
    t_b = _type(b);
    if (t_a == t_b) {
      switch (t_a) {
      case T_STRING:
      case T_CHAR_ARRAY:
      case T_FILE:
      case T_UUID:
	return strview_eq(_to_strview(a), _to_strview(b));
      case T_LONG:
      case T_DATE:
	return _lvalue_unchecked(a) == _lvalue_unchecked(b);
      case T_VECTOR:
      case T_SORTED_SET:
      case T_RATIO:
      case T_VAR:
      case T_MAPENTRY:
      case T_QUEUE:{
	size_t l = get_size(a);
	if (l == get_size(b)) {
	  for (size_t i = 0; i < l; i++) {
	    if (!equals(sc, vector_elem(a, i), vector_elem(b, i))) {
	      return 0;
	    }
	  }
	  return 1;
	}
      }
      case T_ARRAYMAP:{
	size_t l = get_size(a);
	if (l == get_size(b)) {
	  for (size_t i = 0; i < l; i++) {
	    nanoclj_cell_t * ea = decode_pointer(vector_elem(a, i));
	    nanoclj_val_t key = vector_elem(ea, 0), val = vector_elem(ea, 1);
	    size_t j = 0;
	    for (; j < l; j++) {
	      nanoclj_cell_t * eb = decode_pointer(vector_elem(b, j));
	      if (equals(sc, key, vector_elem(eb, 0)) && equals(sc, val, vector_elem(eb, 1))) {
		break;
	      }
	    }
	    if (j == l) return false;
	  }
	  return true;
	}
      }
	break;
      case T_LIST:
      case T_CLOSURE:
      case T_LAZYSEQ:
      case T_MACRO:
      case T_ENVIRONMENT:
	if (equals(sc, _car(a), _car(b))) {
	  return equals(sc, _cdr(a), _cdr(b));
	}
	break;	
      }
    } else if (is_coll_type(t_a) && is_coll_type(t_b)) {
      for (a = seq(sc, a), b = seq(sc, b); a && b; a = next(sc, a), b = next(sc, b)) {
	if (!equals(sc, first(sc, a), first(sc, b))) {
	  return false;
	}
      }
      if (!a && !b) {
	return true;
      }
    }
  }
  
  return false;
}

static inline nanoclj_cell_t * find(nanoclj_t * sc, nanoclj_cell_t * map, nanoclj_val_t key) {
  size_t size = get_size(map);
  for (size_t i = 0; i < size; i++) {
    nanoclj_cell_t * entry = decode_pointer(vector_elem(map, i));
    if (equals(sc, key, vector_elem(entry, 0))) {
      return entry;
    }    
  }
  return NULL;
}

static inline size_t find_index(nanoclj_t * sc, nanoclj_cell_t * coll, nanoclj_val_t key) {
  size_t i = 0, size = get_size(coll);
  if (_type(coll) == T_ARRAYMAP) {
    for ( ; i < size; i++) {
      nanoclj_cell_t * entry = decode_pointer(vector_elem(coll, i));
      if (equals(sc, key, vector_elem(entry, 0))) {
	return i;
      }
    }
  } else {
    for ( ; i < size; i++) {
      nanoclj_val_t v = vector_elem(coll, i);
      if (equals(sc, key, v)) {
	return i;
      }
    }
  }
  return NPOS;
}

static inline bool get_elem(nanoclj_t * sc, nanoclj_cell_t * coll, nanoclj_val_t key, nanoclj_val_t * result) {
#if 1
  if (!coll) {
    return false;
  }
#endif
  long long index;
  switch (_type(coll)) {
  case T_VECTOR:
  case T_RATIO:
  case T_MAPENTRY:
  case T_VAR:
    index = to_long_w_def(key, -1);
    if (index >= 0 && index < get_size(coll)) {
      if (result) *result = vector_elem(coll, index);
      return true;
    }
    break;

  case T_STRING:
  case T_CHAR_ARRAY:
  case T_FILE:
    index = to_long_w_def(key, -1);
    if (index >= 0) {
      const char * str = _strvalue(coll);
      const char * end = str + get_size(coll);
      
      for (; index > 0 && str < end; index--) {
	str = utf8_next(str);
      }
      
      if (index == 0 && str < end) {
	if (result) *result = mk_codepoint(decode_utf8(str));
	return true;
      }
    }
    break;
        
  case T_ARRAYMAP:{
    nanoclj_cell_t * e = find(sc, coll, key);
    if (e) {
      if (result) *result = vector_elem(e, 1);
      return true;
    }
  }
    
  case T_SORTED_SET:{
    size_t size = get_size(coll);
    for (int i = 0; i < size; i++) {
      nanoclj_val_t value = vector_elem(coll, i);
      if (equals(sc, key, value)) {
	if (result) *result = value;
	return true;
      }    
    }
  }

  case T_IMAGE:{
    nanoclj_image_t * image = _image_unchecked(coll);
    if (key.as_long == sc->WIDTH.as_long) {
      if (result) *result = mk_int(image->width);
    } else if (key.as_long == sc->HEIGHT.as_long) {
      if (result) *result = mk_int(image->height);
    } else {
      return false;
    }
    return true;
  }

  case T_AUDIO:{
    nanoclj_audio_t * audio = _audio_unchecked(coll);
    if (key.as_long == sc->CHANNELS.as_long) {
      if (result) *result = mk_int(audio->channels);
    } else {
      return false;
    }
    return true;
  }

  case T_WRITER:
  case T_READER:
  case T_OUTPUT_STREAM:
  case T_INPUT_STREAM:
  case T_GZIP_INPUT_STREAM:
  case T_GZIP_OUTPUT_STREAM:
    if (key.as_long == sc->LINE.as_long) {
      if (result) {
	int l = _line_unchecked(coll);
	*result = l != -1 ? mk_int(l) : mk_nil();
      }
    } else if (key.as_long == sc->COLUMN.as_long) {
      if (result) {
	int c = _column_unchecked(coll);
	*result = c != -1 ? mk_int(c) : mk_nil();
      }
    } else if (key.as_long == sc->GRAPHICS.as_long) {
      if (result) *result = mk_boolean(sc->term_graphics != nanoclj_no_gfx);
    } else {
      return false;
    }
    return true;
  }
  
  return false;
}

/* =================== Canvas ====================== */
#ifdef WIN32
#include "nanoclj_gdi.h"
#else
#include "nanoclj_cairo.h"
#endif

/* =================== Tensors ===================== */
#include "nanoclj_ggml.h"

/* =================== HTTP ======================== */
#ifdef WIN32
#include "nanoclj_winhttp.h"
#else
#include "nanoclj_curl.h"
#endif

/* ========== Environment implementation  ========== */

/*
 * In this implementation, each frame of the environment may be
 * a hash table: a vector of alists hashed by variable name.
 * In practice, we use a vector only for the initial frame;
 * subsequent frames are too small and transient for the lookup
 * speed to out-weigh the cost of making a new vector.
 */

static inline void new_frame_in_env(nanoclj_t * sc, nanoclj_cell_t * old_env) {
  sc->envir = get_cell(sc, T_ENVIRONMENT, 0, sc->EMPTY, old_env, NULL);
}

static inline nanoclj_cell_t * new_slot_spec_in_env(nanoclj_t * sc, nanoclj_cell_t * env,
						    nanoclj_val_t variable, nanoclj_val_t value,
						    nanoclj_cell_t * meta) {
  nanoclj_cell_t * slot0 = get_vector_object(sc, T_VAR, 2);
  set_vector_elem(slot0, 0, variable);
  set_vector_elem(slot0, 1, value);
  _so_vector_metadata(slot0) = meta;
    
  nanoclj_val_t slot = mk_pointer(slot0);
  nanoclj_val_t x = _car(env);
  
  if (is_vector(x)) {
    nanoclj_cell_t * vec = decode_pointer(x);
    if (get_size(vec) == 0) resize_vector(sc, vec, 727);
    symbol_t * s = decode_symbol(variable);
    int location = s->hash % _size_unchecked(vec);
    set_vector_elem(vec, location, mk_pointer(cons(sc, slot, decode_pointer(vector_elem(vec, location)))));
  } else {     
    _set_car(env, mk_pointer(cons(sc, slot, decode_pointer(_car(env)))));
  }
  return slot0;
}

static inline nanoclj_cell_t * find_slot_in_env(nanoclj_t * sc, nanoclj_cell_t * env, nanoclj_val_t hdl, bool all) {  
  for (nanoclj_cell_t * x = env; x && x != &(sc->_EMPTY); x = decode_pointer(_cdr(x))) {
    nanoclj_cell_t * y = decode_pointer(_car(x));
    if (_type(y) == T_VECTOR) {
      symbol_t * s = decode_symbol(hdl);
      int location = s->hash % _size_unchecked(y);
      y = decode_pointer(vector_elem(y, location));
    }
	
    for (; y && y != &(sc->_EMPTY); y = decode_pointer(_cdr(y))) {
      nanoclj_cell_t * var = decode_pointer(_car(y));
      nanoclj_val_t sym = vector_elem(var, 0);
      if (sym.as_long == hdl.as_long) {
	return var;
      }
    }

    if (!all) {
      return NULL;
    }
  }
  
  return NULL;
}

static inline void new_slot_in_env(nanoclj_t * sc, nanoclj_val_t variable, nanoclj_val_t value) {
  new_slot_spec_in_env(sc, sc->envir, variable, value, NULL);
}

static inline nanoclj_val_t set_slot_in_env(nanoclj_t * sc, nanoclj_cell_t * slot, nanoclj_val_t state, nanoclj_cell_t * meta) {
  nanoclj_val_t old_state = _cdr(slot);
  set_vector_elem(slot, 1, state);
  if (meta) _so_vector_metadata(slot) = meta;
  else meta = _so_vector_metadata(slot);
  
  nanoclj_val_t watches0;
  if (get_elem(sc, meta, sc->WATCHES, &watches0)) {
    nanoclj_cell_t * watches = seq(sc, decode_pointer(watches0));
    for (; watches; watches = next(sc, watches)) {
      nanoclj_val_t fn = first(sc, watches);
      nanoclj_call(sc, fn, mk_pointer(cons(sc, mk_nil(), cons(sc, mk_pointer(slot), cons(sc, old_state, cons(sc, state, NULL))))));
    }
  }
  return state;
}

static inline nanoclj_val_t slot_value_in_env(nanoclj_cell_t * slot) {
  if (slot) {
    return vector_elem(slot, 1);
  } else {
    return mk_nil();
  }
}

static inline bool equiv(nanoclj_val_t a, nanoclj_val_t b) {
  int type_a = prim_type(a), type_b = prim_type(b);
  if (type_a == T_REAL || type_b == T_REAL) {
    return to_double(a) == to_double(b);
  } else if (a.as_long == b.as_long) {
    return true;
  } else if (type_a && type_b) {
    return false;
  } else {
    type_a = expand_type(a, type_a);
    type_b = expand_type(b, type_b);
    if (type_a == T_RATIO || type_b == T_RATIO) {
      return to_double(a) == to_double(b);
    } else if (type_a == T_LONG || type_b == T_LONG) {
      return to_long(a) == to_long(b);
    } else {
      return false;
    }
  }
}

static inline int compare(nanoclj_val_t a, nanoclj_val_t b) {
  if (a.as_long == b.as_long) {
    return 0;
  } else if (is_nil(a)) {
    return -1;
  } else if (is_nil(b)) {
    return +1;
  } else {
    int type_a = prim_type(a), type_b = prim_type(b);
    if (type_a == T_REAL || type_b == T_REAL) {
      double ra = to_double(a), rb = to_double(b);
      if (ra < rb) return -1;
      else if (ra > rb) return +1;
      return 0; /* 0.0 = -0.0 */
    } else if (is_integral_type(type_a) && is_integral_type(type_b)) {
      int ia = decode_integer(a), ib = decode_integer(b);
      if (ia < ib) return -1;
      else if (ia > ib) return +1;
      return 0;
    } else if ((type_a == T_KEYWORD && type_b == T_KEYWORD) ||
	       (type_a == T_SYMBOL && type_b == T_SYMBOL)) {  
      symbol_t * sa = decode_symbol(a), * sb = decode_symbol(b);
      int i = strview_cmp(sa->ns, sb->ns);
      return i ? i : strview_cmp(sa->name, sb->name);
    } else if (!type_a && !type_b) {
      nanoclj_cell_t * a2 = decode_pointer(a), * b2 = decode_pointer(b);
      type_a = _type(a2);
      type_b = _type(b2);
      if (type_a == type_b) {
	switch (type_a) {
	case T_STRING:
	case T_CHAR_ARRAY:
	case T_FILE:
	case T_UUID:{
	  strview_t sv1 = _to_strview(a2), sv2 = _to_strview(b2);
	  const char * p1 = sv1.ptr, * p2 = sv2.ptr;
	  const char * end1 = sv1.ptr + sv1.size, * end2 = sv2.ptr + sv2.size;
	  while ( 1 ) {
	    if (p1 >= end1 && p2 >= end2) return 0;
	    else if (p1 >= end1) return -1;
	    else if (p2 >= end2) return 1;
	    int d = decode_utf8(p1) - decode_utf8(p2);
	    if (d) return d;
	    p1 = utf8_next(p1), p2 = utf8_next(p2);
	  }
	}
	case T_LONG:
	case T_DATE:{
	  long long la = _lvalue_unchecked(a2), lb = _lvalue_unchecked(b2);
	  if (la < lb) return -1;
	  else if (la > lb) return +1;
	  else return 0;
	}
	  
	case T_RATIO:{
	  double ra = to_double(a), rb = to_double(b);
	  if (ra < rb) return -1;
	  else if (ra > rb) return +1;
	  return 0;
	}
	  
	case T_VECTOR:
	case T_SORTED_SET:
	case T_MAPENTRY:
	case T_VAR:
	case T_QUEUE:{
	  size_t la = get_size(a2), lb = get_size(b2);
	  if (la < lb) return -1;
	  else if (la > lb) return +1;
	  else {
	    for (size_t i = 0; i < la; i++) {
	      int r = compare(vector_elem(a2, i), vector_elem(b2, i));
	      if (r) return r;
	    }
	    return 0;
	  }
	}
	  break;
	  
	case T_ARRAYMAP:{
	  size_t la = get_size(a2), lb = get_size(b2);
	  if (la < lb) return -1;
	  else if (la > lb) return +1;
	  else {
	    /* TODO: implement map comparison */
	    return 0;
	  }
	}

	case T_CLASS:
	  return (int)a2->type - (int)b2->type;
	  
	case T_LIST:
	case T_CLOSURE:
	case T_LAZYSEQ:
	case T_MACRO:
	case T_ENVIRONMENT:
	  {
	    int r = compare(_car(a2), _car(b2));
	    if (r) return r;
	    return compare(_cdr(a2), _cdr(b2));
	  }
	}
      } else if (type_a == T_NIL) {
	return -1;
      } else if (type_b == T_NIL) {
	return +1;
      }
    } else {
      type_a = expand_type(a, type_a);
      type_b = expand_type(b, type_b);
      if (type_a == T_RATIO || type_b == T_RATIO) {
	double ra = to_double(a), rb = to_double(b);
	if (ra < rb) return -1;
	else if (ra > rb) return +1;
	return 0;
      } else if (type_a == T_LONG || type_b == T_LONG) {
	long long la = to_long(a), lb = to_long(b);
	if (la < lb) return -1;
	else if (la > lb) return +1;
	else return 0;
      }
    }
  }
  return 0;
}

static inline int _compare(const void * a, const void * b) {
  return compare(*(const nanoclj_val_t*)a, *(const nanoclj_val_t*)b);
}

static inline void sort_vector_in_place(nanoclj_cell_t * vec) {
  size_t s = get_size(vec);
  if (s > 0) {
    if (_is_small(vec)) {
      qsort(_smalldata_unchecked(vec), s, sizeof(nanoclj_val_t), _compare);
    } else {
      qsort(_vec_store_unchecked(vec)->data + _offset_unchecked(vec), s, sizeof(nanoclj_val_t), _compare);
    }
  }
}

static int32_t hasheq(nanoclj_t * sc, nanoclj_val_t v) { 
  switch (prim_type(v)) {
  case T_CODEPOINT: /* Clojure doesn't use murmur3 for characters */
  case T_PROC:    
    return murmur3_hash_int(decode_integer(v));

  case T_BOOLEAN:
    return v.as_long == kFALSE ? 1237 : 1231;
    
  case T_LONG:
    return murmur3_hash_long(decode_integer(v));

  case T_REAL:
    if (v.as_double == 0.0) return 0;
    else return (int)v.as_long ^ (int)(v.as_long >> 32);

  case T_SYMBOL:
  case T_KEYWORD:
    return decode_symbol(v)->hash;

  case T_NIL:{
    nanoclj_cell_t * c = decode_pointer(v);
    if (!c) return 0;
    
    switch (_type(c)) {
    case T_LONG:
      return murmur3_hash_long(_lvalue_unchecked(c));

    case T_DATE:
      return (int)_lvalue_unchecked(c) ^ (int)(_lvalue_unchecked(c) >> 32);

    case T_STRING:
    case T_CHAR_ARRAY:
    case T_FILE:
    case T_UUID:{
      strview_t sv = _to_strview(c);
      return murmur3_hash_int(get_string_hashcode(sv.ptr, sv.size));
    }
      
    case T_RATIO:
      return hasheq(sc, vector_elem(c, 0)) ^ hasheq(sc, vector_elem(c, 1));
      
    case T_MAPENTRY:
    case T_VECTOR:
    case T_VAR:
    case T_QUEUE:{
      uint32_t hash = 1;
      size_t n = get_size(c);
      for (size_t i = 0; i < n; i++) {
	hash = 31 * hash + (uint32_t)hasheq(sc, vector_elem(c, i));
      }
      return murmur3_hash_coll(hash, n);
    }

    case T_SORTED_SET:
    case T_ARRAYMAP:{
      uint32_t hash = 0;
      size_t n = get_size(c);
      for (size_t i = 0; i < n; i++) {
	hash += (uint32_t)hasheq(sc, vector_elem(c, i));
      }
      return murmur3_hash_coll(hash, n);
    }

    case T_TYPE:
      return murmur3_hash_int(c->type);

    case T_NIL:
      c = NULL;
    case T_LIST:
    case T_LAZYSEQ:{
      uint32_t hash = 1;
      size_t n = 0;
      for ( ; c; c = next(sc, c), n++) {
	hash = 31 * hash + (uint32_t)hasheq(sc, first(sc, c));
      }
      return murmur3_hash_coll(hash, n);
    }
    }
  }
  }
  return 0;
}

static inline void ok_to_freely_gc(nanoclj_t * sc) {
  _set_car(&(sc->sink), sc->EMPTY);
}

static inline nanoclj_val_t nanoclj_throw(nanoclj_t * sc, nanoclj_cell_t * e) {
  sc->pending_exception = e;
  return mk_nil();
}

/* ========== oblist implementation  ========== */

static inline nanoclj_cell_t * oblist_initial_value(nanoclj_t * sc) {
  nanoclj_cell_t * vec = mk_vector(sc, 727);
  fill_vector(vec, mk_nil());
  return vec;
}

static inline nanoclj_val_t oblist_add_item(nanoclj_t * sc, strview_t ns, strview_t name, nanoclj_val_t v) {
  uint_fast32_t h = murmur3_hash_qualified_string(ns.ptr, ns.size, name.ptr, name.size);
  uint_fast32_t location = h % _size_unchecked(sc->oblist);
  set_vector_elem(sc->oblist, location, mk_pointer(cons(sc, v, decode_pointer(vector_elem(sc->oblist, location)))));
  return v;
}

static inline nanoclj_val_t oblist_find_item(nanoclj_t * sc, uint16_t type, strview_t ns, strview_t name) {
  uint_fast32_t h = murmur3_hash_qualified_string(ns.ptr, ns.size, name.ptr, name.size);
  uint_fast32_t location = h % _size_unchecked(sc->oblist);
  nanoclj_val_t x = vector_elem(sc->oblist, location);
  if (!is_nil(x)) {
    for (; x.as_long != sc->EMPTY.as_long; x = cdr(x)) {
      nanoclj_val_t y = car(x);
      int ty = prim_type(y);
      if (type == ty) {
	symbol_t * s = decode_symbol(y);
	if (strview_eq(ns, s->ns) && strview_eq(name, s->name)) {
	  return y;
	}
      }
    }
  }
  return mk_nil();
}

static inline nanoclj_val_t mk_vector_2d(nanoclj_t * sc, double x, double y) {
  nanoclj_cell_t * vec = mk_vector(sc, 2);
  set_vector_elem(vec, 0, mk_real(x));
  set_vector_elem(vec, 1, mk_real(y));
  return mk_pointer(vec);
}

static inline nanoclj_val_t mk_vector_3d(nanoclj_t * sc, double x, double y, double z) {
  nanoclj_cell_t * vec = mk_vector(sc, 3);
  set_vector_elem(vec, 0, mk_real(x));
  set_vector_elem(vec, 1, mk_real(y));
  set_vector_elem(vec, 2, mk_real(z));
  return mk_pointer(vec);
}

static inline nanoclj_val_t mk_vector_4d(nanoclj_t * sc, double x, double y, double z, double w) {
  nanoclj_cell_t * vec = mk_vector(sc, 4);
  set_vector_elem(vec, 0, mk_real(x));
  set_vector_elem(vec, 1, mk_real(y));
  set_vector_elem(vec, 2, mk_real(z));
  set_vector_elem(vec, 3, mk_real(w));
  return mk_pointer(vec);
}

static inline nanoclj_val_t mk_vector_6d(nanoclj_t * sc, double x, double y, double z, double w, double a, double b) {
  nanoclj_cell_t * vec = mk_vector(sc, 6);
  set_vector_elem(vec, 0, mk_real(x));
  set_vector_elem(vec, 1, mk_real(y));
  set_vector_elem(vec, 2, mk_real(z));
  set_vector_elem(vec, 3, mk_real(w));
  set_vector_elem(vec, 4, mk_real(a));
  set_vector_elem(vec, 5, mk_real(b));
  return mk_pointer(vec);
}

/* Copies a vector so that the copy can be mutated */
static inline nanoclj_cell_t * copy_vector(nanoclj_t * sc, nanoclj_cell_t * vec) {
  size_t len = get_size(vec);
  if (_is_small(vec)) {
    nanoclj_cell_t * new_vec = _get_vector_object(sc, _type(vec), 0, len, NULL);
    nanoclj_val_t * data = _smalldata_unchecked(vec);
    nanoclj_val_t * new_data = _smalldata_unchecked(new_vec);
    memcpy(new_data, data, len * sizeof(nanoclj_val_t));
    return new_vec;
  } else {
    nanoclj_valarray_t * s = mk_valarray(sc, len, 2 * len);
    memcpy(s->data, _vec_store_unchecked(vec)->data + _offset_unchecked(vec), len * sizeof(nanoclj_val_t));
    return _get_vector_object(sc, _type(vec), 0, len, s);
  }
}

static inline void valarray_push(nanoclj_t * sc, nanoclj_valarray_t * vec, nanoclj_val_t val) {
  if (vec->size >= vec->reserved) {
    vec->reserved = 2 * (1 + vec->reserved);
    vec->data = sc->realloc(vec->data, vec->reserved * sizeof(nanoclj_val_t));
  }
  vec->data[vec->size++] = val;
}

static inline void valarray_pop(nanoclj_valarray_t * vec) {
  if (vec->size) vec->size--;
}

static inline size_t append_bytes(nanoclj_t * sc, nanoclj_bytearray_t * s, const uint8_t * ptr, size_t n) {
  if (s->size + n > s->reserved) {
    s->reserved = 2 * (s->size + n);
    s->data = sc->realloc(s->data, s->reserved);
  }
  memcpy(s->data + s->size, ptr, n);
  s->size += n;
  return n;
}
 
static inline size_t append_codepoint(nanoclj_t * sc, nanoclj_bytearray_t * s, int32_t c) {
  if (s->size + 4 >= s->reserved) {
    s->reserved = 2 * (s->size + 4);
    s->data = sc->realloc(s->data, s->reserved);
  }
  size_t n = utf8proc_encode_char(c, (utf8proc_uint8_t *)(s->data + s->size));
  s->size += n;
  return n;
}

static inline nanoclj_cell_t * conjoin(nanoclj_t * sc, nanoclj_cell_t * coll, nanoclj_val_t new_value) {
  if (!coll) coll = &(sc->_EMPTY);

  int t = _type(coll);
  if (_is_sequence(coll) || t == T_NIL || t == T_LIST || t == T_LAZYSEQ) {
    return get_cell(sc, T_LIST, 0, new_value, coll, NULL);
  } else if (is_vector_type(t)) {
    size_t old_size = get_size(coll);
    if (_is_small(coll)) {
      nanoclj_cell_t * new_coll = get_vector_object(sc, t, old_size + 1);
      nanoclj_val_t * data = _smalldata_unchecked(coll);
      nanoclj_val_t * new_data;
      if (_is_small(new_coll)) {
	new_data = _smalldata_unchecked(new_coll);
      } else {
	new_data = _vec_store_unchecked(new_coll)->data;
      }
      memcpy(new_data, data, old_size * sizeof(nanoclj_val_t));
      set_vector_elem(new_coll, old_size, new_value);
      return new_coll;
    } else {
      nanoclj_valarray_t * s = _vec_store_unchecked(coll);
      size_t old_offset = _offset_unchecked(coll);
    
      if (old_offset + old_size != s->size) {
	nanoclj_valarray_t * old_s = s;
	s = mk_valarray(sc, old_size, 2 * old_size + 1);
	memcpy(s->data, old_s->data + old_offset, old_size * sizeof(nanoclj_val_t));
      } else {
	s->refcnt++;
      }

      valarray_push(sc, s, new_value);
      return _get_vector_object(sc, t, old_offset, old_size + 1, s);
    }
  } else if (is_string_type(t)) {
    int32_t c = decode_integer(new_value);
    size_t size = get_size(coll);
    if (_is_small(coll)) {
      size_t input_len = utf8proc_encode_char(c, (utf8proc_uint8_t *)sc->strbuff);
      nanoclj_cell_t * new_coll = get_string_object(sc, t, NULL, size + input_len, 0);
      const uint8_t * data = _smallstrvalue_unchecked(coll);
      uint8_t * new_data;
      if (_is_small(new_coll)) {
	new_data = _smallstrvalue_unchecked(new_coll);
      } else {
	new_data = _str_store_unchecked(new_coll)->data;
      }
      memcpy(new_data, data, size * sizeof(uint8_t));
      memcpy(new_data + size, sc->strbuff, input_len * sizeof(uint8_t));
      return new_coll;
    } else {
      nanoclj_bytearray_t * s = _str_store_unchecked(coll);
      size_t offset = _offset_unchecked(coll);
      if (offset + size != s->size) {
	nanoclj_bytearray_t * old_s = s;
	s = mk_bytearray(sc, size, size);
	memcpy(s->data, old_s->data + offset, size);
	offset = 0;
      } else {
	s->refcnt++;
      }
      size += append_codepoint(sc, s, c);
      return _get_string_object(sc, t, offset, size, s);
    }
  } else {
    return NULL;
  }
}

static inline nanoclj_cell_t * mk_sorted_set(nanoclj_t * sc, int len) {
  return get_vector_object(sc, T_SORTED_SET, len);
}

static inline nanoclj_cell_t * mk_arraymap(nanoclj_t * sc, size_t len) {
  return get_vector_object(sc, T_ARRAYMAP, len);
}

/* get new symbol */
static inline nanoclj_val_t def_symbol_from_sv(nanoclj_t * sc, uint16_t t, strview_t ns, strview_t name) {
  /* first check oblist */
  nanoclj_val_t x = oblist_find_item(sc, t, ns, name);
  if (is_nil(x)) {
    x = oblist_add_item(sc, ns, name, mk_symbol(sc, t, ns, name, 0));
    if (ns.size || t == T_KEYWORD) {
      symbol_t * s = decode_symbol(x);
      s->ns_sym = def_symbol_from_sv(sc, T_SYMBOL, mk_strview(0), ns);
      s->name_sym = def_symbol_from_sv(sc, T_SYMBOL, mk_strview(0), name);
    }
  }
  return x;
}

static inline nanoclj_val_t def_keyword(nanoclj_t * sc, const char *name) {
  return def_symbol_from_sv(sc, T_KEYWORD, mk_strview(0), mk_strview(name));
}

static inline nanoclj_val_t def_symbol(nanoclj_t * sc, const char *name) {
  return def_symbol_from_sv(sc, T_SYMBOL, mk_strview(0), mk_strview(name));
}

static inline nanoclj_val_t intern_with_meta(nanoclj_t * sc, nanoclj_cell_t * envir, nanoclj_val_t symbol, nanoclj_val_t value, nanoclj_cell_t * meta) {
  nanoclj_cell_t * var = find_slot_in_env(sc, envir, symbol, false);
  if (var) set_slot_in_env(sc, var, value, meta);
  else var = new_slot_spec_in_env(sc, envir, symbol, value, meta);
  return mk_pointer(var);
}

static inline nanoclj_val_t intern(nanoclj_t * sc, nanoclj_cell_t * envir, nanoclj_val_t symbol, nanoclj_val_t value) {
  return intern_with_meta(sc, envir, symbol, value, NULL);  
}

static inline nanoclj_val_t intern_symbol(nanoclj_t * sc, nanoclj_cell_t * envir, nanoclj_val_t symbol) {
  nanoclj_cell_t * var = find_slot_in_env(sc, envir, symbol, false);
  if (!var) var = new_slot_spec_in_env(sc, envir, symbol, mk_nil(), NULL);
  return mk_pointer(var);
}

static inline nanoclj_val_t intern_foreign_func(nanoclj_t * sc, nanoclj_cell_t * envir, const char * name, foreign_func fptr, int min_arity, int max_arity) {
  nanoclj_val_t ns_sym = mk_nil();
  get_elem(sc, _cons_metadata(envir), sc->NAME, &ns_sym);

  nanoclj_val_t sym = def_symbol(sc, name);

  nanoclj_cell_t * md = mk_arraymap(sc, 2);
  set_vector_elem(md, 0, mk_mapentry(sc, sc->NS, ns_sym));
  set_vector_elem(md, 0, mk_mapentry(sc, sc->NAME, sym));

  nanoclj_val_t fn = mk_foreign_func(sc, fptr, min_arity, max_arity);
  set_metadata(decode_pointer(fn), md);
  intern_with_meta(sc, envir, sym, fn, md);
  return fn;
}

static inline nanoclj_cell_t * mk_class_with_meta(nanoclj_t * sc, nanoclj_val_t sym, int type_id, nanoclj_cell_t * parent_type, nanoclj_cell_t * md) {
  nanoclj_cell_t * vec = mk_vector(sc, 727);
  fill_vector(vec, mk_nil());
  nanoclj_cell_t * t = get_cell(sc, type_id, T_TYPE, mk_pointer(vec), parent_type, md);
  
  while (sc->types->size <= (size_t)type_id) {
    valarray_push(sc, sc->types, mk_nil());
  }
  sc->types->data[type_id] = mk_pointer(t);
  intern_with_meta(sc, sc->global_env, sym, mk_pointer(t), md);

  return t;
}

static inline nanoclj_cell_t * mk_class_with_fn(nanoclj_t * sc, const char * name, int type_id, nanoclj_cell_t * parent_type, const char * fn) {
  nanoclj_val_t sym = def_symbol_from_sv(sc, T_SYMBOL, mk_strview(0), mk_strview(name));
  nanoclj_cell_t * md = mk_arraymap(sc, 2);
  set_vector_elem(md, 0, mk_mapentry(sc, sc->NAME, sym));
  set_vector_elem(md, 1, mk_mapentry(sc, sc->FILE, mk_string(sc, fn)));
  nanoclj_cell_t * t = mk_class_with_meta(sc, sym, type_id, parent_type, md);

  char * p = strrchr(name, '.');
  if (p && p[1]) {
    intern_with_meta(sc, sc->global_env, def_symbol(sc, p + 1), mk_pointer(t), md);
  }
  return t;
}

static inline nanoclj_cell_t * mk_class(nanoclj_t * sc, const char * name, int type_id, nanoclj_cell_t * parent_type) {
  return mk_class_with_fn(sc, name, type_id, parent_type, __FILE__);
}

/* get new symbol or keyword. % is aliased to %1 */
static inline nanoclj_val_t def_symbol_or_keyword(nanoclj_t * sc, const char *name) {
  if (name[0] == '%' && name[1] == 0) return def_symbol(sc, "%1");
  else {
    uint16_t t = T_SYMBOL;
    if (name[0] == ':') {
      t = T_KEYWORD;
      name++;
    }
    strview_t ns_sv = mk_strview(0), name_sv;
    if (t == T_KEYWORD && name[0] == ':') { /* use the current namespace (::keyword) */
      name++;
      nanoclj_val_t ns_name;
      if (get_elem(sc, _cons_metadata(sc->global_env), sc->NAME, &ns_name)) {
	ns_sv = to_strview(ns_name);
      }
      name_sv = mk_strview(name);
    } else {
      const char * p = strchr(name, '/');
      if (p && p > name) {
	ns_sv = (strview_t){ name, p - name };
	name_sv = mk_strview(p + 1);
      } else {
	name_sv = mk_strview(name);
      }
    }
    return def_symbol_from_sv(sc, t, ns_sv, name_sv);
  }
}

static inline nanoclj_cell_t * def_namespace_with_sym(nanoclj_t *sc, nanoclj_val_t sym, nanoclj_cell_t * md) {
  nanoclj_cell_t * vec = mk_vector(sc, 727);
  fill_vector(vec, mk_nil());
  nanoclj_cell_t * ns = get_cell(sc, T_ENVIRONMENT, 0, mk_pointer(vec), sc->root_env, md);
  if (sc->global_env) {
    intern_with_meta(sc, sc->global_env, sym, mk_pointer(ns), md);
  }
  return ns;
}

static inline nanoclj_cell_t * def_namespace(nanoclj_t * sc, const char *name, const char *file) {
  nanoclj_val_t sym = def_symbol(sc, name);
  nanoclj_cell_t * md = mk_arraymap(sc, 2);
  set_vector_elem(md, 0, mk_mapentry(sc, sc->NAME, sym));
  set_vector_elem(md, 1, mk_mapentry(sc, sc->FILE, mk_string(sc, file)));
  return def_namespace_with_sym(sc, sym, md);
}

static inline nanoclj_cell_t * mk_meta_from_reader(nanoclj_t * sc, nanoclj_val_t p0) {
  nanoclj_cell_t * p = decode_pointer(p0);
  if (_port_type_unchecked(p) == port_file) {
    nanoclj_port_rep_t * pr = _rep_unchecked(p);
    nanoclj_cell_t * md = mk_arraymap(sc, 3);
    set_vector_elem(md, 0, mk_mapentry(sc, sc->LINE, mk_int(_line_unchecked(p) + 1)));
    set_vector_elem(md, 1, mk_mapentry(sc, sc->COLUMN, mk_int(_column_unchecked(p) + 1)));
    set_vector_elem(md, 2, mk_mapentry(sc, sc->FILE, mk_string(sc, pr->stdio.filename)));
    return md;
  } else {
    return mk_arraymap(sc, 0);
  }
}

static inline nanoclj_val_t gensym(nanoclj_t * sc, strview_t prefix) {
  nanoclj_val_t x;
  char name[256];
  nanoclj_shared_context_t * ctx = sc->context;

  nanoclj_mutex_lock( &(ctx->mutex) );

  nanoclj_val_t r = mk_nil();
  for (; ctx->gensym_cnt < LONG_MAX; ctx->gensym_cnt++) {
    size_t len = snprintf(name, 256, "%.*s%ld", (int)prefix.size, prefix.ptr, ctx->gensym_cnt);

    /* first check oblist */
    strview_t ns_sv = mk_strview(0);
    strview_t name_sv = { name, len };
    x = oblist_find_item(sc, T_SYMBOL, ns_sv, name_sv);
    if (is_nil(x)) {
      r = oblist_add_item(sc, ns_sv, name_sv, mk_symbol(sc, T_SYMBOL, ns_sv, name_sv, 0));
      break;
    }
  }

  nanoclj_mutex_unlock( &(ctx->mutex) );
  return r;
}

static inline int gentypeid(nanoclj_t * sc) {
  nanoclj_shared_context_t * ctx = sc->context;
  nanoclj_mutex_lock( &(ctx->mutex) );
  int id = sc->context->gentypeid_cnt++;
  nanoclj_mutex_unlock( &(ctx->mutex) );
  return id;
}

/* make symbol or number literal from string */
static inline nanoclj_val_t mk_primitive(nanoclj_t * sc, const char *q) {
  bool has_dec_point = false, has_fp_exp = false;
  int sign = 1, radix = 0;
  const char * p = q, * div = 0; 

  char c = *p++;
  if ((c == '+') || (c == '-')) {
    if (c == '-') sign = -1;
    c = *p++;
    if (c == '.') {
      has_dec_point = true;
      c = *p++;
    }
  } else if (c == '.') {
    has_dec_point = true;
    c = *p++;
  }
  
  if (has_dec_point) {
    if (digit(c, 10) == -1) {
      return def_symbol_or_keyword(sc, q);
    }
  } else if (c == '0') {
    if (*p == 'x') {
      radix = 16;	
      p++;
      q = p;     
    } else if (digit(*p, 8) >= 0) {
      radix = 8;
      q = p;
    }
  } else {
    int b1 = digit(c, 10);
    if (b1 == -1) {
      return def_symbol_or_keyword(sc, q);    
    } else {
      if (*p == 'r') {
	radix = b1;
	p++;
	q = p;
      } else {
	int b2 = digit(*p, 10);
	if (b2 != -1 && p[1] == 'r') {
	  radix = b1 * 10 + b2;
	  p += 2;
	  q = p;
	}	  
      }
    }
  }

  bool radix_set = true;
  if (!radix) {
    radix_set = false;
    radix = 10;
    sign = 1;
  } else if (radix < 2 || radix > 36) {
    return mk_nil();
  }
  
  for (; (c = *p) != 0; ++p) {
    if (digit(c, radix) == -1) {
      if (!radix_set) {
	if (c == '/' && !div && !has_dec_point) {
	  div = p;
	  break; /* check the rest using strtoll */
	} else if (c == 'e' || c == 'E' || (c == '.' && !has_dec_point)) {
	  has_dec_point = true;
	  break; /* check the rest using strtod */
	}
      }
      return mk_nil();
    }
  }
  
  if (div) {
    char * end1, * end2;
    long long num = strtoll(q, &end1, 10), den = strtoll(div + 1, &end2, 10);
    if (end1 != div || end2 != q + strlen(q)) {
      return mk_nil();
    }
    if (den != 0) {
      num = normalize(num, den, &den);
    } else if (num != 0) {
      num = num < 0 ? -1 : +1;
    }
    if (den == 1) {
      return mk_integer(sc, num);
    } else {
      return mk_ratio_long(sc, num, den);
    }
  } else {
    char * end;
    const char * actual_end = q + strlen(q);   
    if (has_dec_point) {
      double d = strtod(q, &end);
      if (end == actual_end) return mk_real(d);
    } else {
      long long i = strtoll(q, &end, radix);
      if (end == actual_end) return mk_integer(sc, sign * i);
    }
    return mk_nil();   
  }
}

static inline int32_t parse_char_literal(nanoclj_t * sc, const char *name) {
  if (strcmp(name, "space") == 0) {
    return ' ';
  } else if (strcmp(name, "newline") == 0) {
    return '\n';
  } else if (strcmp(name, "return") == 0) {
    return '\r';
  } else if (strcmp(name, "tab") == 0) {
    return '\t';
  } else if (strcmp(name, "formfeed") == 0) {
    return '\f';
  } else if (strcmp(name, "backspace") == 0) {
    return '\b';
  } else if (name[0] == 'u') {
    char * end;
    int32_t c = strtol(name + 1, &end, 16);
    if (!*end) return c;    
  } else if (name[0] == 'o') {
    char * end;
    int32_t c = strtol(name + 1, &end, 8);
    if (!*end) return c;
  } else if (name[0] != 0 && name[0] != ' ' && name[0] != '\n' && *(utf8_next(name)) == 0) {
    return decode_utf8(name);
  }
  return -1;
}

static inline nanoclj_val_t mk_sharp_const(nanoclj_t * sc, char *name) {
  if (strcmp(name, "Inf") == 0) {
    return mk_real(INFINITY);
  } else if (strcmp(name, "-Inf") == 0) {
    return mk_real(-INFINITY);
  } else if (strcmp(name, "NaN") == 0) {
    return mk_real(NAN);
  } else if (strcmp(name, "Eof") == 0) {
    return mk_codepoint(EOF);
  } else {
    return mk_nil();
  }
}

/* ========== Routines for Reading ========== */

static inline nanoclj_cell_t * get_port_object(nanoclj_t * sc, uint16_t type, nanoclj_port_type_t port_type) {
  nanoclj_cell_t * x = get_cell_x(sc, type, T_GC_ATOM, NULL, NULL, NULL);
  if (x) {
    nanoclj_port_rep_t * pr = sc->malloc(sizeof(nanoclj_port_rep_t));
    if (pr) {
      x->_object._port.rep = pr;
      _port_type_unchecked(x) = port_type;
      _port_flags_unchecked(x) = 0;
      _nesting_unchecked(x) = 0;
      _line_unchecked(x) = -1;
      _column_unchecked(x) = -1;

#if RETAIN_ALLOCS
      retain(sc, x);
#endif
      return x;
    }
  }
  sc->pending_exception = sc->OutOfMemoryError;
  return NULL;
}

static inline int http_open_thread(nanoclj_t * sc, strview_t sv) {
  int pipefd[2];
  if (pipe(pipefd) == -1) {
    return 0;
  }

  http_load_t * d = sc->malloc(sizeof(http_load_t));
  d->url = alloc_c_str(sc, sv);
  d->useragent = NULL;
  d->http_keepalive = true;
  d->http_max_connections = 0;
  d->http_max_redirects = 0;
  d->fd = pipefd[1];
  d->malloc = sc->malloc;
  d->free = sc->free;
  d->realloc = sc->realloc;

  nanoclj_cell_t * prop = sc->context->properties;
  
  nanoclj_val_t v;
  if (get_elem(sc, prop, mk_string(sc, "http.agent"), &v)) {
    d->useragent = alloc_c_str(sc, to_strview(v));
  }
  if (get_elem(sc, prop, mk_string(sc, "http.keepalive"), &v)) {
    d->http_keepalive = to_int(v);
  }
  if (get_elem(sc, prop, mk_string(sc, "http.maxConnections"), &v)) {
    d->http_max_connections = to_int(v);
  }
  if (get_elem(sc, prop, mk_string(sc, "http.maxRedirects"), &v)) {
    d->http_max_redirects = to_int(v);
  }

  nanoclj_start_thread(http_load, d);
  
  return pipefd[0];
}

static inline nanoclj_val_t port_rep_from_file(nanoclj_t * sc, uint16_t type, FILE * f, char * filename) {
  nanoclj_cell_t * p = get_port_object(sc, type, port_file);
  if (!p) return mk_nil();

  _line_unchecked(p) = _column_unchecked(p) = 0;

  nanoclj_port_rep_t * pr = _rep_unchecked(p);
  pr->stdio.file = f;
  pr->stdio.filename = filename;
  pr->stdio.num_states = 0;
  pr->stdio.mode = nanoclj_mode_unknown;
  pr->stdio.fg = sc->fg_color;
  pr->stdio.bg = sc->bg_color;
  return mk_pointer(p);
}

static inline nanoclj_val_t port_from_stream(nanoclj_t * sc, uint16_t type, nanoclj_cell_t * stream) {
  nanoclj_cell_t * p = get_port_object(sc, type, port_z);
  if (!p) return mk_nil();
  nanoclj_port_rep_t * pr = _rep_unchecked(p);
  z_stream * strm = &(pr->z.strm);
  /* allocate inflate state */
  strm->zalloc = Z_NULL;
  strm->zfree = Z_NULL;
  strm->opaque = Z_NULL;
  strm->avail_in = 0;
  strm->next_in = Z_NULL;
  int ret;
  if (type == T_GZIP_INPUT_STREAM) {
    ret = inflateInit(strm);
  } else if (type == T_GZIP_OUTPUT_STREAM) {
    ret = deflateInit(strm, 9);
  }
  if (ret != Z_OK) {
    return mk_nil();
  }
  return mk_pointer(p);
}

static inline nanoclj_val_t port_from_filename(nanoclj_t * sc, uint16_t type, strview_t sv) {
  const char * mode;
  switch (type) {
  case T_WRITER: mode = "w"; break;
  case T_OUTPUT_STREAM: mode = "wb"; break;
  case T_READER: mode = "r"; break;
  case T_INPUT_STREAM: mode = "rb"; break;
  default: return mk_nil();
  }

  FILE * f = NULL;
  char * filename = alloc_c_str(sc, sv);
  if (strcmp(filename, "-") == 0) {
    f = stdin;
  } else if (strncmp(filename, "http://", 7) == 0 ||
	     strncmp(filename, "https://", 8) == 0) {
    int fd = http_open_thread(sc, sv);
    f = fdopen(fd, mode);
  } else if (strncmp(filename, "file://", 7) == 0) {
    f = fopen(filename + 7, mode);
  } else if (filename[0] == '|') {
    f = popen(filename + 1, mode);
  } else {
    f = fopen(filename, mode);
  }
  if (!f) {
    sc->free(filename);
    return mk_nil();
  }
  
  return port_rep_from_file(sc, type, f, filename);
}

static inline nanoclj_val_t port_from_string(nanoclj_t * sc, uint16_t type, strview_t sv) {
  nanoclj_cell_t * p = get_port_object(sc, type, port_string);
  if (!p) return mk_nil();
  
  nanoclj_bytearray_t * s = mk_bytearray(sc, sv.size, 0);
  memcpy(s->data, sv.ptr, sv.size);
  
  nanoclj_port_rep_t * pr = _rep_unchecked(p);
  pr->string.data = s;
  pr->string.read_pos = 0;
  
  return mk_pointer(p);
}

static inline nanoclj_cell_t * mk_reader(nanoclj_t * sc, uint16_t t, nanoclj_cell_t * args) {
  nanoclj_val_t f = first(sc, args);
  if (is_cell(f)) {
    nanoclj_cell_t * c = decode_pointer(f);
    if (!c) {
      nanoclj_throw(sc, sc->NullPointerException);
    } else {
      switch (_type(c)) {
      case T_STRING:
      case T_FILE:
	return decode_pointer(port_from_filename(sc, t, _to_strview(c)));
      case T_CHAR_ARRAY:
	return decode_pointer(port_from_string(sc, t, _to_strview(c)));
      case T_READER:
      case T_INPUT_STREAM:
	if (t == T_GZIP_INPUT_STREAM || t == T_GZIP_OUTPUT_STREAM) {
	  return decode_pointer(port_from_stream(sc, t, c));
	} else {
	  return c;
	}
      }
    }
  }
  return NULL;
}

static inline nanoclj_val_t slurp(nanoclj_t * sc, uint16_t t, nanoclj_cell_t * args) {
  nanoclj_cell_t * rdr = mk_reader(sc, t, args);
  nanoclj_bytearray_t * array = mk_bytearray(sc, 0, 0);
  size_t size = 0;
  while ( 1 ) {
    if (t == T_READER) {
      int32_t c = inchar(rdr);
      if (c < 0) break;
      size += append_codepoint(sc, array, c);
    } else {
      int32_t c = inchar_raw(rdr);
      if (c < 0) break;
      uint8_t b = c;
      size += append_bytes(sc, array, &b, 1);
    }
  }
  nanoclj_val_t r = mk_pointer(_get_string_object(sc, T_STRING, 0, size, array));
  port_close(sc, rdr);
  return r;
}

static inline bool file_push(nanoclj_t * sc, nanoclj_val_t f) {
  nanoclj_val_t p = port_from_filename(sc, T_READER, to_strview(f));
  if (!is_nil(p)) {
    valarray_push(sc, sc->load_stack, p);
    return !sc->pending_exception;
  } else {
    return false;
  }
}

static inline void file_pop(nanoclj_t * sc) {
  if (sc->load_stack->size != 0) {
    port_close(sc, decode_pointer(valarray_peek(sc->load_stack)));
    valarray_pop(sc->load_stack);
  }
}

static inline nanoclj_val_t mk_writer_from_callback(nanoclj_t * sc,
						    void (*text) (const char *, size_t, void *),
						    void (*color) (double, double, double, void *),
						    void (*restore) (void *),
						    void (*image) (imageview_t, void*)) {
  nanoclj_cell_t * p = get_port_object(sc, T_WRITER, port_callback);
  nanoclj_port_rep_t * pr = _rep_unchecked(p);
  pr->callback.text = text;
  pr->callback.color = color;
  pr->callback.restore = restore;
  pr->callback.image = image;
  return mk_pointer(p);
}

static inline void putchars(nanoclj_t * sc, const char *s, size_t len, nanoclj_cell_t * out) {
  assert(s);
  nanoclj_port_rep_t * pr = _rep_unchecked(out);
  switch (_port_type_unchecked(out)) {
  case port_file:{
    fwrite(s, 1, len, pr->stdio.file);
    const char * end = s + len;
    while (s < end) {
      update_cursor(decode_utf8(s), out, sc->window_columns);
      s = utf8_next(s);
    }
  }
    break;
  case port_callback:
    pr->callback.text(s, len, sc->ext_data);
    break;
  case port_string:
    append_bytes(sc, pr->string.data, (const uint8_t *)s, len);
    break;
#if NANOCLJ_HAS_CANVAS
  case port_canvas:  
    canvas_show_text(sc, pr->canvas.impl, (strview_t){ s, len });
    break;
#endif
  }
}

static inline void putstr(nanoclj_t * sc, const char *s, nanoclj_cell_t * out) {
  putchars(sc, s, strlen(s), out);
}

static inline void set_color(nanoclj_t * sc, nanoclj_color_t color, nanoclj_val_t out) {
  nanoclj_port_rep_t * pr = rep_unchecked(out);
  switch (port_type_unchecked(out)) {
  case port_file:
    pr->stdio.fg = color;
#ifndef WIN32    
    set_term_fg_color(pr->stdio.file, color, sc->term_colors);
#endif
    break;
  case port_callback:
    if (pr->callback.color) {
      pr->callback.color(color.red / 255.0,
			 color.green / 255.0,
			 color.blue / 255.0,
			 sc->ext_data
			 );
    }
    break;
#if NANOCLJ_HAS_CANVAS
  case port_canvas:
    canvas_set_color(pr->canvas.impl, color);
    break;
#endif    
  }
}

static inline void set_bg_color(nanoclj_t * sc, nanoclj_color_t color, nanoclj_val_t out) {
  nanoclj_port_rep_t * pr = rep_unchecked(out);
  switch (port_type_unchecked(out)) {
  case port_file:
    pr->stdio.bg = color;
#ifndef WIN32    
    set_term_bg_color(pr->stdio.file, color, sc->term_colors);
#endif
    break;
  }
}

static inline void set_linear_gradient(nanoclj_t * sc, nanoclj_cell_t * p0, nanoclj_cell_t * p1, nanoclj_cell_t * colormap, nanoclj_val_t out) {
  nanoclj_port_rep_t * pr = rep_unchecked(out);
  switch (port_type_unchecked(out)) {
  case port_file:
    break;
#if NANOCLJ_HAS_CANVAS
  case port_canvas:
    canvas_set_linear_gradient(pr->canvas.impl, p0, p1, colormap);
    break;
#endif    
  }  
}

/* check c is in chars */
static inline bool is_one_of(const char *s, int c) {
  if (c < 0) return true; /* always accept EOF */
  else if (c < 256) return strchr(s, c) != NULL;
  return false;
}

/* read characters up to delimiter, but cater to character constants */
static inline char *readstr_upto(nanoclj_t * sc, char *delim, nanoclj_cell_t * inport, bool is_escaped) {
  nanoclj_bytearray_t * rdbuff = sc->rdbuff;
  rdbuff->size = 0;
  
  while (1) {
    int c = inchar(inport);
    if (c == EOF) break; 

    append_codepoint(sc, rdbuff, c);
    
    if (is_escaped) {
      is_escaped = false;
    } else if (is_one_of(delim, c)) {
      backchar(c, inport);
      rdbuff->size--;
      break;
    }
  }

  append_codepoint(sc, rdbuff, 0);
  rdbuff->size--;
  return (char *)rdbuff->data;
}

static inline const char * escape_char(int32_t c, char * buffer, bool in_string) {
  if (in_string) {
    switch (c) {
    case 0:	return "\\u0000";
    case ' ':   return " ";
    case '"':	return "\\\"";
    case '\n':	return "\\n";
    case '\t':	return "\\t";
    case '\r':	return "\\r";
    case '\b':	return "\\b";
    case '\\':	return "\\";
    }
  } else {
    switch (c) {
    case -1:	return "##Eof";
    case ' ':	return "\\space";
    case '\n':	return "\\newline";
    case '\r':	return "\\return";
    case '\t':	return "\\tab";
    case '\f':	return "\\formfeed";
    case '\b':	return "\\backspace";
    }
  }
  utf8proc_category_t cat = utf8proc_category(c);
  if (cat == UTF8PROC_CATEGORY_MN ||
      cat == UTF8PROC_CATEGORY_MC ||
      cat == UTF8PROC_CATEGORY_ME ||
      cat == UTF8PROC_CATEGORY_ZS ||
      cat == UTF8PROC_CATEGORY_ZL ||
      cat == UTF8PROC_CATEGORY_ZP ||
      cat == UTF8PROC_CATEGORY_CC ||
      cat == UTF8PROC_CATEGORY_CF ||
      cat == UTF8PROC_CATEGORY_CS ||
      cat == UTF8PROC_CATEGORY_CO) {
    sprintf(buffer, "\\u%04x", c);
  } else {
    utf8proc_uint8_t * p = (utf8proc_uint8_t *)buffer;
    if (!in_string) {
      *p++ = '\\';
    }
    p += utf8proc_encode_char(c, p);
    *p = 0;
  }
  return buffer;
}

/* read string expression "xxx...xxx" */
static inline nanoclj_val_t readstrexp(nanoclj_t * sc, nanoclj_cell_t * inport, bool regex_mode) {
  nanoclj_bytearray_t * rdbuff = sc->rdbuff;
  rdbuff->size = 0;

  int c1 = 0;
  enum { st_ok, st_bsl, st_u1, st_u2, st_u3, st_u4, st_oct2, st_oct3 } state = st_ok;
  bool fin = false;

  do {
    int c = inchar(inport);
    if (c == EOF) { 
      return mk_nil();
    }
    int radix = 16;
    switch (state) {
    case st_ok:
      switch (c) {
      case '\\':
        state = st_bsl;
        break;
      case '"':
	fin = true;
	break;
      default:
        append_codepoint(sc, rdbuff, c);
        break;
      }
      break;
    case st_bsl:
      if (!regex_mode) {
	switch (c) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	  state = st_oct2;
	  c1 = digit(c, 8);
	  break;
	case 'u':
	  state = st_u1;
	  c1 = 0;
	  break;
	case 'n':
	  append_codepoint(sc, rdbuff, '\n');
	  state = st_ok;
	  break;
	case 't':
	  append_codepoint(sc, rdbuff, '\t');
	  state = st_ok;
	  break;
	case 'r':
	  append_codepoint(sc, rdbuff, '\r');
	  state = st_ok;
	  break;
	case '"':
	case '\\':
	  append_codepoint(sc, rdbuff, c);
	  state = st_ok;
	  break;
	default:
	  nanoclj_throw(sc, mk_runtime_exception(sc, mk_string_fmt(sc, "Unsupported escape character: \\%s", escape_char(c, sc->strbuff, false))));
	  return mk_nil();
	}
      } else {
	if (c != '"' && c != '\\') {
	  append_codepoint(sc, rdbuff, '\\');
	}
	append_codepoint(sc, rdbuff, c);
	state = st_ok;
      }
      break;
    case st_oct2:
    case st_oct3:
      radix = 8;
    case st_u1:
    case st_u2:
    case st_u3:
    case st_u4:
      if (c == '"') {
	fin = true;
      } else {
	int d = digit(c, radix);
	if (d == -1) {	 
	  nanoclj_throw(sc, mk_runtime_exception(sc, mk_string_fmt(sc, "Invalid digit: %s", escape_char(c, sc->strbuff, false))));
	  return mk_nil();
	} else {
	  c1 = (c1 * radix) + d;
	}
      }
      if (fin || state == st_u4 || state == st_oct3) {
	append_codepoint(sc, rdbuff, c1);
	state = st_ok;
      } else {
	state++;
      }
      break;      
    }
  } while (!fin);
  
  return mk_pointer(get_string_object(sc, T_STRING, (char *)rdbuff->data, rdbuff->size, 0));
}

/* skip white space characters, returns the first non-whitespace character */
static inline int32_t skipspace(nanoclj_t * sc, nanoclj_cell_t * inport) {
  while ( 1 ) {
    int32_t c = inchar(inport);
    int cat = utf8proc_category(c);
    if (cat != UTF8PROC_CATEGORY_ZS &&
	cat != UTF8PROC_CATEGORY_ZL &&
	cat != UTF8PROC_CATEGORY_ZP &&
	cat != UTF8PROC_CATEGORY_CC) {
      return c;
    }
  }
}

/* get token */
static inline int token(nanoclj_t * sc, nanoclj_cell_t * inport) {
  int c;
  switch (c = skipspace(sc, inport)) {
  case EOF: return TOK_EOF;
  case ')': return TOK_RPAREN;
  case ']': return TOK_RSQUARE;
  case '}': return TOK_RCURLY;
  case '"': return TOK_DQUOTE;
  case '[': return TOK_VEC;
  case '{': return TOK_MAP;
  case '\'': return TOK_QUOTE;
  case '@': return TOK_DEREF;
  case BACKQUOTE: return TOK_BQUOTE;
  case '\\': return TOK_CHAR_CONST;
  case '^': return TOK_META;
  
  case '(':
    c = skipspace(sc, inport);
    if (c == '.') {
      int c2 = inchar(inport);
      backchar(c2, inport);
      if (is_one_of(" \n\t", c2)) {
	backchar(c, inport);
	return TOK_LPAREN;
      } else {
	return TOK_DOT;
      }
    } else {
      backchar(c, inport);
      return TOK_LPAREN;
    } 

  case '$':
    c = inchar(inport);
    if (is_one_of(" \n\t", c)) {
      return TOK_OLD_DOT;
    } else {
      backchar(c, inport);
      backchar('$', inport);
      return TOK_PRIMITIVE;
    }

  case ';':
    while ((c = inchar(inport)) != '\n' && c != EOF) { }
    if (c == EOF) {
      return TOK_EOF;
    } else {
      return token(sc, inport);
    }

  case ',':
    if ((c = inchar(inport)) == '@') {
      return TOK_ATMARK;
    } else if (is_one_of("\n\r ", c)) {
      backchar(c, inport);
      return (token(sc, inport));
    } else {
      backchar(c, inport);
      return TOK_COMMA;
    }
    
  case '#':
    c = inchar(inport);
    if (c == '(') {
      return TOK_FN;
    } else if (c == '{') {
      return TOK_SET;
    } else if (c == '"') {
      return TOK_REGEX;
    } else if (c == '_') {
      return TOK_IGNORE;
    } else if (c == '\'') {
      return TOK_VAR;
    } else if (c == '#') {
      return TOK_SHARP_CONST;
    } else if (c == '!') {
      /* This is a shebang line, so skip it */
      while ((c = inchar(inport)) != '\n' && c != EOF) { }

      if (c == EOF) {
        return TOK_EOF;
      } else {
        return token(sc, inport);
      }
    } else {
      backchar(c, inport);
      return TOK_TAG;
    }
  default:
    backchar(c, inport);
    return TOK_PRIMITIVE;
  }
}

/* ========== Routines for Printing ========== */

static inline void print_slashstring(nanoclj_t * sc, strview_t sv, nanoclj_cell_t * out) {
  const char * p = sv.ptr;
  putstr(sc, "\"", out);
  const char * end = p + sv.size;
  while (p < end) {
    int c = decode_utf8(p);
    p = utf8_next(p);    
    putstr(sc, escape_char(c, sc->strbuff, true), out);    
  }
  putstr(sc, "\"", out);
}

static void print_tensor(nanoclj_t * sc, void * tensor, nanoclj_cell_t * out) {
  switch (tensor_get_n_dims(tensor)) {
  case 1:{
    putchars(sc, "[", 1, out);
    int w = tensor_get_dim(tensor, 0);
    for (int x = 0; x < w; x++) {
      int l = sprintf(sc->strbuff, " %.15g", tensor_get1f(tensor, x));
      putchars(sc, sc->strbuff, l, out);
    }
    putchars(sc, " ]", 2, out);
  }
    break;
  case 2:{
    int w = tensor_get_dim(tensor, 0), h = tensor_get_dim(tensor, 1);
    for (int y = 0; y < h; y++) {
      putchars(sc, y == 0 ? "[" : " ", 1, out);
      for (int x = 0; x < w; x++) {
	int l = sprintf(sc->strbuff, " %.15g", tensor_get2f(tensor, x, y));
	putchars(sc, sc->strbuff, l, out);
      }
      if (y + 1 < h) {
	putchars(sc, " ;\n", 3, out);
      } else {
	putchars(sc, "\n]", 2, out);
      }
    }
  }
    break;
  }
}

static inline bool print_imageview(nanoclj_t * sc, imageview_t iv, nanoclj_cell_t * out) {
  if (!is_writable(out)) {
    return false;
  }

  nanoclj_port_rep_t * pr = _rep_unchecked(out);
  switch (_port_type_unchecked(out)) {
  case port_callback:
    if (pr->callback.image) {
      pr->callback.image(iv, sc->ext_data);
      return true;
    }
  case port_file:
    if (pr->stdio.file == stdout) {
      switch (sc->term_graphics) {
      case nanoclj_no_gfx: break;
#if NANOCLJ_SIXEL
      case nanoclj_sixel:
	if (print_image_sixel(iv, pr->stdio.fg, pr->stdio.bg)) {
	  return true;
	}
	break;
#endif
      case nanoclj_kitty:
	if (print_image_kitty(iv, pr->stdio.fg, pr->stdio.bg)) {
	  return true;
	}
	break;
      }
    }
  }

  return false;
}

static inline nanoclj_cell_t * get_type_object(nanoclj_t * sc, nanoclj_val_t v) {
  int type_id = type(v);
  if (type_id >= 0 && (size_t)type_id < sc->types->size) {
    nanoclj_cell_t * t = decode_pointer(sc->types->data[type_id]);
    if (t) return t;
  }
  return NULL;
}

static inline bool isa(nanoclj_t * sc, nanoclj_cell_t * t0, nanoclj_val_t v) {
  if (is_nil(v)) {
    return false;
  } 
  nanoclj_cell_t * t1 = get_type_object(sc, v);
  while (t1) {
    if (t1 == t0) {
      return true;
    }
    t1 = next(sc, t1);
  }
  return false;
}

/* Uses internal buffer unless string pointer is already available */
static inline void print_primitive(nanoclj_t * sc, nanoclj_val_t l, bool print_flag, nanoclj_cell_t * out) {
  strview_t sv = { 0, 0 };

  switch (prim_type(l)) {
  case T_LONG:
    sv = (strview_t){
      sc->strbuff,
      sprintf(sc->strbuff, "%d", decode_integer(l))
    };
    break;
  case T_REAL:
    if (isnan(l.as_double)) {
      sv = (strview_t){ "##NaN", 5 };
    } else if (isinf(l.as_double)) {
      sv = l.as_double > 0 ? (strview_t){ "##Inf", 5 } : (strview_t){ "##-Inf", 6 };
    } else {
      sv = (strview_t){
	sc->strbuff,
	sprintf(sc->strbuff, "%.15g", l.as_double)
      };
    }
    break;
  case T_CODEPOINT:
    if (print_flag) {
      sv = mk_strview(escape_char(decode_integer(l), sc->strbuff, false));
    } else {
      sv = (strview_t){
	sc->strbuff,
	utf8proc_encode_char(decode_integer(l), (utf8proc_uint8_t *)sc->strbuff)
      };
    }
    break;
  case T_BOOLEAN:
  case T_SYMBOL:
  case T_KEYWORD:
    sv = to_strview(l);
    break;
  case T_NIL:{
    nanoclj_cell_t * c = decode_pointer(l);
    if (c == NULL) {
      sv = (strview_t){ "nil", 3 };
    } else {
      switch (_type(c)) {
      case T_NIL:
	sv = _to_strview(c);
	break;
      case T_CLASS:
      case T_VAR:{
	nanoclj_val_t ns_v = mk_nil(), name_v = mk_nil();
	get_elem(sc, _cons_metadata(c), sc->NAME, &name_v);
	get_elem(sc, _cons_metadata(c), sc->NS, &ns_v);
	if (!is_nil(name_v)) {
	  sv = to_strview(name_v);
	  if (!is_nil(ns_v)) {
	    strview_t ns = to_strview(ns_v);
	    const char * fmt = _type(c) == T_VAR ? "#'%.*s/%.*s" : "%.*s/%.*s";
	    sv = (strview_t) {
	      sc->strbuff,
	      snprintf(sc->strbuff, STRBUFFSIZE, fmt, (int)ns.size, ns.ptr, (int)sv.size, sv.ptr)
	    };	    
	  } else if (_type(c) == T_VAR) {
	    sv = (strview_t) {
	      sc->strbuff,
	      snprintf(sc->strbuff, STRBUFFSIZE, "#'%.*s", (int)sv.size, sv.ptr)
	    };
	  }
	}
      }
	break;
      case T_LONG:
	sv = (strview_t){
	  sc->strbuff,
	  sprintf(sc->strbuff, "%lld", _lvalue_unchecked(c))
	};
	break;
      case T_DATE:{
	time_t t = _lvalue_unchecked(c) / 1000;
	if (print_flag) {	
	  int msec = _lvalue_unchecked(c) % 1000;
	  struct tm tm;
	  gmtime_r(&t, &tm);
	  sv = (strview_t){
	    sc->strbuff,
	    sprintf(sc->strbuff, "#inst \"%04d-%02d-%02dT%02d:%02d:%02d.%03dZ\"",
		    1900 + tm.tm_year, 1 + tm.tm_mon, tm.tm_mday,
		    tm.tm_hour, tm.tm_min, tm.tm_sec, msec)
	  };
	} else {
	  sv = mk_strview(ctime_r(&t, sc->strbuff));
	  sv.size--;
	}
      }
	break;
      case T_UUID:
	sv = _to_strview(c);
	if (print_flag) {
	  sv = (strview_t){
	    sc->strbuff,
	    snprintf(sc->strbuff, STRBUFFSIZE, "#uuid \"%.*s\"", (int)sv.size, sv.ptr)
	  };	  
	}
	break;
      case T_STRING:
	sv = _to_strview(c);
	if (print_flag) {
	  print_slashstring(sc, sv, out);
	  return;
	}
	break;
      case T_WRITER:{
	nanoclj_port_rep_t * pr = _rep_unchecked(c);
	switch (_port_type_unchecked(c)) {
	case port_string:
	  sv = (strview_t){
	    (char *)pr->string.data->data,
	    pr->string.data->size
	  };
	  break;
#if NANOCLJ_HAS_CANVAS
	case port_canvas:
	  if (_port_type_unchecked(out) == port_file) {
	    nanoclj_port_rep_t * out_pr = _rep_unchecked(out);
	    FILE * fh = out_pr->stdio.file;
	    _column_unchecked(c) = _column_unchecked(out);
	    _line_unchecked(c) = _line_unchecked(out);
	  }
	  if (print_imageview(sc, canvas_get_imageview(pr->canvas.impl), out)) {
	    return;
	  }
#endif
	}
      }
	break;
      case T_ENVIRONMENT:{
	nanoclj_val_t name_v;
	if (get_elem(sc, _cons_metadata(c), sc->NAME, &name_v)) {
	  sv = to_strview(name_v);
	}
	if (print_flag) {
	  sv = (strview_t){
	    sc->strbuff,
	    snprintf(sc->strbuff, STRBUFFSIZE, "#<Namespace %.*s>", (int)sv.size, sv.ptr)
	  };
	}
      }
	break;
      case T_RATIO:
	sv = (strview_t){
	  sc->strbuff,
	  snprintf(sc->strbuff, STRBUFFSIZE, "%lld/%lld", to_long(vector_elem(c, 0)), to_long(vector_elem(c, 1)))
	};
	break;
      case T_FILE:
	sv = _to_strview(c);
	if (print_flag) {
	  sv = (strview_t){
	    sc->strbuff,
	    snprintf(sc->strbuff, STRBUFFSIZE, "#<File %.*s>", (int)sv.size, sv.ptr)
	  };
	}
	break;
      case T_TENSOR:
	print_tensor(sc, _tensor_unchecked(c), out);
	return;
      case T_IMAGE:
	if (print_imageview(sc, _to_imageview(c), out)) {
	  return;
	} else {
	  nanoclj_image_t * img = _image_unchecked(c);
	  sv = (strview_t){
	    sc->strbuff,
	    snprintf(sc->strbuff, STRBUFFSIZE, "#<Image %d %d %d>", img->width, img->height,get_format_channels(img->format))
	  };
	}
	break;
      }
    }
  }
    break;
  }

  if (!sv.ptr) {
    if (isa(sc, sc->Throwable, l)) {
      sv = to_strview(car(l));
    } else {
      nanoclj_val_t name_v;
      if (get_elem(sc, _cons_metadata(get_type_object(sc, l)), sc->NAME, &name_v)) {
	sv = to_strview(name_v);
	sv = (strview_t){
	  sc->strbuff,
	  snprintf(sc->strbuff, STRBUFFSIZE, "#object[%.*s]", (int)sv.size, sv.ptr)
	};
      } else {
	sv = (strview_t){ "#object", 7 };
      }
    }
  }
  
  putchars(sc, sv.ptr, sv.size, out);
}

static inline size_t seq_length(nanoclj_t * sc, nanoclj_cell_t * a) {
  if (!a) {
    return 0;
  }
  size_t i = 0;  
  switch (_type(a)) {
  case T_NIL:
    return 0;
        
  case T_VECTOR:
  case T_ARRAYMAP:
  case T_SORTED_SET:
  case T_QUEUE:
  case T_RATIO:
  case T_STRING:
  case T_CHAR_ARRAY:
  case T_FILE:
  case T_MAPENTRY:
  case T_VAR:
    return get_size(a);

  case T_LIST:
    for (; a; a = next(sc, a), i++) { }
    return i;

  case T_LAZYSEQ:
#if 1
    for (a = seq(sc, a); a; a = next(sc, a), i++) { }
    return i;
#else
    while ( 1 ) {
      if (a == &(sc->_EMPTY)) return i;
      nanoclj_val_t b = _cdr(a);
      if (!is_cell(b)) return i;
      a = decode_pointer(b);
      i++;
    }
#endif
  }
  return 1;
}

/* Creates a collection, args are seqed */
static inline nanoclj_cell_t * mk_collection(nanoclj_t * sc, int type, nanoclj_cell_t * args) {
  size_t len = seq_length(sc, args);

  if (type == T_ARRAYMAP) len /= 2;
  
  nanoclj_cell_t * coll = get_vector_object(sc, type, len);
  if (coll) {
    if (type == T_ARRAYMAP) {
      for (size_t i = 0; i < len; args = rest(sc, args), i++) {
	nanoclj_val_t key = first(sc, args);
	args = rest(sc, args);
	nanoclj_val_t val = first(sc, args);
	set_vector_elem(coll, i, mk_mapentry(sc, key, val));
      }
    } else {
      for (size_t i = 0; i < len; args = rest(sc, args), i++) {
	set_vector_elem(coll, i, first(sc, args));
      }
      if (type == T_SORTED_SET) {
	sort_vector_in_place(coll);
      }
    }
  }
  return coll;
}

static inline nanoclj_val_t mk_format(nanoclj_t * sc, strview_t fmt0, nanoclj_cell_t * args) {
  char * fmt = sc->malloc(2 * fmt0.size + 1);
  size_t fmt_size = 0;
  for (size_t i = 0; i < fmt0.size; i++) {
    if (fmt0.ptr[i] == '%' && i + 1 < fmt0.size && (fmt0.ptr[i + 1] == 's' || fmt0.ptr[i + 1] == 'c')) {
      memcpy(fmt + fmt_size, "%.*s", 4);
      fmt_size += 4;
      i++;
    } else {
      fmt[fmt_size++] = fmt0.ptr[i];
    }
  }
  fmt[fmt_size] = 0;
  
  int n_args = 0, plan = 0, m = 1;
  long long arg0l = 0, arg1l = 0;
  double arg0d = 0.0, arg1d = 0.0;
  strview_t arg0s, arg1s;
  char * p = sc->strbuff;
  
  for (; args; args = next(sc, args), n_args++, m *= 4) {
    nanoclj_val_t arg = first(sc, args);
    if (is_nil(arg)) {
      nanoclj_throw(sc, sc->NullPointerException);
      sc->free(fmt);
      return mk_nil();
    }
    
    switch (type(arg)) {
    case T_LONG:
    case T_DATE:
    case T_PROC:
    case T_BOOLEAN:
      switch (n_args) {
      case 0: arg0l = to_long(arg); break;
      case 1: arg1l = to_long(arg); break;
      }
      plan += 1 * m;
      break;
    case T_REAL:
      switch (n_args) {
      case 0: arg0d = arg.as_double; break;
      case 1: arg1d = arg.as_double; break;
      }
      plan += 2 * m;    
      break;      
    case T_CODEPOINT:{
      size_t len = utf8proc_encode_char(decode_integer(arg), (utf8proc_uint8_t *)p);
      strview_t sv = { p, len };
      p += len;
      switch (n_args) {
      case 0: arg0s = sv; break;
      case 1: arg1s = sv; break;
      }
      plan += 3 * m;
      break;
    }
    case T_STRING:
    case T_CHAR_ARRAY:
    case T_FILE:
    case T_UUID:
    case T_SYMBOL:
    case T_KEYWORD:
      switch (n_args) {
      case 0: arg0s = to_strview(arg); break;
      case 1: arg1s = to_strview(arg); break;
      }
      plan += 3 * m;
      break;
    }
  }

  nanoclj_val_t r;
  switch (plan) {
  case 1: r = mk_string_fmt(sc, fmt, arg0l); break;
  case 2: r = mk_string_fmt(sc, fmt, arg0d); break;
  case 3: r = mk_string_fmt(sc, fmt, (int)arg0s.size, arg0s.ptr); break;
    
  case 5: r = mk_string_fmt(sc, fmt, arg0l, arg1l); break;
  case 6: r = mk_string_fmt(sc, fmt, arg0d, arg1l); break;
  case 7: r = mk_string_fmt(sc, fmt, (int)arg0s.size, arg0s.ptr, arg1l); break;
    
  case 9: r = mk_string_fmt(sc, fmt, arg0l, arg1d); break;
  case 10: r = mk_string_fmt(sc, fmt, arg0d, arg1d); break;
  case 11: r = mk_string_fmt(sc, fmt, (int)arg0s.size, arg0s.ptr, arg1d); break;

  case 13: r = mk_string_fmt(sc, fmt, arg0l, arg1s); break;
  case 14: r = mk_string_fmt(sc, fmt, arg0d, arg1s); break;
  case 15: r = mk_string_fmt(sc, fmt, (int)arg0s.size, arg0s.ptr, (int)arg1s.size, arg1s.ptr); break;

  default: r = mk_string(sc, fmt);
  }
  sc->free(fmt);
  return r;
}

/* ========== Routines for Evaluation Cycle ========== */

/* reverse list --- in-place */
static inline nanoclj_cell_t * reverse_in_place(nanoclj_t * sc, nanoclj_cell_t * p) {
  nanoclj_cell_t * result = &(sc->_EMPTY);
  while (p != &(sc->_EMPTY)) {
    nanoclj_cell_t * q = decode_pointer(_cdr(p));
    _set_cdr(p, mk_pointer(result));
    result = p;
    p = q;
  }
  return result;
}

static inline nanoclj_cell_t * reverse_in_place_with_term(nanoclj_t * sc, nanoclj_val_t term, nanoclj_cell_t * list) {
  nanoclj_cell_t * p = list, * q;
  nanoclj_val_t result = term;
  while (p != &(sc->_EMPTY)) {
    q = decode_pointer(_cdr(p));
    _set_cdr(p, result);
    result = mk_pointer(p);
    p = q;
  }
  return decode_pointer(result);
}

/* ========== Evaluation Cycle ========== */

static nanoclj_val_t get_err_port(nanoclj_t * sc) {
  return slot_value_in_env(find_slot_in_env(sc, sc->envir, sc->ERR, true));
}

static nanoclj_val_t get_out_port(nanoclj_t * sc) {
  return slot_value_in_env(find_slot_in_env(sc, sc->envir, sc->OUT_SYM, true));
}

static nanoclj_val_t get_in_port(nanoclj_t * sc) {
  return slot_value_in_env(find_slot_in_env(sc, sc->envir, sc->IN_SYM, true));
}

/* Too small to turn into function */
#define  BEGIN     do {
#define  END  } while (0)
#define s_goto(sc,a) BEGIN                                  \
    sc->op = (a);                                           \
    return true; END

#define s_return(sc,a) return _s_return(sc,a)

static inline bool _s_return(nanoclj_t * sc, nanoclj_val_t a) { 
  sc->value = a;
  if (sc->dump <= 0) {
    return false;
  }
  sc->dump--;
  dump_stack_frame_t * frame = sc->dump_base + sc->dump;
  
  sc->op = frame->op;
  sc->args = frame->args;
  sc->envir = frame->envir;
  sc->code = frame->code;
#ifdef USE_RECUR_REGISTER
  sc->recur = frame->recur;
#endif
  if (sc->op) {
    return true;
  } else {
    return false;
  }
}

static inline void s_rewind(nanoclj_t * sc) {
  while (sc->dump > 0) {
    dump_stack_frame_t * frame = sc->dump_base + sc->dump - 1;
    if (frame->op) {
      sc->dump--;
    } else {
      break;
    }
  }
}

static inline bool _Error_1(nanoclj_t * sc, const char *msg) {
  nanoclj_throw(sc, mk_runtime_exception(sc, mk_string(sc, msg)));
  return false;
}

#define Error_0(sc,s)    return _Error_1(sc, s)

static inline nanoclj_cell_t * resolve(nanoclj_t * sc, nanoclj_cell_t * env0, nanoclj_val_t sym0) {
  symbol_t * s = decode_symbol(sym0);
  nanoclj_cell_t * env;
  nanoclj_val_t sym;
  bool all_namespaces = true;
  if (s->ns.size) {
    nanoclj_cell_t * c = find_slot_in_env(sc, env0, s->ns_sym, true);
    if (c) {
      env = decode_pointer(slot_value_in_env(c));
      sym = s->name_sym;
      all_namespaces = false;
    } else {
      symbol_t * s2 = decode_symbol(s->ns_sym);
      nanoclj_val_t msg = mk_string_fmt(sc, "%.*s is not defined", (int)s2->name.size, s2->name.ptr);
      nanoclj_throw(sc, mk_runtime_exception(sc, msg));
      return NULL;
    }	  
  } else {
    env = env0;
    sym = sym0;
  }
  return find_slot_in_env(sc, env, sym, all_namespaces);
}

static inline void dump_stack_reset(nanoclj_t * sc) {
  /* in this implementation, sc->dump is the number of frames on the stack */
  sc->dump = 0;
}

static inline void dump_stack_initialize(nanoclj_t * sc) {
  sc->dump_size = 0;
  sc->dump_base = 0;
  dump_stack_reset(sc);
}

static inline void dump_stack_free(nanoclj_t * sc) {
  free(sc->dump_base);
  sc->dump_base = NULL;
  sc->dump = 0;
  sc->dump_size = 0;
}

#define s_retbool(tf)    s_return(sc, mk_boolean(tf))

static inline bool destructure(nanoclj_t * sc, nanoclj_cell_t * binding, nanoclj_cell_t * y, size_t num_args, bool first_level) {
  size_t n = get_size(binding);
  
  if (n >= 2 && vector_elem(binding, n - 2).as_long == sc->AS.as_long) {
    nanoclj_val_t as = vector_elem(binding, n - 1);
    new_slot_in_env(sc, as, mk_pointer(y));
    n -= 2;
  }
  
  if (first_level) {
    bool multi = n >= 2 && vector_elem(binding, n - 2).as_long == sc->AMP.as_long;
    if (!multi && num_args != n) {
      return false;
    } else if (multi && num_args < n - 2) {
      return false;
    }
  }

  for (size_t i = 0; i < n; i++, y = next(sc, y)) {
    nanoclj_val_t e = vector_elem(binding, i);
    if (e.as_long == sc->AMP.as_long) {
      if (++i < n) {
	e = vector_elem(binding, i);
	if (is_primitive(e)) {
	  new_slot_in_env(sc, e, mk_pointer(y));
	} else {
	  if (!destructure(sc, decode_pointer(e), seq(sc, y), 0, false)) {
	    return false;
	  }
	}
      }
      y = NULL;
      break;
    } else if (!first_level || y) {
      if (e.as_long == sc->UNDERSCORE.as_long) {
	/* ignore argument */
      } else if (is_primitive(e)) {
	new_slot_in_env(sc, e, first(sc, y));
      } else {
	nanoclj_val_t y2 = first(sc, y);
	if (!is_cell(y2)) {
	  return false;
	} else if (!destructure(sc, decode_pointer(e), seq(sc, decode_pointer(y2)), 0, false)) {
	  return false;
	}
      }
    } else {
      return false;
    }
  }
  if (first_level && y) {
    /* Too many arguments */
    return false;
  }
  return true;
}

static inline bool destructure_value(nanoclj_t * sc, nanoclj_val_t e, nanoclj_val_t arg) {
  if (e.as_long == sc->UNDERSCORE.as_long) {
    /* ignore argument */
  } else if (is_primitive(e)) {
    new_slot_in_env(sc, e, arg);
  } else if (!is_cell(arg)) {
    return false;
  } else if (!destructure(sc, decode_pointer(e), seq(sc, decode_pointer(arg)), 0, false)) {
    return false;
  }
  return true;
}

/* Constructs an object by type, args are seqed */
static inline nanoclj_val_t mk_object(nanoclj_t * sc, uint_fast16_t t, nanoclj_cell_t * args) {
  nanoclj_val_t x, y;
  nanoclj_cell_t * z;
  int i;
  
  switch (t) {
  case T_NIL:
    return sc->EMPTY;
    
  case T_BOOLEAN:
    return mk_boolean(is_true(first(sc, args)));

  case T_STRING:
  case T_CHAR_ARRAY:
  case T_FILE:
  case T_UUID:{
    strview_t sv = to_strview(first(sc, args));
    return mk_pointer(get_string_object(sc, t, sv.ptr, sv.size, 0));
  }

  case T_VECTOR:
  case T_ARRAYMAP:
  case T_SORTED_SET:
  case T_QUEUE:
    return mk_pointer(mk_collection(sc, t, args));

  case T_CLOSURE:
    x = first(sc, args);
    if (is_cell(x) && first(sc, decode_pointer(x)).as_long == sc->LAMBDA.as_long) {
      x = mk_pointer(rest(sc, decode_pointer(x)));
    }
    args = next(sc, args);
    z = args ? decode_pointer(first(sc, args)) : sc->envir;
    /* make closure. first is code. second is environment */
    return mk_pointer(get_cell(sc, T_CLOSURE, 0, x, z, NULL));
    
  case T_LONG:
    /* Casts the argument to long (or int if it is sufficient) */
    return mk_integer(sc, to_long(first(sc, args)));

  case T_DATE:
    if (!args) {
      return mk_date(sc, system_time() / 1000);
    } else {
      x = first(sc, args);
      if (is_string(x)) {
	strview_t sv = to_strview(x);
	struct tm tm;
	bool is_valid = false;
	if (sv.size >= 10 && sscanf(sv.ptr, "%04d-%02d-%02d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday) == 3) {
	  if (sv.size >= 19 && sv.ptr[10] == 'T' && sscanf(sv.ptr + 11, "%02d:%02d:%02d", &tm.tm_hour, &tm.tm_min, &tm.tm_sec) == 3) {
	    if (sv.size == 19 || (sv.size == 20 && sv.ptr[19] == 'Z')) {
	      is_valid = true;
	    }
	  } else if (sv.size == 10 || (sv.size == 11 && sv.ptr[10] == 'Z')) {
	    tm.tm_sec = tm.tm_min = tm.tm_hour = 0;
	    is_valid = true;
	  }
	}
	if (is_valid) {
	  tm.tm_mon--;
	  tm.tm_year -= 1900;
	  tm.tm_isdst = -1;
	  return mk_date(sc, (long long)timegm(&tm) * 1000);
	}
	return mk_nil();
      } else {
	return mk_date(sc, to_long(x));
      }
    }
  
  case T_REAL:
    x = first(sc, args);
    if (is_string(x)) {
      strview_t sv = to_strview(x);
      size_t n = sv.size < STRBUFFSIZE ? sv.size : STRBUFFSIZE - 1;
      memcpy(sc->strbuff, sv.ptr, n);
      sc->strbuff[n] = 0;
      char * end;
      double d = strtod(sc->strbuff, &end);
      if (sc->strbuff + n == end) return mk_real(d);
      else return mk_nil();
    } else {
      return mk_real(to_double(first(sc, args)));
    }

  case T_CODEPOINT:
    i = to_int(first(sc, args));
    if (i < 0 || i > 0x10ffff) {
      return nanoclj_throw(sc, mk_illegal_arg_exception(sc, "Invalid codepoint"));
    } else {
      return mk_codepoint(i);
    }
    
  case T_CLASS:{
    nanoclj_val_t name = first(sc, args);
    nanoclj_val_t parent = second(sc, args);
    nanoclj_cell_t * meta = mk_meta_from_reader(sc, valarray_peek(sc->load_stack));
    return mk_pointer(mk_class_with_meta(sc, name, gentypeid(sc), decode_pointer(parent), meta));
  }

  case T_ENVIRONMENT:{
    nanoclj_cell_t * parent = NULL;
    if (args) {
      parent = decode_pointer(first(sc, args));
    }
    nanoclj_cell_t * vec = mk_vector(sc, 727);
    fill_vector(vec, mk_nil());
    return mk_pointer(get_cell(sc, T_ENVIRONMENT, 0, mk_pointer(vec), parent, NULL));
  }

  case T_LIST:
    x = first(sc, args);
    y = second(sc, args);
    if (is_cell(y)) {
      nanoclj_cell_t * tail = decode_pointer(y);
      if (!tail) {
	return mk_pointer(get_cell(sc, T_LIST, 0, x, NULL, NULL));
      } else {
	int t = _type(tail);
	if (!_is_sequence(tail) && t != T_LAZYSEQ && t != T_LIST && is_seqable_type(t)) {
	  tail = seq(sc, tail);
	}
	return mk_pointer(get_cell(sc, T_LIST, 0, x, tail, NULL));
      }
    }
    return mk_pointer(get_cell(sc, T_LIST, 0, x, NULL, NULL));

  case T_MAPENTRY:
    return mk_mapentry(sc, first(sc, args), second(sc, args));
        
  case T_SYMBOL:
  case T_KEYWORD:
    x = first(sc, args);
    args = next(sc, args);
    if (args) {
      return def_symbol_from_sv(sc, t, to_strview(x), to_strview(first(sc, args)));
    } else {
      return def_symbol_from_sv(sc, t, mk_strview(0), to_strview(x));
    }
    
  case T_LAZYSEQ:
  case T_DELAY:
    /* make closure. first is code. second is environment */
    return mk_pointer(get_cell(sc, t, 0, mk_pointer(cons(sc, sc->EMPTY, decode_pointer(first(sc, args)))), sc->envir, NULL));
    
  case T_READER:
  case T_INPUT_STREAM:
  case T_GZIP_INPUT_STREAM:
    return mk_pointer(mk_reader(sc, t, args));
    
  case T_WRITER:
  case T_OUTPUT_STREAM:
  case T_GZIP_OUTPUT_STREAM:
    if (!args) {
      return port_from_string(sc, t, mk_strview(0));
    } else {
      x = first(sc, args);
      if (is_nil(x)) {
	nanoclj_throw(sc, sc->NullPointerException);
	return mk_nil();
      } else if (type(x) == t) {
	return x;
      } else if (is_string(x) || is_file(x)) {
	return port_from_filename(sc, t, to_strview(x));
      } else if (is_char_array(x)) {
	return port_from_string(sc, t, to_strview(x));
      } else {
	args = next(sc, args);
	if (args && t == T_WRITER) {
	  y = first(sc, args);
	  nanoclj_color_t fill_color = sc->bg_color;
	  int channels = 3;
	  strview_t pdf_fn = { 0 };
	  for (args = next(sc, args); args; args = next(sc, args)) {
	    nanoclj_val_t a = first(sc, args);
	    if (a.as_long == sc->GRAY.as_long) {
	      channels = 1;
	    } else if (a.as_long == sc->RGB.as_long) {
	      channels = 3;
	    } else if (a.as_long == sc->RGBA.as_long) {
	      channels = 4;
	    } else if (a.as_long == sc->PDF.as_long) {
	      channels = 0;
	      args = next(sc, args);
	      pdf_fn = to_strview(first(sc, args));
	    } else if (!is_nil(a)) {
	      fill_color = to_color(a);
	    }
	  }
#if NANOCLJ_HAS_CANVAS
	  if (channels) {
	    nanoclj_cell_t * port = get_port_object(sc, t, port_canvas);
	    _rep_unchecked(port)->canvas.impl = mk_canvas(sc,
							  (int)(to_double(x) * sc->window_scale_factor),
							  (int)(to_double(y) * sc->window_scale_factor),
							  channels,
							  sc->fg_color, fill_color);
	    return mk_pointer(port);
	  } else if (pdf_fn.size) {
	    nanoclj_cell_t * port = get_port_object(sc, t, port_canvas);
	    _rep_unchecked(port)->canvas.impl = mk_canvas_pdf(sc,
							      (int)(to_double(x) * sc->window_scale_factor),
							      (int)(to_double(y) * sc->window_scale_factor),
							      pdf_fn,
							      sc->fg_color, fill_color);
	    return mk_pointer(port);
	  }
#endif
	}
      }
      return mk_nil();
    }
    break;

  case T_IMAGE:
    if (!args) {
      return mk_image(sc, 0, 0, 0, 0, NULL);
    } else {
      x = first(sc, args);
      if (is_image(x)) {
	return x;
      } else if (is_writer(x) && port_type_unchecked(x) == port_canvas) {
#if NANOCLJ_HAS_CANVAS
	imageview_t iv = canvas_get_imageview(rep_unchecked(x)->canvas.impl);
	nanoclj_val_t img = mk_image(sc, iv.width, iv.height, iv.stride, iv.format, NULL);
	memcpy(image_unchecked(img)->data, iv.ptr, get_image_size(iv.width, iv.height, iv.stride, iv.format));
#endif
	return img;
      }
    }

  case T_TENSOR:
    switch (seq_length(sc, args)) {
    case 1: return mk_tensor_1f(sc, to_long(first(sc, args)));
    case 2: return mk_tensor_2f(sc, to_long(first(sc, args)), to_long(second(sc, args)));
    case 3: return mk_tensor_3f(sc, to_long(first(sc, args)), to_long(second(sc, args)), to_long(third(sc, args)));
    }
  }
  
  return mk_nil();
}

static inline bool unpack_args_0(nanoclj_t * sc) {
  if (!sc->args) {
    return true;
  } else {
    nanoclj_val_t ns = mk_string(sc, "clojure.core");
    const char * fn = dispatch_table[(int)sc->op].name;
    nanoclj_throw(sc, mk_arity_exception(sc, seq_length(sc, sc->args), ns, mk_string(sc, fn)));
    return false;
  }
}

static inline bool unpack_args_0_plus(nanoclj_t * sc, nanoclj_cell_t ** arg_next) {
  *arg_next = sc->args;
  return true;
}

static inline bool unpack_args_1(nanoclj_t * sc, nanoclj_val_t * arg0) {
  if (sc->args) {
    *arg0 = first(sc, sc->args);
    if (next(sc, sc->args) == NULL) {
      return true;
    }
  }
  nanoclj_val_t ns = mk_string(sc, "clojure.core");
  const char * fn = dispatch_table[(int)sc->op].name;
  nanoclj_throw(sc, mk_arity_exception(sc, seq_length(sc, sc->args), ns, mk_string(sc, fn)));
  return false;
}

static inline bool unpack_args_1_plus(nanoclj_t * sc, nanoclj_val_t * arg0, nanoclj_cell_t ** arg_next) {
  if (sc->args) {
    *arg0 = first(sc, sc->args);
    *arg_next = next(sc, sc->args);
    return true;
  }
  nanoclj_val_t ns = mk_string(sc, "clojure.core");
  const char * fn = dispatch_table[(int)sc->op].name;
  nanoclj_throw(sc, mk_arity_exception(sc, seq_length(sc, sc->args), ns, mk_string(sc, fn)));
  return false;
}

static inline bool unpack_args_2(nanoclj_t * sc, nanoclj_val_t * arg0, nanoclj_val_t * arg1) {
  if (sc->args) {
    *arg0 = first(sc, sc->args);
    nanoclj_cell_t * r = next(sc, sc->args);
    if (r) {
      *arg1 = first(sc, r);
      if (next(sc, r) == NULL) {
	return true;
      }
    }
  }
  nanoclj_val_t ns = mk_string(sc, "clojure.core");
  const char * fn = dispatch_table[(int)sc->op].name;
  nanoclj_throw(sc, mk_arity_exception(sc, seq_length(sc, sc->args), ns, mk_string(sc, fn)));
  return false;
}

static inline bool unpack_args_3(nanoclj_t * sc, nanoclj_val_t * arg0, nanoclj_val_t * arg1, nanoclj_val_t * arg2) {
  if (sc->args) {
    *arg0 = first(sc, sc->args);
    nanoclj_cell_t * r = next(sc, sc->args);
    if (r) {
      *arg1 = first(sc, r);
      r = next(sc, r);
      if (r) {
	*arg2 = first(sc, r);
	if (next(sc, r) == NULL) {
	  return true;
	}
      }
    }
  }
  nanoclj_val_t ns = mk_string(sc, "clojure.core");
  const char * fn = dispatch_table[(int)sc->op].name;
  nanoclj_throw(sc, mk_arity_exception(sc, seq_length(sc, sc->args), ns, mk_string(sc, fn)));
  return false;
}

static inline bool unpack_args_5(nanoclj_t * sc, nanoclj_val_t * arg0, nanoclj_val_t * arg1,
				 nanoclj_val_t * arg2, nanoclj_val_t * arg3, nanoclj_val_t * arg4) {
  if (sc->args) {
    *arg0 = first(sc, sc->args);
    nanoclj_cell_t * r = next(sc, sc->args);
    if (r) {
      *arg1 = first(sc, r);
      r = next(sc, r);
      if (r) {
	*arg2 = first(sc, r);
	r = next(sc, r);
	if (r) {
	  *arg3 = first(sc, r);
	  r = next(sc, r);
	  if (r) {
	    *arg4 = first(sc, r);
	    if (next(sc, r) == NULL) {
	      return true;
	    }
	  }
	}
      }
    }
  }
  nanoclj_val_t ns = mk_string(sc, "clojure.core");
  const char * fn = dispatch_table[(int)sc->op].name;
  nanoclj_throw(sc, mk_arity_exception(sc, seq_length(sc, sc->args), ns, mk_string(sc, fn)));
  return false;
}

static inline bool unpack_args_2_plus(nanoclj_t * sc, nanoclj_val_t * arg0, nanoclj_val_t * arg1, nanoclj_cell_t ** arg_next) {
  if (sc->args) {
    *arg0 = first(sc, sc->args);
    nanoclj_cell_t * r = next(sc, sc->args);
    if (r) {
      *arg1 = first(sc, r);
      *arg_next = next(sc, r);
      return true;
    }
  }
  nanoclj_val_t ns = mk_string(sc, "clojure.core");
  const char * fn = dispatch_table[(int)sc->op].name;
  nanoclj_throw(sc, mk_arity_exception(sc, seq_length(sc, sc->args), ns, mk_string(sc, fn)));
  return false;
}

static inline int get_literal_fn_arity(nanoclj_t * sc, nanoclj_val_t body, bool * has_rest) {  
  if (is_list(body)) {
    bool has_rest2;
    int n1 = get_literal_fn_arity(sc, car(body), has_rest);
    int n2 = get_literal_fn_arity(sc, cdr(body), &has_rest2);
    if (has_rest2) *has_rest = true;
    return n1 > n2 ? n1 : n2;
  } else if (body.as_long == sc->ARG_REST.as_long) {
    *has_rest = true;
    return 0;
  } else if (body.as_long == sc->ARG1.as_long) {
    *has_rest = false;
    return 1;
  } else if (body.as_long == sc->ARG2.as_long) {
    *has_rest = false;
    return 2;
  } else if (body.as_long == sc->ARG3.as_long) {
    *has_rest = false;
    return 3;
  } else {
    *has_rest = false;
    return 0;
  }
}

static NANOCLJ_THREAD_SIG thread_main(void *ptr) {
  nanoclj_t * sc = ptr;
  Eval_Cycle(sc, OP_EVAL);
  return 0;
}

static inline void eval_in_thread(nanoclj_t * sc, nanoclj_val_t code) {
  nanoclj_t * child = sc->malloc(sizeof(nanoclj_t));
  memcpy(child, sc, sizeof(nanoclj_t));
  
  child->args = NULL;
  child->code = code;
#ifdef USE_RECUR_REGISTER    
  child->recur = mk_nil();
#endif
  child->tok = 0;

  dump_stack_initialize(child);

  child->pending_exception = NULL;

  child->save_inport = mk_nil();
  child->load_stack = NULL;
  child->rdbuff = NULL;
  child->value = mk_nil();
  child->tensor_ctx = NULL;

  /* init sink */
  child->sink.type = T_LIST;
  child->sink.flags = MARK;
  _set_car(&(child->sink), sc->EMPTY);
  _set_cdr(&(child->sink), sc->EMPTY);
  _cons_metadata(&(child->sink)) = NULL;

  nanoclj_start_thread(thread_main, child);
}

/* Executes and opcode, and returns true if execution should continue */
static inline bool opexe(nanoclj_t * sc, enum nanoclj_opcode op) {
  nanoclj_val_t x, y;  
  nanoclj_cell_t * meta, * z;
  int syn;
  nanoclj_cell_t * params;
  nanoclj_val_t arg0, arg1, arg2, arg3, arg4;
  nanoclj_cell_t * arg_next;
  
#if 0
  strview_t op_sv = to_strview(mk_proc(sc->op));
  fprintf(stderr, "opexe %d %.*s\n", (int)sc->op, (int)op_sv.size, op_sv.ptr);
#endif

  switch (op) {
  case OP_LOAD_FILE:                /* load-file */
    if (!unpack_args_1(sc, &arg0)) {
      return false;
    } else if (!file_push(sc, arg0)) {
      strview_t sv = to_strview(arg0);
      nanoclj_val_t msg = mk_string_fmt(sc, "Unable to open %.*s", (int)sv.size, sv.ptr);
      nanoclj_throw(sc, mk_runtime_exception(sc, msg));
      return false;
    } else {
      s_goto(sc, OP_T0LVL);    
    }

  case OP_T0LVL:               /* top level */
    /* If we reached the end of file, this loop is done. */
    if (port_flags_unchecked(valarray_peek(sc->load_stack)) & PORT_SAW_EOF) {
      if (sc->global_env != sc->root_env) {
#if 0
	fprintf(stderr, "restoring root env\n");
#endif
	sc->envir = sc->global_env = sc->root_env;
      }

      if (nesting_unchecked(valarray_peek(sc->load_stack)) != 0) {
	Error_0(sc, "Unmatched parentheses");
      }
      
      file_pop(sc);
      
      if (sc->load_stack->size == 0) {
#if 0
        sc->args = sc->EMPTY;
        s_goto(sc, OP_QUIT);
#else
	return false;
#endif
      } else {
	s_return(sc, sc->value); 
      }
      /* NOTREACHED */
    }
    
    /* Set up another iteration of REPL */
    sc->save_inport = get_in_port(sc);
    intern(sc, sc->root_env, sc->IN_SYM, valarray_peek(sc->load_stack));
      
    s_save(sc, OP_T0LVL, NULL, sc->EMPTY);
    s_save(sc, OP_T1LVL, NULL, sc->EMPTY);

    sc->args = NULL;
    s_goto(sc, OP_READ);

  case OP_T1LVL:               /* top level */
    sc->code = sc->value;
    intern(sc, sc->root_env, sc->IN_SYM, sc->save_inport);
    s_goto(sc, OP_EVAL);

  case OP_READ:                /* read */
    x = get_in_port(sc);
    if (is_cell(x)) {
      nanoclj_cell_t * inport = decode_pointer(x);
      if (is_readable(inport)) {
	sc->tok = token(sc, inport);
	if (sc->tok == TOK_EOF) {
	  s_return(sc, mk_nil());
	}
	s_goto(sc, OP_RD_SEXPR);
      }
    }
    Error_0(sc, "Not a reader");

  case OP_READM:               /* .read */
    if (!unpack_args_1(sc, &arg0)) {
      return false;
    } else if (is_cell(arg0)) {
      nanoclj_cell_t * p = decode_pointer(arg0);
      if (is_readable(p)) {
	s_return(sc, mk_codepoint(inchar(p)));
      }
    }
    Error_0(sc, "Not a reader");

  case OP_GENSYM:
    if (!unpack_args_0_plus(sc, &arg_next)) {
      return false;
    } else {
      strview_t sv = arg_next ? to_strview(first(sc, arg_next)) : (strview_t){ "G__", 3 };
      s_return(sc, gensym(sc, sv));
    }

  case OP_EVAL:                /* main part of evaluation */
    switch (prim_type(sc->code)) {
    case T_SYMBOL:
      if (sc->code.as_long == sc->CURRENT_NS.as_long) { /* special symbols */
	s_return(sc, mk_pointer(sc->global_env));
      } else if (sc->code.as_long == sc->ENV.as_long) {
	s_return(sc, mk_pointer(sc->envir));
#ifdef USE_RECUR_REGISTER
      } else if (sc->code.as_long == sc->RECUR.as_long) {
	s_return(sc, sc->recur);
#endif
      } else {
	nanoclj_cell_t * c = resolve(sc, sc->envir, sc->code);
	if (sc->pending_exception) {
	  return false;
	} else if (c) {
	  s_return(sc, slot_value_in_env(c));
	} else {
	  symbol_t * s = decode_symbol(sc->code);
	  nanoclj_val_t msg = mk_string_fmt(sc, "Use of undeclared Var %.*s", (int)s->full_name.size, s->full_name.ptr);
	  nanoclj_throw(sc, mk_runtime_exception(sc, msg));
	  return false;
	}
      }
    case T_NIL:{
      nanoclj_cell_t * code_cell = decode_pointer(sc->code);
      if (code_cell) {
	switch (_type(code_cell)) {
	case T_LIST:
	  if ((syn = syntaxnum(_car(code_cell))) != 0) {       /* SYNTAX */
	    sc->code = _cdr(code_cell);
	    s_goto(sc, syn);
	  } else {                  /* first, eval top element and eval arguments */
#ifdef VECTOR_ARGS
	    s_save(sc, OP_E0ARGS, sc->EMPTYVEC, sc->code);
#else
	    s_save(sc, OP_E0ARGS, NULL, sc->code);
#endif
	    /* If no macros => s_save(sc,OP_E1ARGS, sc->EMPTY, cdr(sc->code)); */
	    sc->code = _car(code_cell);
	    s_goto(sc, OP_EVAL);
	  }
	  
	case T_VECTOR:{
	  if (get_size(code_cell) > 0) {
	    s_save(sc, OP_E0VEC, NULL, mk_pointer(copy_vector(sc, decode_pointer(sc->code))));
	    sc->code = vector_elem(code_cell, 0);
	    s_goto(sc, OP_EVAL);
	  }
	}
	}
      }
    }
    }
    /* Default */
    s_return(sc, sc->code);

  case OP_E0VEC:{	       /* eval vector element */
    nanoclj_cell_t * vec = decode_pointer(sc->code);
    long long i = sc->args ? to_long(_car(sc->args)) : 0;
    set_vector_elem(vec, i, sc->value);
    if (i + 1 < get_size(vec)) {
      nanoclj_cell_t * args = sc->args;
      if (args) _set_car(args, mk_integer(sc, i + 1));
      else args = cons(sc, mk_integer(sc, i + 1), NULL);
      s_save(sc, OP_E0VEC, args, sc->code);
      sc->code = vector_elem(vec, i + 1);
      s_goto(sc, OP_EVAL);
    } else {
      s_return(sc, sc->code);
    }
  }
    
  case OP_E0ARGS:              /* eval arguments */
    if (is_macro(sc->value)) {  /* macro expansion */
      s_save(sc, OP_DOMACRO, NULL, sc->EMPTY);
      sc->args = cons(sc, sc->code, NULL);
      sc->code = sc->value;
      s_goto(sc, OP_APPLY);
    } else {
      sc->code = cdr(sc->code);
      s_goto(sc, OP_E1ARGS);
    }

  case OP_E1ARGS:              /* eval arguments */
#ifdef VECTOR_ARGS
    sc->args = conjoin(sc, sc->args, sc->value);
#else
    sc->args = cons(sc, sc->value, sc->args);
#endif
    if (is_list(sc->code)) {    /* continue */
      s_save(sc, OP_E1ARGS, sc->args, cdr(sc->code));
      sc->code = car(sc->code);
      sc->args = NULL;
      s_goto(sc, OP_EVAL);
    } else {                    /* end */
      if (_type(sc->args) == T_LIST) {
	sc->args = reverse_in_place(sc, sc->args);
      }
      sc->code = first(sc, sc->args);
      sc->args = next(sc, sc->args);
      s_goto(sc, OP_APPLY);
    }

  case OP_APPLY:               /* apply 'code' to 'args' */
    switch (prim_type(sc->code)) {
    case T_PROC: 
      s_goto(sc, decode_integer(sc->code));
      
    case T_KEYWORD:
    case T_SYMBOL:{
      nanoclj_val_t coll = first(sc, sc->args), elem;
      if (is_cell(coll) && get_elem(sc, decode_pointer(coll), sc->code, &elem)) {
	s_return(sc, elem);
      } else {
	nanoclj_cell_t * r = next(sc, sc->args);
	if (r) {
	  s_return(sc, first(sc, r));
	} else {
	  s_return(sc, mk_nil());
	}
      }
    }

    case T_NIL:{      
      nanoclj_cell_t * code_cell = decode_pointer(sc->code);
      if (!code_cell) {
	nanoclj_throw(sc, sc->NullPointerException);
	return false;
      }
      bool set_recur_point = false;
      nanoclj_val_t params0;
      
      switch (_type(code_cell)) {
      case T_VAR:
	sc->code = slot_value_in_env(code_cell);
	s_goto(sc, OP_APPLY);

      case T_CLASS:
	x = mk_object(sc, code_cell->type, sc->args);
	if (sc->pending_exception) return false;
	s_return(sc, x);
	
      case T_FOREIGN_FUNCTION:{
	/* Keep nested calls from GC'ing the arglist */
	retain(sc, sc->args);
	if (_min_arity_unchecked(code_cell) > 0 || _max_arity_unchecked(code_cell) != -1) {
	  int n = seq_length(sc, sc->args);
	  if (n < _min_arity_unchecked(code_cell) || n > _max_arity_unchecked(code_cell)) {
	    nanoclj_val_t ns_v = mk_nil(), name_v = mk_nil();
	    get_elem(sc, _ff_metadata(code_cell), sc->NS, &ns_v);
	    get_elem(sc, _ff_metadata(code_cell), sc->NAME, &name_v);
	    nanoclj_throw(sc, mk_arity_exception(sc, n, ns_v, name_v));
	    return false;
	  }
	}
	x = _ff_unchecked(code_cell)(sc, sc->args);
	if (sc->pending_exception) {
	  return false;
	} else {
	  s_return(sc, x);
	}
      }
      case T_FOREIGN_METHOD:{
	/* Keep nested calls from GC'ing the arglist */
	retain(sc, sc->args);
	/* TODO */
	s_return(sc, mk_nil());
      }
      case T_VECTOR:
      case T_ARRAYMAP:
      case T_SORTED_SET:
      case T_IMAGE:
      case T_AUDIO:
      case T_STRING:
      case T_CHAR_ARRAY:
      case T_WRITER:
      case T_READER:
	if (!unpack_args_1_plus(sc, &arg0, &arg_next)) {
	  return false;
	} else if (get_elem(sc, decode_pointer(sc->code), arg0, &x)) {
	  s_return(sc, x);
	} else if (arg_next) {
	  s_return(sc, first(sc, arg_next));
	} else {
	  Error_0(sc, "No item in collection");
	}
      case T_CLOSURE:
	set_recur_point = true;
      case T_MACRO:
      case T_LAZYSEQ:
      case T_DELAY:
	/* Should not accept lazyseq or delay */
	/* make environment */
	new_frame_in_env(sc, closure_env(code_cell));
	x = closure_code(code_cell);

	if (set_recur_point) {
#ifndef USE_RECUR_REGISTER
	  new_slot_in_env(sc, sc->RECUR, sc->code);
#else
	  sc->recur = sc->code;
#endif
	}
	
	params0 = car(x);
	params = decode_pointer(params0);

	if (_type(params) == T_VECTOR) { /* Clojure style arguments */
	  int n = seq_length(sc, sc->args);
	  if (!destructure(sc, params, sc->args, n, true)) {
	    nanoclj_val_t ns_v = mk_nil(), name_v = mk_nil();
	    get_elem(sc, _cons_metadata(code_cell), sc->NS, &ns_v);
	    get_elem(sc, _cons_metadata(code_cell), sc->NAME, &name_v);
	    nanoclj_throw(sc, mk_arity_exception(sc, n, ns_v, name_v));
	    return false;
	  }
	  sc->code = cdr(x);
	} else if (_type(params) == T_LIST && is_vector(_car(params))) { /* Clojure style multi-arity arguments */
	  size_t needed_args = seq_length(sc, sc->args);
	  bool found_match = false;
	  for ( ; x.as_long != sc->EMPTY.as_long; x = cdr(x)) {
	    nanoclj_val_t vec = caar(x);

	    if (destructure(sc, decode_pointer(vec), sc->args, needed_args, true)) {
	      found_match = true;
	      sc->code = cdar(x);
	      break;
	    }
	  }
	  
	  if (!found_match) {
	    nanoclj_val_t ns_v = mk_nil(), name_v = mk_nil();
	    get_elem(sc, _cons_metadata(code_cell), sc->NS, &ns_v);
	    get_elem(sc, _cons_metadata(code_cell), sc->NAME, &name_v);
	    nanoclj_throw(sc, mk_arity_exception(sc, needed_args, ns_v, name_v));
	    return false;
	  }
	} else {
	  x = params0;
	  nanoclj_cell_t * yy = sc->args;
	  for ( ; is_list(x); x = cdr(x), yy = next(sc, yy)) {
	    if (!yy) {
	      nanoclj_val_t ns_v = mk_nil(), name_v = mk_nil();
	      get_elem(sc, _cons_metadata(code_cell), sc->NS, &ns_v);
	      get_elem(sc, _cons_metadata(code_cell), sc->NAME, &name_v);
	      nanoclj_throw(sc, mk_arity_exception(sc, seq_length(sc, sc->args), ns_v, name_v));
	      return false;
	    } else {
	      new_slot_in_env(sc, car(x), first(sc, yy));
	    }
	  }
	  
	  if (x.as_long == sc->EMPTY.as_long) {
	    /*--
	     * if (y.as_long != sc->EMPTY.as_long) {
	     *   Error_0(sc,"too many arguments");
	     * }
	     */
	  } else if (is_symbol(x)) {
	    new_slot_in_env(sc, x, mk_pointer(yy));
	  } else {
	    Error_0(sc, "Syntax error in closure: not a symbol");
	  }
	  sc->code = cdr(closure_code(code_cell));
	}
	sc->args = NULL;
	s_goto(sc, OP_DO);
      }
      break;
    }
    }
    Error_0(sc, "Illegal function");    
    
  case OP_DOMACRO:             /* do macro */
    sc->code = sc->value;
    s_goto(sc, OP_EVAL);

  case OP_LAMBDA:              /* lambda */
    /* If the hook is defined, apply it to sc->code, otherwise
       set sc->value fall thru */
    {
      nanoclj_cell_t * f = find_slot_in_env(sc, sc->envir, sc->COMPILE_HOOK, true);
      if (!f) {
        sc->value = sc->code;
        /* Fallthru */
      } else {
        s_save(sc, OP_LAMBDA1, sc->args, sc->code);
        sc->args = cons(sc, sc->code, NULL);
        sc->code = slot_value_in_env(f);
        s_goto(sc, OP_APPLY);
      }
    }

  case OP_LAMBDA1:{
    x = sc->value;
    bool has_name = false;
    
    if (is_symbol(car(x)) && is_vector(cadr(x))) {
      has_name = true;
    } else if (is_symbol(car(x)) && (is_list(cadr(x)) && is_vector(caadr(x)))) {
      has_name = true;
    }

    if (has_name) {
      x = cdr(x);
    }
    
    /* make closure. first is code. second is environment */
    s_return(sc, mk_pointer(get_cell(sc, T_CLOSURE, 0, x, sc->envir, NULL)));
  }
   
  case OP_QUOTE:               /* quote */
    s_return(sc, car(sc->code));

  case OP_VAR:                 /* var */
    x = car(sc->code);
    if (is_symbol(x)) {
      nanoclj_cell_t * var = resolve(sc, sc->envir, x);
      if (sc->pending_exception) {
	return false;
      } else if (var) {
	s_return(sc, mk_pointer(var));
      } else {
	symbol_t * s = decode_symbol(x);
	nanoclj_val_t msg = mk_string_fmt(sc, "Use of undeclared Var %.*s", (int)s->full_name.size, s->full_name.ptr);
	nanoclj_throw(sc, mk_runtime_exception(sc, msg));
	return false;
      }
    } else {
      Error_0(sc, "Not a symbol");
    }

  case OP_NS_RESOLVE:                /* ns-resolve */
    if (!unpack_args_1_plus(sc, &arg0, &arg_next)) {
      return false;
    } else if (arg_next) {
      s_return(sc, mk_pointer(resolve(sc, decode_pointer(arg0), first(sc, arg_next))));
    } else {
      s_return(sc, mk_pointer(resolve(sc, sc->envir, arg0)));
    }

  case OP_INTERN:
    if (!unpack_args_2_plus(sc, &arg0, &arg1, &arg_next)) {
      return false;
    } else if (!is_environment(arg0) || !is_symbol(arg1)) {
      Error_0(sc, "Invalid types");
    } else { /* arg0: namespace, arg1: symbol */
      nanoclj_cell_t * ns = decode_pointer(arg0);
      if (arg_next) {
	s_return(sc, intern(sc, ns, arg1, first(sc, arg_next)));
      } else {
	s_return(sc, intern_symbol(sc, ns, arg1));
      }
    }
    
  case OP_DEF0:{                /* define */    
    nanoclj_cell_t * code = decode_pointer(sc->code);
    x = _car(code);
    if (!is_symbol(x)) {
      Error_0(sc, "Variable is not a symbol");
    }
    nanoclj_cell_t * meta = mk_meta_from_reader(sc, valarray_peek(sc->load_stack));
    meta = conjoin(sc, meta, mk_mapentry(sc, sc->NAME, x));

    nanoclj_val_t ns_name;
    if (get_elem(sc, _cons_metadata(sc->global_env), sc->NAME, &ns_name)) {
      meta = conjoin(sc, meta, mk_mapentry(sc, sc->NS, ns_name));
    }
      
    if (_caddr(code).as_long != sc->EMPTY.as_long) {
      meta = conjoin(sc, meta, mk_mapentry(sc, sc->DOC, _cadr(code)));
      sc->code = _caddr(code);
    } else {
      sc->code = _cadr(code);
    }
    
    s_save(sc, OP_DEF1, meta, x);
    s_goto(sc, OP_EVAL);
  }
    
  case OP_DEF1:{                /* define */
    meta = sc->args;
    nanoclj_cell_t * var = find_slot_in_env(sc, sc->global_env, sc->code, false);
    if (var) {
      set_slot_in_env(sc, var, sc->value, meta);
    } else {
      var = new_slot_spec_in_env(sc, sc->global_env, sc->code, sc->value, meta);
    }
    s_return(sc, mk_pointer(var));
  }
    
  case OP_SET:
    if (!unpack_args_2(sc, &arg0, &arg1)) {
      return false;
    } else {
      nanoclj_cell_t * var = find_slot_in_env(sc, sc->envir, arg0, true);
      if (var) {
	s_return(sc, set_slot_in_env(sc, var, arg1, NULL));
      } else {
	Error_0(sc, "Use of undeclared var");
      }
    }  
    
  case OP_DO:               /* do */
    if (!is_list(sc->code)) {
      s_return(sc, sc->code);
    }
    if (cdr(sc->code).as_long != sc->EMPTY.as_long) {
      s_save(sc, OP_DO, NULL, cdr(sc->code));
    }
    sc->code = car(sc->code);
    s_goto(sc, OP_EVAL);

  case OP_THREAD:
    if (!is_list(sc->code)) {
      s_return(sc, sc->code);
    }
    eval_in_thread(sc, sc->code);
    s_return(sc, mk_nil());

  case OP_IF0:                 /* if */
    s_save(sc, OP_IF1, NULL, cdr(sc->code));
    sc->code = car(sc->code);
    s_goto(sc, OP_EVAL);

  case OP_IF1:                 /* if */
    if (is_true(sc->value)) {
      sc->code = car(sc->code);
    } else {
      /* (if false 1) ==> () because car(sc->EMPTY) = sc->EMPTY */
      sc->code = cadr(sc->code);
    }
    /* Hack: Clojure differs from Scheme so change '() to nil */
    if (sc->code.as_long == sc->EMPTY.as_long) {
      sc->code = mk_nil();
    }
    s_goto(sc, OP_EVAL);

  case OP_LOOP:{                /* loop */   
    x = car(sc->code);
    if (!is_cell(x)) {
      Error_0(sc, "Syntax error");
    }
    nanoclj_cell_t * input_vec = decode_pointer(x);
    if (_type(input_vec) != T_VECTOR) {
      Error_0(sc, "Syntax error");
    }

    size_t n = get_size(input_vec) / 2;

    nanoclj_cell_t * values = 0;
    nanoclj_cell_t * args = mk_vector(sc, n);

    for (int i = (int)n - 1; i >= 0; i--) {
      set_vector_elem(args, i, vector_elem(input_vec, 2 * i + 0));
      values = cons(sc, vector_elem(input_vec, 2 * i + 1), values);
    }

    nanoclj_cell_t * body = decode_pointer(cdr(sc->code));
    body = cons(sc, mk_pointer(args), body);
    body = get_cell(sc, T_CLOSURE, 0, mk_pointer(body), sc->envir, NULL);
    
    sc->code = mk_pointer(cons(sc, mk_pointer(body), values));
    sc->args = NULL;
    s_goto(sc, OP_EVAL);
  }

  case OP_LET0:                /* let */
    x = car(sc->code);
    
    if (is_vector(x)) {       /* Clojure style */
      nanoclj_cell_t * vec = decode_pointer(x);
      new_frame_in_env(sc, sc->envir);
      s_save(sc, OP_LET1_VEC, NULL, sc->code);
      
      sc->code = vector_elem(vec, 1);
      sc->args = NULL;
      s_goto(sc, OP_EVAL);
    } else {
      sc->args = NULL;
      sc->value = sc->code;
      sc->code = is_symbol(x) ? cadr(sc->code) : x;

      s_goto(sc, OP_LET1);
    }

  case OP_LET1_VEC:{   
    nanoclj_cell_t * vec = decode_pointer(car(sc->code));
    long long i = sc->args ? to_long(_car(sc->args)) : 0;
    destructure_value(sc, vector_elem(vec, i), sc->value);
    
    if (i + 2 < get_size(vec)) {
      nanoclj_cell_t * args = sc->args;
      if (args) _set_car(args, mk_integer(sc, i + 2));
      else args = cons(sc, mk_integer(sc, i + 2), NULL);
      s_save(sc, OP_LET1_VEC, args, sc->code);
      
      sc->code = vector_elem(vec, i + 3);
      sc->args = NULL;
      s_goto(sc, OP_EVAL);      
    } else {
      sc->code = cdr(sc->code);
      sc->args = NULL;
      s_goto(sc, OP_DO);
    }
  }
    
  case OP_LET1:                /* let (calculate parameters) */
    sc->args = cons(sc, sc->value, sc->args);
    
    if (is_list(sc->code)) {    /* continue */
      if (!is_list(car(sc->code)) || !is_list(cdar(sc->code))) {
	Error_0(sc, "Bad syntax of binding spec in let");
      }
      s_save(sc, OP_LET1, sc->args, cdr(sc->code));
      sc->code = cadar(sc->code);
      sc->args = NULL;
      s_goto(sc, OP_EVAL);
    } else {                    /* end */
      sc->args = reverse_in_place(sc, sc->args);
      sc->code = _car(sc->args);
      sc->args = decode_pointer(_cdr(sc->args));
      s_goto(sc, OP_LET2);      
    }

  case OP_LET2:                /* let */
    new_frame_in_env(sc, sc->envir);
    for (x = is_symbol(car(sc->code)) ? cadr(sc->code) : car(sc->code),
	   z = sc->args; z != &(sc->_EMPTY); x = cdr(x), z = decode_pointer(_cdr(z))) {
      new_slot_in_env(sc, caar(x), _car(z));
    }
    if (is_symbol(car(sc->code))) {     /* named let */
      for (x = cadr(sc->code), sc->args = NULL; x.as_long != sc->EMPTY.as_long; x = cdr(x)) {
        if (!is_list(x))
          Error_0(sc, "Bad syntax of binding in let");
        if (!is_list(car(x)))
          Error_0(sc, "Bad syntax of binding in let");
        sc->args = cons(sc, caar(x), sc->args);
      }
      /* make closure. first is code. second is environment */
      x = mk_pointer(get_cell(sc, T_CLOSURE, 0, mk_pointer(cons(sc, mk_pointer(reverse_in_place(sc, sc->args)),
								decode_pointer(cddr(sc->code)))), sc->envir, NULL));

      new_slot_in_env(sc, car(sc->code), x);
      sc->code = cddr(sc->code);
      sc->args = NULL;
    } else {
      sc->code = cdr(sc->code);
      sc->args = NULL;
    }
    s_goto(sc, OP_DO);

  case OP_COND0:               /* cond */
    if (sc->code.as_long == sc->EMPTY.as_long) {
      s_return(sc, mk_nil());
    }
    if (!is_list(sc->code)) {
      Error_0(sc, "Syntax error in cond");
    }
    s_save(sc, OP_COND1, NULL, sc->code);
    sc->code = car(sc->code);
    s_goto(sc, OP_EVAL);

  case OP_COND1:               /* cond */
    if (is_true(sc->value)) {
      if ((sc->code = cdr(sc->code)).as_long == sc->EMPTY.as_long) {
        s_return(sc, sc->value);
      }
      if (is_nil(sc->code)) {
        if (!is_list(cdr(sc->code))) {
          Error_0(sc, "Syntax error in cond");
        }
        x = mk_pointer(cons(sc, sc->QUOTE, cons(sc, sc->value, NULL)));
        sc->code = mk_pointer(cons(sc, cadr(sc->code), cons(sc, x, NULL)));
        s_goto(sc, OP_EVAL);
      }
      sc->code = mk_pointer(cons(sc, car(sc->code), NULL));
      s_goto(sc, OP_DO);
    } else {
      if ((sc->code = cddr(sc->code)).as_long == sc->EMPTY.as_long) {
        s_return(sc, mk_nil());
      } else {
        s_save(sc, OP_COND1, NULL, sc->code);
        sc->code = car(sc->code);
        s_goto(sc, OP_EVAL);
      }
    }
    
  case OP_LAZYSEQ:               /* lazy-seq */
    s_return(sc, mk_pointer(get_cell(sc, T_LAZYSEQ, 0, mk_pointer(cons(sc, sc->EMPTY, decode_pointer(sc->code))), sc->envir, NULL)));

  case OP_AND0:                /* and */
    if (sc->code.as_long == sc->EMPTY.as_long) {
      s_return(sc, mk_boolean(true));
    }
    s_save(sc, OP_AND1, NULL, cdr(sc->code));
    sc->code = car(sc->code);
    s_goto(sc, OP_EVAL);

  case OP_AND1:                /* and */
    if (is_false(sc->value)) {
      s_return(sc, sc->value);
    } else if (sc->code.as_long == sc->EMPTY.as_long) {
      s_return(sc, sc->value);
    } else {
      s_save(sc, OP_AND1, NULL, cdr(sc->code));
      sc->code = car(sc->code);
      s_goto(sc, OP_EVAL);
    }

  case OP_OR0:                 /* or */
    if (sc->code.as_long == sc->EMPTY.as_long) {
      s_return(sc, (nanoclj_val_t)kFALSE);
    }
    s_save(sc, OP_OR1, NULL, cdr(sc->code));
    sc->code = car(sc->code);
    s_goto(sc, OP_EVAL);

  case OP_OR1:                 /* or */
    if (is_true(sc->value)) {
      s_return(sc, sc->value);
    } else if (sc->code.as_long == sc->EMPTY.as_long) {
      s_return(sc, sc->value);
    } else {
      s_save(sc, OP_OR1, NULL, cdr(sc->code));
      sc->code = car(sc->code);
      s_goto(sc, OP_EVAL);
    }

  case OP_MACRO0:              /* macro */
    if (is_list(car(sc->code))) {
      x = caar(sc->code);
      sc->code = mk_pointer(cons(sc, sc->LAMBDA, cons(sc, cdar(sc->code), decode_pointer(cdr(sc->code)))));
    } else {
      x = car(sc->code);
      sc->code = cadr(sc->code);
    }
    if (!is_symbol(x)) {
      Error_0(sc, "Variable is not a symbol");
    }
    s_save(sc, OP_MACRO1, NULL, x);
    s_goto(sc, OP_EVAL);

  case OP_MACRO1:{              /* macro */
    cell_type(sc->value) = T_MACRO;
    nanoclj_cell_t * var = find_slot_in_env(sc, sc->envir, sc->code, false);
    if (var) {
      set_slot_in_env(sc, var, sc->value, NULL);
    } else {
      new_slot_in_env(sc, sc->code, sc->value);
    }
    s_return(sc, sc->code);
  }
    
  case OP_TRY:
    if (!is_list(sc->code)) {
      s_return(sc, sc->code);
    }
    x = car(sc->code);
    y = cdr(sc->code);
    x = eval(sc, decode_pointer(x));
    if (!sc->pending_exception) {
      s_return(sc, x);
    } else {
      nanoclj_val_t e = mk_pointer(sc->pending_exception);
      nanoclj_cell_t * c = seq(sc, decode_pointer(y));
      for (; c; c = next(sc, c)) {
	nanoclj_cell_t * item = decode_pointer(first(sc, c));
	nanoclj_val_t w = first(sc, item);
	if (w.as_long == sc->CATCH.as_long) {
	  item = next(sc, item);
	  nanoclj_val_t type_sym = first(sc, item);
	  nanoclj_cell_t * var = find_slot_in_env(sc, sc->envir, type_sym, true);
	  if (var) {
	    nanoclj_val_t typ = slot_value_in_env(var);
	    if (is_cell(typ) && isa(sc, decode_pointer(typ), e)) {
	      item = next(sc, item);
	      nanoclj_val_t sym = first(sc, item);
	      
	      new_frame_in_env(sc, sc->envir);
	      new_slot_in_env(sc, sym, e);
	      
	      sc->args = NULL;
	      sc->code = mk_pointer(rest(sc, item));
	      sc->pending_exception = NULL;
	      s_goto(sc, OP_DO);
	    }
	  }
	} else {
	  fprintf(stderr, "unhandled exception branch\n");
	}
      }
      fprintf(stderr, "no handler found\n");
      return false;
    }
      
  case OP_PAPPLY:              /* apply* */
    if (!unpack_args_2(sc, &arg0, &arg1)) {
      return false;
    }
    sc->code = arg0;
    sc->args = seq(sc, decode_pointer(arg1));
    s_goto(sc, OP_APPLY);

  case OP_PEVAL:               /* eval */
    if (!unpack_args_1(sc, &arg0)) {
      return false;
    } else {
      sc->code = arg0;
      s_goto(sc, OP_EVAL);
    }

  case OP_RATIONALIZE:
    if (!unpack_args_1(sc, &arg0)) {
      return false;
    } else if (prim_type(arg0) == T_REAL) {
      if (arg0.as_double == 0) {
	nanoclj_throw(sc, mk_arithmetic_exception(sc, "Divide by zero"));
	return false;
      } else {
	x = mk_ratio_from_double(sc, arg0);
	if (is_nil(x)) {
	  nanoclj_throw(sc, mk_arithmetic_exception(sc, "Integer overflow"));
	  return false;
	}
	s_return(sc, x);
      }
    } else {
      s_return(sc, arg0);
    }
    
  case OP_INC:
    if (!unpack_args_1(sc, &arg0)) {
      return false;
    } else if (is_nil(arg0)) {
      nanoclj_throw(sc, sc->NullPointerException);
      return false;
    }
    switch (prim_type(arg0)) {
    case T_LONG: s_return(sc, mk_integer(sc, decode_integer(arg0) + 1));
    case T_REAL: s_return(sc, mk_real(arg0.as_double + 1.0));
    case T_NIL: {
      nanoclj_cell_t * c = decode_pointer(arg0);
      switch (_type(c)) {
      case T_LONG: {
	long long v = _lvalue_unchecked(c);
	if (v < LLONG_MAX) {
	  s_return(sc, mk_integer(sc, v + 1));
	} else {
	  nanoclj_throw(sc, mk_arithmetic_exception(sc, "Integer overflow"));
	  return false;
	}
      }
      case T_RATIO:{
	long long num = to_long(vector_elem(c, 0)), den = to_long(vector_elem(c, 1)), res;
	if (__builtin_saddll_overflow(num, den, &res) == false) {
	  s_return(sc, mk_ratio_long(sc, res, den));
	} else {
	  nanoclj_throw(sc, mk_arithmetic_exception(sc, "Integer overflow"));
	  return false;
	}
      }
      }
    }
    }
    nanoclj_throw(sc, mk_class_cast_exception(sc, "Argument cannot be cast to java.lang.Number"));
    return false;

  case OP_DEC:
    if (!unpack_args_1(sc, &arg0)) {
      return false;
    } else if (is_nil(arg0)) {
      nanoclj_throw(sc, sc->NullPointerException);
      return false;
    }
    switch (prim_type(arg0)) {
    case T_LONG: s_return(sc, mk_integer(sc, decode_integer(arg0) - 1));
    case T_REAL: s_return(sc, mk_real(arg0.as_double - 1.0));
    case T_NIL: {
      nanoclj_cell_t * c = decode_pointer(arg0);
      switch (_type(c)) {
      case T_LONG:{
	long long v = _lvalue_unchecked(c);
	if (v > LLONG_MIN) {
	  s_return(sc, mk_integer(sc, v - 1));
	} else {
	  nanoclj_throw(sc, mk_arithmetic_exception(sc, "Integer overflow"));
	  return false;
	}
      case T_RATIO:{
	long long num = to_long(vector_elem(c, 0)), den = to_long(vector_elem(c, 1)), res;
	if (__builtin_ssubll_overflow(num, den, &res) == false) {
	  s_return(sc, mk_ratio_long(sc, res, den));
	} else {
	  nanoclj_throw(sc, mk_arithmetic_exception(sc, "Integer overflow"));
	  return false;
	}
      }
      }    
      }
    }
    }
    nanoclj_throw(sc, mk_class_cast_exception(sc, "Argument cannot be cast to java.lang.Number"));
    return false;

  case OP_ADD:                 /* add */
    if (!unpack_args_2(sc, &arg0, &arg1)) {
      return false;
    } else if (is_nil(arg0) || is_nil(arg1)) {
      nanoclj_throw(sc, sc->NullPointerException);
      return false;
    } else {
      uint16_t tx = type(arg0), ty = type(arg1);
      if (!is_numeric_type(tx) || !is_numeric_type(ty)) {
	nanoclj_throw(sc, mk_class_cast_exception(sc, "Argument cannot be cast to java.lang.Number"));
	return false;
      } else if (tx == T_TENSOR || ty == T_TENSOR) {
	s_return(sc, tensor_add(sc, to_tensor(sc, arg0), to_tensor(sc, arg1)));
      } else if (tx == T_REAL || ty == T_REAL) {
	s_return(sc, mk_real(to_double(arg0) + to_double(arg1)));
      } else if (tx == T_RATIO || ty == T_RATIO) {
	long long den_x, den_y, den;
	long long num_x = get_ratio(arg0, &den_x);
	long long num_y = get_ratio(arg1, &den_y);
	long long num = normalize(num_x * den_y + num_y * den_x,
				  den_x * den_y, &den);
	if (den == 1) {
	  s_return(sc, mk_integer(sc, num));
	} else {
	  s_return(sc, mk_ratio_long(sc, num, den));
	}
      } else {
	long long res;
	if (__builtin_saddll_overflow(to_long(arg0), to_long(arg1), &res) == false) {
	  s_return(sc, mk_integer(sc, res));
	} else {
	  nanoclj_throw(sc, mk_arithmetic_exception(sc, "Integer overflow"));
	  return false;
	}
      }
    }
    
  case OP_SUB:                 /* minus */
    if (!unpack_args_2(sc, &arg0, &arg1)) {
      return false;
    } else if (is_nil(arg0) || is_nil(arg1)) {
      nanoclj_throw(sc, sc->NullPointerException);
      return false;
    } else {
      uint16_t tx = type(arg0), ty = type(arg1);
      if (!is_numeric_type(tx) || !is_numeric_type(ty)) {
	nanoclj_throw(sc, mk_class_cast_exception(sc, "Argument cannot be cast to java.lang.Number"));
	return false;
      } else if (tx == T_REAL || ty == T_REAL) {
	s_return(sc, mk_real(to_double(arg0) - to_double(arg1)));
      } else if (tx == T_RATIO || ty == T_RATIO) {
	long long den_x, den_y, den;
	long long num_x = get_ratio(arg0, &den_x);
	long long num_y = get_ratio(arg1, &den_y);
	long long num = normalize(num_x * den_y - num_y * den_x,
				  den_x * den_y, &den);
	if (den == 1) {
	  s_return(sc, mk_integer(sc, num));
	} else {
	  s_return(sc, mk_ratio_long(sc, num, den));
	}
      } else {
	long long res;
	if (__builtin_ssubll_overflow(to_long(arg0), to_long(arg1), &res) == false) {
	  s_return(sc, mk_integer(sc, res));
	} else {
	  nanoclj_throw(sc, mk_arithmetic_exception(sc, "Integer overflow"));
	  return false;
	}	
      }
    }
  
  case OP_MUL:                 /* multiply */
    if (!unpack_args_2(sc, &arg0, &arg1)) {
      return false;
    } else if (is_nil(arg0) || is_nil(arg1)) {
      nanoclj_throw(sc, sc->NullPointerException);
      return false;
    } else {
      uint16_t tx = type(arg0), ty = type(arg1);
      if (!is_numeric_type(tx) || !is_numeric_type(ty)) {
      	nanoclj_throw(sc, mk_class_cast_exception(sc, "Argument cannot be cast to java.lang.Number"));
	return false;
      } else if (tx == T_REAL || ty == T_REAL) {
	s_return(sc, mk_real(to_double(arg0) * to_double(arg1)));
      } else if (tx == T_RATIO || ty == T_RATIO) {
	long long den_x, den_y;
	long long num_x = get_ratio(arg0, &den_x);
	long long num_y = get_ratio(arg1, &den_y);
	long long num, den;
	if (__builtin_smulll_overflow(num_x, num_y, &num) ||
	    __builtin_smulll_overflow(den_x, den_y, &den)) {
	  nanoclj_throw(sc, mk_arithmetic_exception(sc, "Integer overflow"));
	  return false;
	}
	num = normalize(num, den, &den);
	if (den == 1) {
	  s_return(sc, mk_integer(sc, num));
	} else {
	  s_return(sc, mk_ratio_long(sc, num, den));
	}
      } else {
	long long res;
	if (__builtin_smulll_overflow(to_long(arg0), to_long(arg1), &res) == false) {
	  s_return(sc, mk_integer(sc, res));
	} else {
	  nanoclj_throw(sc, mk_arithmetic_exception(sc, "Integer overflow"));
	  return false;
	} 
	s_return(sc, mk_integer(sc, res));
      }
    }
	
  case OP_DIV:                 /* divide */
    if (!unpack_args_2(sc, &arg0, &arg1)) {
      return false;
    } else if (is_nil(arg0) || is_nil(arg1)) {
      nanoclj_throw(sc, sc->NullPointerException);
      return false;
    } else {
      uint16_t tx = type(arg0), ty = type(arg1);
      if (!is_numeric_type(tx) || !is_numeric_type(ty)) {
      	nanoclj_throw(sc, mk_class_cast_exception(sc, "Argument cannot be cast to java.lang.Number"));
	return false;
      } else if (tx == T_REAL || ty == T_REAL) {
	s_return(sc, mk_real(to_double(arg0) / to_double(arg1)));
      } else if (tx == T_RATIO || ty == T_RATIO) {
	long long den_x, den_y;
	long long num_x = get_ratio(arg0, &den_x);
	long long num_y = get_ratio(arg1, &den_y);
	long long num, den;
	if (__builtin_smulll_overflow(num_x, den_y, &num) ||
	    __builtin_smulll_overflow(den_x, num_y, &den)) {
	  nanoclj_throw(sc, mk_arithmetic_exception(sc, "Integer overflow"));
	  return false;
	}
	num = normalize(num, den, &den);
	if (den == 1) {
	  s_return(sc, mk_integer(sc, num));
	} else {
	  s_return(sc, mk_ratio_long(sc, num, den));
	}
	s_return(sc, mk_nil());
      } else {
	long long divisor = to_long(arg1);
	if (divisor == 0) {
	  nanoclj_throw(sc, mk_arithmetic_exception(sc, "Divide by zero"));
	  return false;
	} else {
	  long long dividend = to_long(arg0);
	  if (dividend == LLONG_MIN && divisor == -1) {
	    nanoclj_throw(sc, mk_arithmetic_exception(sc, "Integer overflow"));
	    return false;
	  } else if (dividend % divisor == 0) {
	    s_return(sc, mk_integer(sc, dividend / divisor));
	  } else {
	    long long den;
	    long long num = normalize(dividend, divisor, &den);
	    s_return(sc, mk_ratio_long(sc, num, den));
	  }
	}
      }
    }
    
  case OP_REM:                 /* rem */
    if (!unpack_args_2(sc, &arg0, &arg1)) {
      return false;
    } else if (is_nil(arg0) || is_nil(arg1)) {
      nanoclj_throw(sc, sc->NullPointerException);
      return false;
    } else {
      uint16_t tx = type(arg0), ty = type(arg1);
      if (!is_numeric_type(tx) || !is_numeric_type(ty)) {
      	nanoclj_throw(sc, mk_class_cast_exception(sc, "Argument cannot be cast to java.lang.Number"));
	return false;
      } else if (tx == T_REAL || ty == T_REAL) {
	double a = to_double(arg0), b = to_double(arg1);
	if (b == 0.0) {
	  nanoclj_throw(sc, mk_arithmetic_exception(sc, "Division by zero"));
	  return false;
	}
	double res = fmod(a, b);
	/* set remainder sign the same as the first arg */
	if (res > 0 && a < 0) res -= fabs(b);
	else if (res < 0 && a > 0) res += fabs(b);
	
	s_return(sc, mk_real(res));
      } else {
	long long a = to_long(arg0), b = to_long(arg1);
	if (b == 0) {
	  nanoclj_throw(sc, mk_arithmetic_exception(sc, "Division by zero"));
	  return false;
	} else {
	  long long res = a % b;
	  /* set remainder sign the same as the first arg */
	  if (res > 0 && a < 0) res -= labs(b);
	  else if (res < 0 && a > 0) res += labs(b);
	  
	  s_return(sc, mk_integer(sc, res));
	}
      }
    }

  case OP_POP:
    if (!unpack_args_1(sc, &arg0)) {
      return false;
    } else if (is_cell(arg0)) {
      nanoclj_cell_t * c = decode_pointer(arg0);
      if (is_empty(sc, c)) {
	nanoclj_throw(sc, mk_illegal_state_exception(sc, "Can't pop empty collection"));
	return false;
      } else {
	int t = _type(c);
	if (t == T_VECTOR || t == T_STRING) {
	  s_return(sc, mk_pointer(remove_suffix(sc, c, 1)));
	} else if (t == T_QUEUE) {
	  s_return(sc, mk_pointer(remove_prefix(sc, c, 1)));
	} else if (is_seqable_type(t)) {
	  s_return(sc, mk_pointer(rest(sc, c)));
	}
      }
    }
    Error_0(sc, "No protocol method IStack.-pop defined for type");

  case OP_PEEK:
    if (!unpack_args_1(sc, &arg0)) {
      return false;
    } else if (is_cell(arg0)) {
      nanoclj_cell_t * c = decode_pointer(arg0);
      if (c) {
	int t = _type(c);
	if (t != T_QUEUE && is_vector_type(t)) {
	  size_t s = get_size(c);
	  if (s >= 1) s_return(sc, vector_elem(c, get_size(c) - 1));
	  else s_return(sc, mk_nil());
	} else if (is_seqable_type(t)) {
	  s_return(sc, first(sc, c));
	}
      }
    }
    Error_0(sc, "No protocol method IStack.-peek defined for type");
    
  case OP_FIRST:                 /* first */
    if (!unpack_args_1(sc, &arg0)) {
      return false;
    } else if (is_cell(arg0)) {
      nanoclj_cell_t * c = decode_pointer(arg0);
      if (!c || is_seqable_type(_type(c))) {
	s_return(sc, first(sc, c));
      }
    }
    Error_0(sc, "Value is not ISeqable");

  case OP_SECOND:
    if (!unpack_args_1(sc, &arg0)) {
      return false;
    } else if (is_cell(arg0)) {
      nanoclj_cell_t * c = decode_pointer(arg0);
      if (!c || is_seqable_type(_type(c))) {
	s_return(sc, second(sc, c));
      }
    }
    Error_0(sc, "Value is not ISeqable");

  case OP_REST:                 /* rest */
  case OP_NEXT:
    if (!unpack_args_1(sc, &arg0)) {
      return false;
    } else if (is_cell(arg0)) {
      nanoclj_cell_t * c = decode_pointer(arg0);
      if (!c || is_seqable_type(_type(c))) {
	if (op == OP_NEXT) {
	  s_return(sc, mk_pointer(next(sc, c)));
	} else {
	  s_return(sc, mk_pointer(rest(sc, c)));
	}
      }
    }
    Error_0(sc, "Value is not ISeqable");

  case OP_GET:             /* get */
    if (!unpack_args_2_plus(sc, &arg0, &arg1, &arg_next)) {
      return false;
    } else if (is_cell(arg0) && get_elem(sc, decode_pointer(arg0), arg1, &x)) {
      s_return(sc, x);
    } else {
      /* Not found => return the not-found value if provided */
      s_return(sc, first(sc, arg_next));
    }

  case OP_FIND:
    if (!unpack_args_2(sc, &arg0, &arg1)) {
      return false;
    } else if (is_cell(arg0)) {
      nanoclj_cell_t * coll = decode_pointer(arg0);
      if (coll && _type(coll) == T_ARRAYMAP) {
	s_return(sc, mk_pointer(find(sc, coll, arg1)));
      }
    }
    s_return(sc, mk_nil());

  case OP_CONTAINSP:
    if (!unpack_args_2(sc, &arg0, &arg1)) {
      return false;
    }
    s_retbool(is_cell(arg0) && get_elem(sc, decode_pointer(arg0), arg1, NULL));
    
  case OP_CONJ:             /* -conj */
    if (!unpack_args_2(sc, &arg0, &arg1)) {
      return false;
    } else if (is_cell(arg0)) {
      nanoclj_cell_t * coll = decode_pointer(arg0);
      if (!coll) coll = &(sc->_EMPTY);
      switch (_type(coll)) {
      case T_VECTOR:{
	size_t vector_len = get_size(coll);

	/* If MapEntry is added to vector, it is an index value pair */
	if (type(arg1) == T_MAPENTRY) {
	  nanoclj_cell_t * y2 = decode_pointer(arg1);
	  long long index = to_long_w_def(first(sc, y2), -1);
	  nanoclj_val_t value = second(sc, y2);
	  if (index < 0 || index >= vector_len) {
	    Error_0(sc, "Index out of bounds");     
	  } else {
	    nanoclj_cell_t * vec = copy_vector(sc, coll);
	    if (vec) set_vector_elem(vec, index, value);
	    s_return(sc, mk_pointer(vec));
	  }
	} else {
	  s_return(sc, mk_pointer(conjoin(sc, coll, arg1)));
	}
      }
	
      case T_ARRAYMAP:{
	nanoclj_cell_t * p = decode_pointer(arg1);
	if (_type(p) == T_ARRAYMAP) {
	  size_t other_len = get_size(p);
	  for (size_t i = 0; i < other_len; i++) {
	    nanoclj_cell_t * e = decode_pointer(vector_elem(p, i));
	    nanoclj_val_t key = vector_elem(e, 0), val = vector_elem(e, 1);
	    size_t j = find_index(sc, coll, key);
	    if (j != NPOS) {
	      coll = copy_vector(sc, coll);
	      /* Rebuild the mapentry in case the input is not mapentry */
	      set_vector_elem(coll, j, mk_mapentry(sc, key, val));
	    } else {
	      coll = conjoin(sc, coll, mk_mapentry(sc, key, val));
	    }
	  }
	  s_return(sc, mk_pointer(coll));
	} else {
	  nanoclj_val_t key = first(sc, p), val = second(sc, p);
	  size_t i = find_index(sc, coll, key);
	  if (i != NPOS) {
	    coll = copy_vector(sc, coll);
	    /* Rebuild the mapentry in case the input is not mapentry */
	    set_vector_elem(coll, i, mk_mapentry(sc, key, val));
	    s_return(sc, mk_pointer(coll));
	  } else {
	    /* Not found */
	    s_return(sc, mk_pointer(conjoin(sc, coll, mk_mapentry(sc, key, val))));
	  }
	}
      }
      
      case T_SORTED_SET:{
	if (!get_elem(sc, coll, arg1, NULL)) {
	  nanoclj_cell_t * vec = conjoin(sc, coll, arg1);
	  vec = copy_vector(sc, vec);
	  sort_vector_in_place(vec);
	  s_return(sc, mk_pointer(vec));
	} else {
	  s_return(sc, arg0);
	}
      }
	
      case T_NIL:
      case T_LIST:
      case T_LAZYSEQ:
      case T_STRING:
      case T_CHAR_ARRAY:
      case T_FILE:
      case T_QUEUE:
	s_return(sc, mk_pointer(conjoin(sc, coll, arg1)));
      }
    }
    Error_0(sc, "No protocol method ICollection.-conj defined");

  case OP_DISJ:
    if (!unpack_args_2(sc, &arg0, &arg1)) {
      return false;
    } else if (is_cell(arg0)) {
      nanoclj_cell_t * c = decode_pointer(arg0);
      if (!c) {
	s_return(sc, mk_nil());
      } else if (_type(c) == T_SORTED_SET) {
	size_t i = find_index(sc, c, arg1);
	if (i == NPOS) {
	  s_return(sc, arg0);
	} else {
	  size_t n = get_size(c);
	  nanoclj_cell_t * new_set;
	  if (i == 0) {
	    new_set = subvec(sc, c, 1, n - 1);
	  } else {
	    new_set = subvec(sc, c, 0, i);
	    for (i++; i < n; i++) {
	      new_set = conjoin(sc, new_set, vector_elem(c, i));
	    }
	  }
	  s_return(sc, mk_pointer(new_set));
	}
      }
    }
    Error_0(sc, "No protocol method ICollection.-disjoin defined for type");

  case OP_COUNT:
    if (!unpack_args_1(sc, &arg0)) {
      return false;
    } else if (is_cell(arg0)) {      
      s_return(sc, mk_integer(sc, count(sc, decode_pointer(arg0))));
    }
    Error_0(sc, "No protocol method ICounted.-count defined for type");
    
  case OP_SUBVEC:
    if (!unpack_args_2_plus(sc, &arg0, &arg1, &arg_next)) {
      return false;
    } else if (is_cell(arg0)) {
      nanoclj_cell_t * c = decode_pointer(arg0);
      long long start = to_long(arg1);
      long long end;
      if (arg_next) {
	end = to_long(first(sc, arg_next));
      } else {
	end = get_size(c);
      }
      if (end < start) end = start;
      s_return(sc, mk_pointer(subvec(sc, c, start, end - start)));
    }
    Error_0(sc, "Not a vector");

  case OP_SUBS:
    if (!unpack_args_2_plus(sc, &arg0, &arg1, &arg_next)) {
      return false;
    } else if (is_cell(arg0)) {
      nanoclj_cell_t * c = decode_pointer(arg0);
      if (!c) {
	nanoclj_throw(sc, sc->NullPointerException);
	return false;
      } else if (is_string_type(_type(c))) {
	long long start = to_long(arg1);
	if (arg_next) {
	  long long new_n = to_long(first(sc, arg_next)) - start;
	  if (start > 0) c = remove_prefix(sc, c, start);
	  long long n = utf8_num_codepoints(_strvalue(c), get_size(c));
	  if (new_n < n) c = remove_suffix(sc, c, n - new_n);
	} else if (start > 0) {
	  c = remove_prefix(sc, c, start);
	}
	s_return(sc, mk_pointer(c));
      }
    }
    Error_0(sc, "Not a string");

  case OP_NOT:                 /* not */
    if (!unpack_args_1(sc, &arg0)) {
      return false;
    }
    s_retbool(is_false(arg0));
    
  case OP_EQUIV:                  /* equiv */
    if (!unpack_args_2(sc, &arg0, &arg1)) {
      return false;
    } else if (is_nil(arg0) || is_nil(arg1)) {
      nanoclj_throw(sc, sc->NullPointerException);
      return false;
    } else if (!is_number(arg0) || !is_number(arg1)) {
      nanoclj_throw(sc, mk_class_cast_exception(sc, "Argument cannot be cast to java.lang.Number"));
      return false;
    } else {
      s_retbool(equiv(arg0, arg1));
    }
    
  case OP_LT:                  /* lt */
    if (!unpack_args_2(sc, &arg0, &arg1)) {
      return false;
    }
    s_retbool(compare(arg0, arg1) < 0);

  case OP_GT:                  /* gt */
    if (!unpack_args_2(sc, &arg0, &arg1)) {
      return false;
    }
    s_retbool(compare(arg0, arg1) > 0);
  
  case OP_LE:                  /* le */
    if (!unpack_args_2(sc, &arg0, &arg1)) {
      return false;
    }
    s_retbool(compare(arg0, arg1) <= 0);

  case OP_GE:                  /* ge */
    if (!unpack_args_2(sc, &arg0, &arg1)) {
      return false;
    }
    s_retbool(compare(arg0, arg1) >= 0);
    
  case OP_EQ:                  /* equals? */
    if (!unpack_args_2(sc, &arg0, &arg1)) {
      return false;
    }
    s_retbool(equals(sc, arg0, arg1));

  case OP_BIT_AND:
    if (!unpack_args_2(sc, &arg0, &arg1)) {
      return false;
    } else if (is_nil(arg0) || is_nil(arg1)) {
      nanoclj_throw(sc, sc->NullPointerException);
      return false;
    } else {
      s_return(sc, mk_integer(sc, to_long(arg0) & to_long(arg1)));
    }

  case OP_BIT_OR:
    if (!unpack_args_2(sc, &arg0, &arg1)) {
      return false;
    } else if (is_nil(arg0) || is_nil(arg1)) {
      nanoclj_throw(sc, sc->NullPointerException);
      return false;
    } else {
      s_return(sc, mk_integer(sc, to_long(arg0) | to_long(arg1)));
    }

  case OP_BIT_XOR:
    if (!unpack_args_2(sc, &arg0, &arg1)) {
      return false;
    } else if (is_nil(arg0) || is_nil(arg1)) {
      nanoclj_throw(sc, sc->NullPointerException);
      return false;
    } else {
      s_return(sc, mk_integer(sc, to_long(arg0) ^ to_long(arg1)));
    }

  case OP_BIT_SHIFT_LEFT:
    if (!unpack_args_2(sc, &arg0, &arg1)) {
      return false;
    } else if (is_nil(arg0) || is_nil(arg1)) {
      nanoclj_throw(sc, sc->NullPointerException);
      return false;
    } else {
      s_return(sc, mk_integer(sc, to_long(arg0) << to_long(arg1)));
    }

  case OP_BIT_SHIFT_RIGHT:
    if (!unpack_args_2(sc, &arg0, &arg1)) {
      return false;
    } else if (is_nil(arg0) || is_nil(arg1)) {
      nanoclj_throw(sc, sc->NullPointerException);
      return false;
    } else {
      s_return(sc, mk_integer(sc, to_long(arg0) >> to_long(arg1)));
    }

  case OP_UNSIGNED_BIT_SHIFT_RIGHT:
    if (!unpack_args_2(sc, &arg0, &arg1)) {
      return false;
    } else if (is_nil(arg0) || is_nil(arg1)) {
      nanoclj_throw(sc, sc->NullPointerException);
      return false;
    } else {
      s_return(sc, mk_integer(sc, (unsigned long long)to_long(arg0) >> to_long(arg1)));
    }
    
  case OP_TYPE:                /* type */
  case OP_CLASS:
    if (!unpack_args_1(sc, &arg0)) {
      return false;
    } else if (is_nil(arg0)) {
      s_return(sc, mk_nil());
    } else {
      if (op == OP_TYPE && is_cell(arg0)) {
	nanoclj_cell_t * c = decode_pointer(arg0);
	if (c && !_is_gc_atom(c) && get_elem(sc, _cons_metadata(c), sc->TYPE, &x)) {
	  s_return(sc, x);
	}
      }
      s_return(sc, mk_pointer(get_type_object(sc, arg0)));
    }

  case OP_DOT:
    if (!unpack_args_2_plus(sc, &arg0, &arg1, &arg_next)) {
      return false;
    } else if (is_nil(arg0)) {
      nanoclj_throw(sc, sc->NullPointerException);
      return false;
    } else if (!is_symbol(arg1)) {
      Error_0(sc, "Unknown dot form");
    } else {
      nanoclj_cell_t * typ = is_type(arg0) ? decode_pointer(arg0) : get_type_object(sc, arg0);
      while (typ) {
	nanoclj_cell_t * var = resolve(sc, typ, arg1);
	if (var) {
	  nanoclj_val_t v = slot_value_in_env(var);
	  if (!is_nil(v) && (type(v) == T_FOREIGN_FUNCTION || type(v) == T_CLOSURE || type(v) == T_PROC)) {
	    sc->code = v;	    
	    sc->args = cons(sc, arg0, arg_next);
	    s_goto(sc, OP_APPLY);
	  } else {
	    s_return(sc, v);
	  }
	} else {
	  typ = next(sc, typ);
	}
      }
      strview_t sv = to_strview(arg1);
      nanoclj_val_t msg = mk_string_fmt(sc, "%.*s is not a function", (int)sv.size, sv.ptr);
      nanoclj_throw(sc, mk_runtime_exception(sc, msg));
      return false;
    }

  case OP_INSTANCEP:
    if (!unpack_args_2(sc, &arg0, &arg1)) {
      return false;
    } else if (is_cell(arg0)) { /* arg0: type, arg1: object */
      nanoclj_cell_t * t = decode_pointer(arg0);
      if (!t) {
	nanoclj_throw(sc, sc->NullPointerException);
	return false;
      } else if (_type(t) == T_CLASS) {
	s_retbool(isa(sc, t, arg1));
      }
    }
    nanoclj_throw(sc, mk_illegal_arg_exception(sc, "Invalid argument types"));
    return false;

  case OP_IDENTICALP:                  /* identical? */
    if (!unpack_args_2(sc, &arg0, &arg1)) {
      return false;
    }
    s_retbool(arg0.as_long == arg1.as_long);

  case OP_DEREF:
    if (!unpack_args_1(sc, &arg0)) {
      return false;
    } else if (is_cell(arg0)) {
      nanoclj_cell_t * c = decode_pointer(arg0);
      if (c) {
	if (_type(c) == T_LAZYSEQ || _type(c) == T_DELAY) {
	  if (!_is_realized(c)) {
	    /* Should change type to closure here */
	    s_save(sc, OP_SAVE_FORCED, NULL, arg0);
	    sc->code = arg0;
	    sc->args = NULL;
	    s_goto(sc, OP_APPLY);
	  } else if (_type(c) == T_LAZYSEQ && is_nil(_car(c)) && is_nil(_cdr(c))) {
	    s_return(sc, sc->EMPTY);
	  }
	}
	if (_type(c) == T_DELAY) {
	  s_return(sc, _car(c));
	} else if (_type(c) == T_VAR) {
	  s_return(sc, vector_elem(c, 1));
	} else {
	  s_return(sc, arg0);
	}
      }
    }
    s_return(sc, arg0);
    
  case OP_SAVE_FORCED:         /* Save forced value replacing lazy-seq or delay */
    if (!is_cell(sc->code)) {
      s_return(sc, sc->code);
    } else {
      nanoclj_cell_t * c = decode_pointer(sc->code);
      _set_realized(c);
      if (_type(c) == T_DELAY) {
	_set_car(c, sc->value);
	_set_cdr(c, sc->EMPTY);
	s_return(sc, sc->value);
      } else if (is_nil(sc->value) || sc->value.as_long == sc->EMPTY.as_long) {
	_set_car(c, mk_nil());
	_set_cdr(c, mk_nil());
	s_return(sc, sc->EMPTY);
      } else {
	_set_car(c, car(sc->value));
	_set_cdr(c, cdr(sc->value));
	s_return(sc, sc->code);
      }
    }
    
  case OP_PR:               /* pr- */
  case OP_PRINT:            /* print- */
    if (!unpack_args_1(sc, &arg0)) {
      return false;
    } else {
      x = get_out_port(sc);
      if (is_cell(x)) {
	print_primitive(sc, arg0, op == OP_PR, decode_pointer(x));
      }
    }
    s_return(sc, mk_nil());

  case OP_WRITEM:               /* .writer */
    if (!unpack_args_2(sc, &arg0, &arg1)) {
      return false;
    } else if (is_cell(arg0)) {
      nanoclj_cell_t * out = decode_pointer(arg0);
      if (is_writable(out)) {
	if (_type(out) == T_OUTPUT_STREAM) {
	  char c = to_int(arg1);
	  putchars(sc, &c, 1, out);
	} else {
	  print_primitive(sc, arg1, false, out);
	}
	s_return(sc, mk_nil());
      }
    }
    Error_0(sc, "Not a writer");

  case OP_FORMAT:
    if (!unpack_args_1_plus(sc, &arg0, &arg_next)) {
      return false;
    }
    s_return(sc, mk_format(sc, to_strview(arg0), arg_next));

  case OP_THROW:                /* throw */
    if (!unpack_args_1(sc, &arg0)) {
      return false;
    } else {
      sc->pending_exception = get_cell(sc, T_RUNTIME_EXCEPTION, 0, arg0, NULL, NULL);
      return false;
    }
    
  case OP_CLOSEM:        /* .close */
    if (!unpack_args_1(sc, &arg0)) {
      return false;
    } else if (is_nil(arg0)) {
      nanoclj_throw(sc, sc->NullPointerException);
      return false;
    } else if (is_cell(arg0)) {
      nanoclj_cell_t * c = decode_pointer(arg0);
      switch (_type(c)) {
      case T_READER:
      case T_WRITER:
      case T_INPUT_STREAM:
      case T_OUTPUT_STREAM:
      case T_GZIP_INPUT_STREAM:
      case T_GZIP_OUTPUT_STREAM:
	s_return(sc, mk_int(port_close(sc, c)));
      }
      s_return(sc, mk_nil());
    }

    /* ========== reading part ========== */
    
  case OP_RD_SEXPR:
    x = get_in_port(sc);
    if (is_cell(x)) {
      nanoclj_cell_t * inport = decode_pointer(x);
      char * s;
      if (is_readable(inport)) {
	switch (sc->tok) {
	case TOK_EOF:
	  s_return(sc, mk_codepoint(EOF));
	
	case TOK_FN:
	case TOK_LPAREN:
	  if (sc->tok == TOK_FN) {
	    s_save(sc, OP_RD_FN, NULL, sc->EMPTY);
	  }
	  sc->tok = token(sc, inport);
	  if (sc->tok == TOK_RPAREN) {       /* Empty list */
	    s_return(sc, sc->EMPTY);
	  } else if (sc->tok == TOK_OLD_DOT) {
	    Error_0(sc, "Illegal dot expression");
	  } else {
	    _nesting_unchecked(inport)++;
	    s_save(sc, OP_RD_LIST, NULL, sc->EMPTY);
	    s_goto(sc, OP_RD_SEXPR);
	  }
	
	case TOK_VEC:
	  sc->tok = token(sc, inport);
	  if (sc->tok == TOK_RSQUARE) {	/* Empty vector */
	    s_return(sc, mk_pointer(sc->EMPTYVEC));
	  } else if (sc->tok == TOK_OLD_DOT) {
	    Error_0(sc, "Illegal dot expression");
	  } else {
	    _nesting_unchecked(inport)++;
	    s_save(sc, OP_RD_VEC_ELEMENT, sc->EMPTYVEC, sc->EMPTY);
	    s_goto(sc, OP_RD_SEXPR);
	  }      
	
	case TOK_SET:
	case TOK_MAP:
	  if (sc->tok == TOK_SET) {
	    s_save(sc, OP_RD_SET, NULL, sc->EMPTY);
	  } else {
	    s_save(sc, OP_RD_MAP, NULL, sc->EMPTY);
	  }
	  sc->tok = token(sc, inport);
	  if (sc->tok == TOK_RCURLY) {     /* Empty map or set */
	    s_return(sc, sc->EMPTY);
	  } else if (sc->tok == TOK_OLD_DOT) {
	    Error_0(sc, "Illegal dot expression");
	  } else {
	    _nesting_unchecked(inport)++;
	    s_save(sc, OP_RD_MAP_ELEMENT, NULL, sc->EMPTY);
	    s_goto(sc, OP_RD_SEXPR);
	  }
	
	case TOK_QUOTE:
	  s_save(sc, OP_RD_QUOTE, NULL, sc->EMPTY);
	  sc->tok = token(sc, inport);
	  s_goto(sc, OP_RD_SEXPR);
	
	case TOK_DEREF:
	  s_save(sc, OP_RD_DEREF, NULL, sc->EMPTY);
	  sc->tok = token(sc, inport);
	  s_goto(sc, OP_RD_SEXPR);
	
	case TOK_VAR:
	  s_save(sc, OP_RD_VAR, NULL, sc->EMPTY);
	  sc->tok = token(sc, inport);
	  s_goto(sc, OP_RD_SEXPR);      
	
	case TOK_BQUOTE:
	  sc->tok = token(sc, inport);
	  if (sc->tok == TOK_VEC) {
	    s_save(sc, OP_RD_QQUOTEVEC, NULL, sc->EMPTY);
	    sc->tok = TOK_LPAREN; /* ? */
	    s_goto(sc, OP_RD_SEXPR);
	  } else {
	    s_save(sc, OP_RD_QQUOTE, NULL, sc->EMPTY);
	  }
	  s_goto(sc, OP_RD_SEXPR);
	
	case TOK_COMMA:
	  s_save(sc, OP_RD_UNQUOTE, NULL, sc->EMPTY);
	  sc->tok = token(sc, inport);
	  s_goto(sc, OP_RD_SEXPR);
	case TOK_ATMARK:
	  s_save(sc, OP_RD_UQTSP, NULL, sc->EMPTY);
	  sc->tok = token(sc, inport);
	  s_goto(sc, OP_RD_SEXPR);
	case TOK_PRIMITIVE:
	  s = readstr_upto(sc, DELIMITERS, inport, false);
	  x = mk_primitive(sc, s);
	  if (is_nil(x)) {
	    nanoclj_throw(sc, mk_number_format_exception(sc, mk_string_fmt(sc, "Invalid number format [%s]", s)));
	    return false;
	  } else {
	    s_return(sc, x);
	  }
	case TOK_DQUOTE:
	  x = readstrexp(sc, inport, false);
	  if (is_nil(x)) return false;
	  s_return(sc, x);
	
	case TOK_REGEX:
	  x = readstrexp(sc, inport, true);
	  if (!is_nil(x)) {
	    x = mk_regex(sc, x);
	    if (!is_nil(x)) {
	      s_return(sc, x);
	    }
	  }
	  Error_0(sc, "Invalid regex");
	
	case TOK_CHAR_CONST:{
	  const char * p = readstr_upto(sc, DELIMITERS, inport, true);
	  int c = parse_char_literal(sc, p);
	  if (c == -1) {
	    nanoclj_val_t msg = mk_string_fmt(sc, "Undefined character literal: %s", p);
	    nanoclj_throw(sc, mk_runtime_exception(sc, msg));
	    return false;
	  } else {
	    s_return(sc, mk_codepoint(c));
	  }
	}
	    
	case TOK_TAG:{
	  nanoclj_val_t tag = mk_string(sc, readstr_upto(sc, DELIMITERS, inport, false));
	  if (skipspace(sc, inport) != '"') {
	    Error_0(sc, "Invalid literal");	 
	  }
	  nanoclj_val_t value = readstrexp(sc, inport, false);
	  strview_t tag_sv = to_strview(tag);
	
	  if (tag_sv.size == 4 && memcmp(tag_sv.ptr, "inst", 4) == 0) {
	    s_return(sc, mk_object(sc, T_DATE, cons(sc, value, NULL)));	
	  } else if (tag_sv.size == 4 && memcmp(tag_sv.ptr, "uuid", 4) == 0) {
	    s_return(sc, mk_object(sc, T_UUID, cons(sc, value, NULL)));	
	  } else {
	    nanoclj_cell_t * f = find_slot_in_env(sc, sc->envir, sc->TAG_HOOK, true);
	    if (f) {
	      nanoclj_val_t hook = slot_value_in_env(f);
	      if (!is_nil(hook)) {
		sc->code = mk_pointer(cons(sc, hook, cons(sc, tag, cons(sc, value, NULL))));
		s_goto(sc, OP_EVAL);
	      }
	    }
	  
	    nanoclj_val_t msg = mk_string_fmt(sc, "No reader function for tag %.*s", (int)tag_sv.size, tag_sv.ptr);
	    nanoclj_throw(sc, mk_runtime_exception(sc, msg));
	    return false;
	  }
	}

	case TOK_DOT:{
 	  nanoclj_val_t m = mk_primitive(sc, readstr_upto(sc, DELIMITERS, inport, false));
	  strview_t sv = to_strview(m);
	  s_save(sc, OP_RD_DOT, NULL, m);
	  sc->tok = token(sc, inport);
	  if (sc->tok == TOK_RPAREN) {       /* Empty list */
	    s_return(sc, sc->EMPTY);
	  } else if (sc->tok == TOK_OLD_DOT) {
	    Error_0(sc, "Illegal dot expression");
	  } else {
	    _nesting_unchecked(inport)++;
	    s_save(sc, OP_RD_LIST, NULL, sc->EMPTY);
	    s_goto(sc, OP_RD_SEXPR);
	  }
	}

	case TOK_META:
	case TOK_IGNORE:
	  s_save(sc, OP_RD_IGNORE, NULL, sc->EMPTY);
	  sc->tok = token(sc, inport);
	  if (sc->tok == TOK_EOF) {
	    s_return(sc, mk_codepoint(EOF));
	  }
	  s_goto(sc, OP_RD_SEXPR);      
	
	case TOK_SHARP_CONST:
	  x = mk_sharp_const(sc, readstr_upto(sc, DELIMITERS, inport, false));
	  if (is_nil(x)) {
	    Error_0(sc, "Undefined sharp expression");
	  } else {
	    s_return(sc, x);
	  }
	
	default:
	  Error_0(sc, "Illegal token");
	}
	break;
      }
    }
    Error_0(sc, "Not a reader");
    
  case OP_RD_LIST:
    x = get_in_port(sc);
    if (is_cell(x)) {
      nanoclj_cell_t * inport = decode_pointer(x);
      if (is_readable(inport)) {
	sc->args = cons(sc, sc->value, sc->args);
	sc->tok = token(sc, inport);
	if (sc->tok == TOK_EOF) {
	  s_return(sc, mk_codepoint(EOF));
	} else if (sc->tok == TOK_RPAREN) {
	  _nesting_unchecked(inport)--;
	  s_return(sc, mk_pointer(reverse_in_place(sc, sc->args)));
	} else if (sc->tok == TOK_OLD_DOT) {
	  s_save(sc, OP_RD_OLD_DOT, sc->args, sc->EMPTY);
	  sc->tok = token(sc, inport);
	  s_goto(sc, OP_RD_SEXPR);
	} else {
	  s_save(sc, OP_RD_LIST, sc->args, sc->EMPTY);
	  s_goto(sc, OP_RD_SEXPR);
	}
      }
    }
    Error_0(sc, "Not a reader");
    
  case OP_RD_VEC_ELEMENT:
    x = get_in_port(sc);
    if (is_cell(x)) {
      nanoclj_cell_t * inport = decode_pointer(x);
      if (is_readable(inport)) {
	sc->args = conjoin(sc, sc->args, sc->value);
	sc->tok = token(sc, inport);
	if (sc->tok == TOK_EOF) {
	  s_return(sc, mk_codepoint(EOF));
	} else if (sc->tok == TOK_RSQUARE) {
	  _nesting_unchecked(inport)--;
	  s_return(sc, mk_pointer(sc->args));
	} else {
	  s_save(sc, OP_RD_VEC_ELEMENT, sc->args, sc->EMPTY);
	  s_goto(sc, OP_RD_SEXPR);
	}
      }
    }
    Error_0(sc, "Not a reader");

  case OP_RD_MAP_ELEMENT:
    x = get_in_port(sc);
    if (is_cell(x)) {
      nanoclj_cell_t * inport = decode_pointer(x);
      if (is_readable(inport)) {
	sc->args = cons(sc, sc->value, sc->args);
	sc->tok = token(sc, inport);
	if (sc->tok == TOK_EOF) {
	  s_return(sc, mk_codepoint(EOF));
	} else if (sc->tok == TOK_RCURLY) {
	  _nesting_unchecked(inport)--;
	  s_return(sc, mk_pointer(reverse_in_place(sc, sc->args)));
	} else {
	  s_save(sc, OP_RD_MAP_ELEMENT, sc->args, sc->EMPTY);
	  s_goto(sc, OP_RD_SEXPR);
	}
      }
    }
    Error_0(sc, "Not a reader");
    
  case OP_RD_OLD_DOT:
    x = get_in_port(sc);
    if (is_cell(x)) {
      nanoclj_cell_t * inport = decode_pointer(x);
      if (is_readable(inport)) {
	if (token(sc, inport) != TOK_RPAREN) {
	  Error_0(sc, "Illegal dot expression");
	} else {
	  _nesting_unchecked(inport)--;
	  s_return(sc, mk_pointer(reverse_in_place_with_term(sc, sc->value, sc->args)));
	}
      }
    }
    Error_0(sc, "Not a reader");

  case OP_RD_QUOTE:
    s_return(sc, mk_pointer(cons(sc, sc->QUOTE, cons(sc, sc->value, NULL))));

  case OP_RD_DEREF:
    s_return(sc, mk_pointer(cons(sc, sc->DEREF, cons(sc, sc->value, NULL))));

  case OP_RD_VAR:
    s_return(sc, mk_pointer(cons(sc, sc->VAR, cons(sc, sc->value, NULL))));
    
  case OP_RD_QQUOTE:
    s_return(sc, mk_pointer(cons(sc, sc->QQUOTE, cons(sc, sc->value, NULL))));
    
  case OP_RD_QQUOTEVEC:
    s_return(sc, mk_pointer(cons(sc, def_symbol(sc, "apply"),
				 cons(sc, def_symbol(sc, "vector"),
				      cons(sc, mk_pointer(cons(sc, sc->QQUOTE,
							       cons(sc, sc->value, NULL))), NULL)))));
    
  case OP_RD_UNQUOTE:
    s_return(sc, mk_pointer(cons(sc, sc->UNQUOTE, cons(sc, sc->value, NULL))));

  case OP_RD_UQTSP:
    s_return(sc, mk_pointer(cons(sc, sc->UNQUOTESP, cons(sc, sc->value, NULL))));
    
  case OP_RD_FN:{
    bool has_rest;
    int n_args = get_literal_fn_arity(sc, sc->value, &has_rest);
    size_t vsize = n_args + (has_rest ? 2 : 0);
    nanoclj_cell_t * vec = mk_vector(sc, vsize);
    if (n_args >= 1) set_vector_elem(vec, 0, sc->ARG1);
    if (n_args >= 2) set_vector_elem(vec, 1, sc->ARG2);
    if (n_args >= 3) set_vector_elem(vec, 2, sc->ARG3);
    if (has_rest) {
      set_vector_elem(vec, n_args, sc->AMP);
      set_vector_elem(vec, n_args + 1, sc->ARG_REST);
    }
    s_return(sc, mk_pointer(cons(sc, sc->LAMBDA, cons(sc, mk_pointer(vec), cons(sc, sc->value, NULL)))));
  }
    
  case OP_RD_SET:
    if (sc->value.as_long == sc->EMPTY.as_long) {
      s_return(sc, mk_pointer(mk_sorted_set(sc, 0)));
    } else {
      s_return(sc, mk_pointer(cons(sc, sc->SORTED_SET, decode_pointer(sc->value))));
    }
    
  case OP_RD_MAP:
    if (sc->value.as_long == sc->EMPTY.as_long) {
      s_return(sc, mk_pointer(mk_arraymap(sc, 0)));
    } else {
      s_return(sc, mk_pointer(cons(sc, sc->ARRAY_MAP, decode_pointer(sc->value))));
    }

  case OP_RD_DOT:{
    nanoclj_val_t method = sc->code;
    nanoclj_val_t inst = car(sc->value);
    nanoclj_val_t args = cdr(sc->value);
    s_return(sc, mk_pointer(cons(sc, sc->DOT, cons(sc, inst, cons(sc, method, decode_pointer(args))))));
  }

  case OP_RD_IGNORE:
    x = get_in_port(sc);
    if (is_cell(x)) {
      nanoclj_cell_t * inport = decode_pointer(x);
      sc->tok = token(sc, inport);
      if (sc->tok == TOK_EOF) {
	s_return(sc, mk_codepoint(EOF));
      }
      s_goto(sc, OP_RD_SEXPR);
    }
    Error_0(sc, "Not a reader");
	
  case OP_SEQ:
    if (!unpack_args_1(sc, &arg0)) {
      return false;
    } else if (is_cell(arg0)) {
      nanoclj_cell_t * c = decode_pointer(arg0);
      if (!c) s_return(sc, mk_nil());
      if (is_seqable_type(_type(c))) {
	s_return(sc, mk_pointer(seq(sc, c)));
      }
    }
    Error_0(sc, "Value is not ISeqable");

  case OP_RSEQ:
    if (!unpack_args_1(sc, &arg0)) {
      return false;
    } else if (is_cell(arg0)) {
      nanoclj_cell_t * c = decode_pointer(arg0);
      if (c) {
	int t = _type(c);
	if (is_string_type(t) || is_vector_type(t)) {
	  s_return(sc, mk_pointer(rseq(sc, c)));	
	}
      }
    }
    Error_0(sc, "Value is not reverse seqable");

  case OP_EMPTYP:
    if (!unpack_args_1(sc, &arg0)) {
      return false;
    } else if (is_cell(arg0)) {
      nanoclj_cell_t * c = decode_pointer(arg0);
      if (!c) {
	s_retbool(true);
      } else if (is_seqable_type(_type(c))) {
	s_retbool(is_empty(sc, c));
      }
    }
    Error_0(sc, "Value is not ISeqable");
    
  case OP_HASH:
    if (!unpack_args_1(sc, &arg0)) {
      return false;
    } else if (is_primitive(arg0)) {    
      s_return(sc, mk_int(hasheq(sc, arg0)));
    } else {
      nanoclj_cell_t * c = decode_pointer(arg0);
      if (!c) {
	s_return(sc, mk_nil());
      } else {
	if (!(c->flags & T_HASHED)) {
	  c->hasheq = hasheq(sc, arg0);
	  c->flags &= T_HASHED;
	}
	s_return(sc, mk_int(c->hasheq));
      }
    }

  case OP_COMPARE:
    if (!unpack_args_2(sc, &arg0, &arg1)) {
      return false;
    }
    s_return(sc, mk_int(compare(arg0, arg1)));
	     
  case OP_SORT:{
    if (!unpack_args_1(sc, &arg0)) {
      return false;
    }
    x = arg0;
    if (!is_cell(x)) {
      s_return(sc, x);
    }
    nanoclj_cell_t * seq = decode_pointer(x);
    size_t l = seq_length(sc, seq);
    if (l == 0) {
      s_return(sc, sc->EMPTY);
    } else {
      nanoclj_cell_t * vec = mk_vector(sc, l);
      for (size_t i = 0; i < l; i++, seq = rest(sc, seq)) {
	set_vector_elem(vec, i, first(sc, seq));
      }
      sort_vector_in_place(vec);
      nanoclj_cell_t * r = 0;
      for (int i = (int)l - 1; i >= 0; i--) {
	r = conjoin(sc, r, vector_elem(vec, i));
      }
      s_return(sc, mk_pointer(r));
    }
  }

  case OP_UTF8MAP:{
    if (!unpack_args_2(sc, &arg0, &arg1)) {
      return false;
    }
    nanoclj_val_t input = arg0;
    nanoclj_val_t options = arg1;
    utf8proc_uint8_t * dest = NULL;
    strview_t sv = to_strview(input);
    utf8proc_ssize_t s = utf8proc_map((const utf8proc_uint8_t *)sv.ptr,
				      (utf8proc_ssize_t)sv.size,
				      &dest,
				      (utf8proc_option_t)to_long(options)
				      );
    if (s >= 0) {
      nanoclj_val_t r = mk_string_from_sv(sc, (strview_t){ (char *)dest, s });
      free(dest); /* dest is reserved with standard malloc */
      s_return(sc, r);
    } else {
      s_return(sc, mk_nil());
    }
  }

  case OP_TOUPPER:
    if (!unpack_args_1(sc, &arg0)) {
      return false;
    } else {
      s_return(sc, mk_codepoint(utf8proc_toupper(to_int(arg0))));
    }

  case OP_TOLOWER:
    if (!unpack_args_1(sc, &arg0)) {
      return false;
    } else {
      s_return(sc, mk_codepoint(utf8proc_tolower(to_int(arg0))));
    }

  case OP_TOTITLE:
    if (!unpack_args_1(sc, &arg0)) {
      return false;
    } else {
      s_return(sc, mk_codepoint(utf8proc_totitle(to_int(arg0))));
    }

  case OP_CATEGORY:
    if (!unpack_args_1(sc, &arg0)) {
      return false;
    } else {
      s_return(sc, mk_int(utf8proc_category(to_int(arg0))));
    }    

  case OP_META:
    if (!unpack_args_1(sc, &arg0)) {
      return false;
    }
    if (is_cell(arg0)) {
      nanoclj_cell_t * c = decode_pointer(arg0);
      if (c) s_return(sc, mk_pointer(get_metadata(c)));
    }
    s_return(sc, mk_nil());
    
  case OP_IN_NS:
    if (!unpack_args_1(sc, &arg0)) {
      return false;
    } else {
      nanoclj_cell_t * var = find_slot_in_env(sc, sc->root_env, arg0, true);
      nanoclj_cell_t * ns;
      if (var) {
	ns = decode_pointer(slot_value_in_env(var));
      } else {
	nanoclj_cell_t * meta = mk_meta_from_reader(sc, valarray_peek(sc->load_stack));
	meta = conjoin(sc, meta, mk_mapentry(sc, sc->NAME, arg0));
	ns = def_namespace_with_sym(sc, arg0, meta);
      }
      bool r = _s_return(sc, mk_pointer(ns));
      sc->envir = sc->global_env = ns;
      return r;
    }
    
  case OP_RE_PATTERN:
    if (!unpack_args_1(sc, &arg0)) {
      return false;
    } else {
      x = mk_regex(sc, arg0);
      if (is_nil(x)) {
	Error_0(sc, "Invalid regular expression");
      } else {
	s_return(sc, x);
      }
    }

  case OP_RE_FIND:
    if (!unpack_args_2(sc, &arg0, &arg1)) {
      return false;
    } else if (is_cell(arg0)) {
      nanoclj_cell_t * re = decode_pointer(arg0);
      if (!re) {
	nanoclj_throw(sc, sc->NullPointerException);
	return false;
      } else if (_type(re) == T_REGEX) {
	strview_t sv = to_strview(arg1);
	pcre2_match_data * md = pcre2_match_data_create(128, NULL);
	int rc = pcre2_match(_re_unchecked(re), (PCRE2_SPTR8)sv.ptr, sv.size, 0, 0, md, NULL);
	if (rc <= 0) {
	  s_return(sc, mk_nil());
	} else {
	  PCRE2_SIZE * ovec = pcre2_get_ovector_pointer(md);
	  strview_t rsv = (strview_t){ sv.ptr + ovec[0], ovec[1] - ovec[0] };
	  s_return(sc, mk_string_from_sv(sc, rsv));
	}
      }
    }
    nanoclj_throw(sc, mk_illegal_arg_exception(sc, "Invalid argument types"));
    return false;

  case OP_ADD_WATCH:
    if (!unpack_args_3(sc, &arg0, &arg1, &arg2)) {
      return false;
    } else if (is_cell(arg0)) {
      nanoclj_cell_t * c = decode_pointer(arg0);
      if (!c) {
	nanoclj_throw(sc, sc->NullPointerException);
	return false;
      } else if (_type(c) == T_VAR) {
	nanoclj_cell_t * md = get_metadata(c);
	size_t wi = NPOS;
	if (md) wi = find_index(sc, md, sc->WATCHES);
	else md = mk_arraymap(sc, 0);
	if (wi == NPOS) {
	  md = conjoin(sc, md, mk_mapentry(sc, sc->WATCHES, mk_pointer(cons(sc, arg2, NULL))));
	} else {
	  md = copy_vector(sc, md);
	  nanoclj_val_t old_watches = second(sc, decode_pointer(vector_elem(md, wi)));
	  set_vector_elem(md, wi, mk_mapentry(sc, sc->WATCHES, mk_pointer(cons(sc, arg2, decode_pointer(old_watches)))));
	}
	_so_vector_metadata(c) = md;
	s_return(sc, arg0);
      }
    }
    Error_0(sc, "Watches can only be added to Vars");

  case OP_GET_CELL_FLAGS:
    if (!unpack_args_1(sc, &arg0)) {
      return false;
    } else if (is_cell(arg0)) {
      nanoclj_cell_t * c = decode_pointer(arg0);
      if (c) s_return(sc, mk_int(c->flags));
    }
    s_return(sc, mk_int(0));

  case OP_NAMESPACE:
    if (!unpack_args_1(sc, &arg0)) {
      return false;
    } else if (prim_type(arg0) == T_KEYWORD || prim_type(arg0) == T_SYMBOL) {
      x = decode_symbol(arg0)->ns_sym;
      s_return(sc, is_nil(x) ? x : mk_string_from_sv(sc, to_strview(x))); 
    } else {
      nanoclj_throw(sc, mk_illegal_arg_exception(sc, "Invalid argument"));
      return false;
    }
    
  case OP_NAME:
    if (!unpack_args_1(sc, &arg0)) {
      return false;
    } else {
      if (prim_type(arg0) == T_SYMBOL || prim_type(arg0) == T_KEYWORD) {
	symbol_t * s = decode_symbol(arg0);
	if (!is_nil(s->name_sym)) arg0 = s->name_sym;
      }
      s_return(sc, mk_string_from_sv(sc, to_strview(arg0)));
    }
    
  case OP_TENSOR_SET:
    if (!unpack_args_3(sc, &arg0, &arg1, &arg2)) {
      return false;
    } else if (is_cell(arg0)) {
      nanoclj_cell_t * t = decode_pointer(arg0);
      if (!t) {
	nanoclj_throw(sc, sc->NullPointerException);
	return false;
      } else if (_type(t) == T_TENSOR) {
	tensor_set1f(_tensor_unchecked(t), to_long(arg1), to_double(arg2));
	s_return(sc, mk_nil());
      }
    }     
    nanoclj_throw(sc, mk_illegal_arg_exception(sc, "Invalid argument types"));
    return false;
    
  case OP_SET_COLOR:
    if (!unpack_args_1(sc, &arg0)) {
      return false;
    } else if (is_nil(arg0)) {
      nanoclj_throw(sc, sc->NullPointerException);
    } else {
      set_color(sc, to_color(arg0), get_out_port(sc));
    }
    s_return(sc, mk_nil());

  case OP_SET_BG_COLOR:
    if (!unpack_args_1(sc, &arg0)) {
      return false;
    } else if (is_nil(arg0)) {
      nanoclj_throw(sc, sc->NullPointerException);
    } else {
      set_bg_color(sc, to_color(arg0), get_out_port(sc));
    }
    s_return(sc, mk_nil());

  case OP_SET_LINEAR_GRADIENT:
    if (!unpack_args_3(sc, &arg0, &arg1, &arg2)) {
      return false;
    } else if (is_cell(arg0) && is_cell(arg1) && is_cell(arg2)) {
      set_linear_gradient(sc, decode_pointer(arg0), decode_pointer(arg1), decode_pointer(arg2), get_out_port(sc));
      s_return(sc, mk_nil());
    }
    nanoclj_throw(sc, mk_illegal_arg_exception(sc, "Invalid argument types"));
    return false;
        
  case OP_SET_FONT_SIZE:
    if (!unpack_args_1(sc, &arg0)) {
      return false;
    }
#if NANOCLJ_HAS_CANVAS
    x = get_out_port(sc);
    if (is_writer(x) && port_type_unchecked(x) == port_canvas) {
      canvas_set_font_size(rep_unchecked(x)->canvas.impl, to_double(arg0) * sc->window_scale_factor);
    }
#endif
    s_return(sc, mk_nil());
    
  case OP_SET_LINE_WIDTH:
    if (!unpack_args_1(sc, &arg0)) {
      return false;      
    }
#if NANOCLJ_HAS_CANVAS
    x = get_out_port(sc);
    if (is_writer(x) && port_type_unchecked(x) == port_canvas) {
      /* Do not scale line width whit DPI scale factor */
      canvas_set_line_width(rep_unchecked(x)->canvas.impl, to_double(arg0));
    }
#endif
    s_return(sc, mk_nil());

  case OP_MOVETO:
    if (!unpack_args_2(sc, &arg0, &arg1)) {
      return false;
    }
#if NANOCLJ_HAS_CANVAS
    x = get_out_port(sc);
    if (is_writer(x) && port_type_unchecked(x) == port_canvas) {
      canvas_move_to(rep_unchecked(x)->canvas.impl, to_double(arg0) * sc->window_scale_factor, to_double(arg1) * sc->window_scale_factor);
    }
#endif
    s_return(sc, mk_nil());
    
  case OP_LINETO:
    if (!unpack_args_2(sc, &arg0, &arg1)) {
      return false;
    }
#if NANOCLJ_HAS_CANVAS
    x = get_out_port(sc);
    if (is_writer(x) && port_type_unchecked(x) == port_canvas) {
      canvas_line_to(rep_unchecked(x)->canvas.impl, to_double(arg0) * sc->window_scale_factor, to_double(arg1) * sc->window_scale_factor);
    }
#endif
    s_return(sc, mk_nil());

  case OP_ARC:
    if (!unpack_args_5(sc, &arg0, &arg1, &arg2, &arg3, &arg4)) {
      return false;
    }
#if NANOCLJ_HAS_CANVAS
    x = get_out_port(sc);
    if (is_writer(x) && port_type_unchecked(x) == port_canvas) {
      canvas_arc(rep_unchecked(x)->canvas.impl, to_double(arg0) * sc->window_scale_factor, to_double(arg1) * sc->window_scale_factor,
		 to_double(arg2) * sc->window_scale_factor, to_double(arg3), to_double(arg4));
    }
#endif
    s_return(sc, mk_nil());

  case OP_NEW_PATH:
    if (!unpack_args_0(sc)) {
      return false;
    }
#if NANOCLJ_HAS_CANVAS
    x = get_out_port(sc);
    if (is_writer(x) && port_type_unchecked(x) == port_canvas) {
      canvas_new_path(rep_unchecked(x)->canvas.impl);
    }
#endif
    s_return(sc, mk_nil());

  case OP_CLOSE_PATH:
    if (!unpack_args_0(sc)) {
      return false;
    }
#if NANOCLJ_HAS_CANVAS
    x = get_out_port(sc);
    if (is_writer(x) && port_type_unchecked(x) == port_canvas) {
      canvas_close_path(rep_unchecked(x)->canvas.impl);
    }
#endif
    s_return(sc, mk_nil());

  case OP_CLIP:
    if (!unpack_args_0(sc)) {
      return false;
    }
#if NANOCLJ_HAS_CANVAS
    x = get_out_port(sc);
    if (is_writer(x) && port_type_unchecked(x) == port_canvas) {
      canvas_clip(rep_unchecked(x)->canvas.impl);
    }
#endif
    s_return(sc, mk_nil());

  case OP_RESET_CLIP:
    if (!unpack_args_0(sc)) {
      return false;
    }
#if NANOCLJ_HAS_CANVAS
    x = get_out_port(sc);
    if (is_writer(x) && port_type_unchecked(x) == port_canvas) {
      canvas_reset_clip(rep_unchecked(x)->canvas.impl);
    }
#endif
    s_return(sc, mk_nil());

  case OP_STROKE:
    if (!unpack_args_0(sc)) {
      return false;
    }
#if NANOCLJ_HAS_CANVAS
    x = get_out_port(sc);
    if (is_writer(x) && port_type_unchecked(x) == port_canvas) {
      canvas_stroke(rep_unchecked(x)->canvas.impl);
    }
#endif
    s_return(sc, mk_nil());

  case OP_FILL:
    if (!unpack_args_0(sc)) {
      return false;
    }
#if NANOCLJ_HAS_CANVAS
    x = get_out_port(sc);
    if (is_writer(x) && port_type_unchecked(x) == port_canvas) {
      canvas_stroke(rep_unchecked(x)->canvas.impl);
    }
#endif
    s_return(sc, mk_nil());

  case OP_GET_TEXT_EXTENTS:
    if (!unpack_args_1(sc, &arg0)) {
      return false;
    }
#if NANOCLJ_HAS_CANVAS
    x = get_out_port(sc);
    if (is_writer(x) && port_type_unchecked(x) == port_canvas) {
      double width, height;
      canvas_get_text_extents(sc, rep_unchecked(x)->canvas.impl, to_strview(arg0), &width, &height);
      s_return(sc, mk_vector_2d(sc, width / sc->window_scale_factor, height / sc->window_scale_factor));
    }
#endif
    
  case OP_SAVE:
    if (!unpack_args_0(sc)) {
      return false;
    }
#if NANOCLJ_HAS_CANVAS
    x = get_out_port(sc);
    if (is_writer(x) && port_type_unchecked(x) == port_canvas) {
      canvas_save(rep_unchecked(x)->canvas.impl);
    }
#endif
    s_return(sc, mk_nil());

  case OP_RESTORE:
    if (!unpack_args_0(sc)) {
      return false;
    }
    x = get_out_port(sc);
    if (is_writer(x)) {
      nanoclj_port_rep_t * pr = rep_unchecked(x);
      switch (port_type_unchecked(x)) {
#if NANOCLJ_HAS_CANVAS
      case port_canvas:
	canvas_restore(pr->canvas.impl);
	break;
#endif
      case port_file:
	reset_color(pr->stdio.file);
	pr->stdio.fg = sc->fg_color;
	pr->stdio.bg = sc->bg_color;
	break;
      case port_callback:
	if (pr->callback.restore) {
	  pr->callback.restore(sc->ext_data);
	}
	break;
      }
    }
    s_return(sc, mk_nil());

  case OP_RESIZE:
    if (!unpack_args_2(sc, &arg0, &arg1)) {
      return false;
    }
    x = get_out_port(sc);
#if NANOCLJ_HAS_CANVAS
    if (is_writer(x) && port_type_unchecked(x) == port_canvas) {
      rep_unchecked(x)->canvas.impl = mk_canvas(sc,
						(int)(to_double(arg0) * sc->window_scale_factor),
						(int)(to_double(arg1) * sc->window_scale_factor),
						canvas_get_chan(rep_unchecked(x)->canvas.impl),
						sc->fg_color, sc->bg_color);
    }
#endif
    s_return(sc, mk_nil());    

  case OP_FLUSH:
    x = get_out_port(sc);
    if (is_writer(x)) {
      nanoclj_port_rep_t * pr = rep_unchecked(x);
      switch (port_type_unchecked(x)) {
      case port_canvas:
	canvas_flush(pr->canvas.impl);	
	break;
      case port_file:
	fflush(pr->stdio.file);
	break;
      }
    }
    s_return(sc, mk_nil());

  case OP_MODE:
    if (!unpack_args_1(sc, &arg0)) {
      return false;
    } else {
      x = get_out_port(sc);
      if (is_writer(x)) {
	nanoclj_port_rep_t * pr = rep_unchecked(x);
	nanoclj_display_mode_t mode;
	if (arg0.as_long == sc->INLINE.as_long) {
	  mode = nanoclj_mode_inline;
	} else if (arg0.as_long == sc->BLOCK.as_long) {
	  mode = nanoclj_mode_block;
	} else {
	  Error_0(sc, "Invalid mode");
	}
	switch (port_type_unchecked(x)) {
	case port_file:
	  if (mode != pr->stdio.mode) {
	    pr->stdio.mode = mode;
	    set_display_mode(pr->stdio.file, mode);
	  }
	  break;
	}
      }
    }
    s_return(sc, mk_nil());

  case OP_TRANSLATE:
    if (!unpack_args_2(sc, &arg0, &arg1)) {
      return false;
    } else {
      x = get_out_port(sc);
      if (is_writer(x) && port_type_unchecked(x) == port_canvas) {
	canvas_translate(rep_unchecked(x)->canvas.impl, to_double(arg0), to_double(arg1));
      }
      s_return(sc, mk_nil());
    }

  case OP_SCALE:
    if (!unpack_args_2(sc, &arg0, &arg1)) {
      return false;
    } else {
      x = get_out_port(sc);
      if (is_writer(x) && port_type_unchecked(x) == port_canvas) {
	canvas_scale(rep_unchecked(x)->canvas.impl, to_double(arg0), to_double(arg1));
      }
      s_return(sc, mk_nil());
    }

  case OP_ROTATE:
    if (!unpack_args_1(sc, &arg0)) {
      return false;
    } else {
      x = get_out_port(sc);
      if (is_writer(x) && port_type_unchecked(x) == port_canvas) {
	canvas_rotate(rep_unchecked(x)->canvas.impl, to_double(arg0));
      }
      s_return(sc, mk_nil());
    }

  default:
    snprintf(sc->strbuff, STRBUFFSIZE, "%d: illegal operator", sc->op);
    Error_0(sc, sc->strbuff);
  }
  return false;                 /* NOTREACHED */
}
 
/* kernel of this interpreter */
static void Eval_Cycle(nanoclj_t * sc, enum nanoclj_opcode op) {
  sc->op = op;
  for (;;) {
    ok_to_freely_gc(sc);
    bool r = opexe(sc, sc->op);
    if (sc->pending_exception) {
      s_rewind(sc);
      _s_return(sc, mk_nil());
      return;
    }
    if (!r) {
      return;
    }
  }
}

/* ========== Initialization of internal keywords ========== */

static inline void assign_syntax(nanoclj_t * sc, const char *name, unsigned int syntax) {
  strview_t ns_sv = mk_strview(0);
  strview_t name_sv = { name, strlen(name) };
  oblist_add_item(sc, ns_sv, name_sv, mk_symbol(sc, T_SYMBOL, ns_sv, name_sv, syntax));
}

static inline void assign_proc(nanoclj_t * sc, nanoclj_cell_t * ns, enum nanoclj_opcode op, opcode_info * i) {
  nanoclj_val_t ns_sym = mk_nil();
  get_elem(sc, _cons_metadata(ns), sc->NAME, &ns_sym);

  nanoclj_val_t x = def_symbol(sc, i->name);
  nanoclj_val_t y = mk_proc(op);
  nanoclj_cell_t * meta = mk_arraymap(sc, 4);
  set_vector_elem(meta, 0, mk_mapentry(sc, sc->NS, ns_sym));
  set_vector_elem(meta, 1, mk_mapentry(sc, sc->NAME, x));
  set_vector_elem(meta, 2, mk_mapentry(sc, sc->DOC, mk_string(sc, i->doc)));
  set_vector_elem(meta, 3, mk_mapentry(sc, sc->FILE, mk_string(sc, __FILE__)));
  new_slot_spec_in_env(sc, ns, x, y, meta);
}

static inline void update_window_info(nanoclj_t * sc, nanoclj_cell_t * out) {
  nanoclj_val_t size = mk_nil(), cell_size = mk_nil();

  sc->window_lines = 0;
  sc->window_columns = 0;
  
  if (out && is_writable(out) && _port_type_unchecked(out) == port_file) {
    nanoclj_port_rep_t * pr = _rep_unchecked(out);
    FILE * fh = pr->stdio.file;
    int lines, cols, width, height;
    if (get_window_size(stdin, fh, &cols, &lines, &width, &height)) {
      double f = width / cols > 15 ? 2.0 : 1.0;
      sc->window_columns = cols;
      sc->window_lines = lines;
      sc->window_scale_factor = f;
      size = mk_vector_2d(sc, width / f, height / f);
      cell_size = mk_vector_2d(sc, width / cols / f, height / lines / f);
    }

    pr->stdio.fg = sc->fg_color;
    pr->stdio.bg = sc->bg_color;
  }
  
  intern(sc, sc->global_env, sc->WINDOW_SIZE, size);
  intern(sc, sc->global_env, sc->CELL_SIZE, cell_size);
  intern(sc, sc->global_env, sc->WINDOW_SCALE_F, mk_real(sc->window_scale_factor));
} 

/* initialization of nanoclj_t */
#if USE_INTERFACE
static inline nanoclj_val_t cons_checked(nanoclj_t * sc, nanoclj_val_t head, nanoclj_val_t tail) {
  if (is_cell(tail)) {
    return mk_pointer(cons(sc, head, decode_pointer(tail)));
  } else {
    return mk_nil();
  }
}

static nanoclj_val_t checked_car(nanoclj_val_t p) {
  if (is_cell(p) && !is_gc_atom(p)) {
    return car(p);
  } else {
    return mk_nil();
  }
}
static nanoclj_val_t checked_cdr(nanoclj_val_t p) {
  if (is_cell(p) && !is_gc_atom(p)) {
    return cdr(p);
  } else {
    return mk_nil();
  }
}
static void fill_vector_checked(nanoclj_val_t vec, nanoclj_val_t elem) {
  if (is_vector(vec)) {
    fill_vector(decode_pointer(vec), elem);
  }
}
static nanoclj_val_t vector_elem_checked(nanoclj_val_t vec, size_t ielem) {
  if (is_vector(vec)) {
    return vector_elem(decode_pointer(vec), ielem);
  } else {
    return mk_nil();
  }
}
static void set_vector_elem_checked(nanoclj_val_t vec, size_t ielem, nanoclj_val_t newel) {
  if (is_vector(vec)) {
    set_vector_elem(decode_pointer(vec), ielem, newel);
  }
}
static nanoclj_val_t first_checked(nanoclj_t * sc, nanoclj_val_t coll) {
  if (is_cell(coll)) {
    return first(sc, decode_pointer(coll));
  } else {
    return mk_nil();
  }
}

static bool is_empty_checked(nanoclj_t * sc, nanoclj_val_t coll) {
  if (is_cell(coll)) {
    return is_empty(sc, decode_pointer(coll));
  } else {
    return false;
  }
}
static nanoclj_val_t rest_checked(nanoclj_t * sc, nanoclj_val_t coll) {
  if (is_cell(coll)) {
    return mk_pointer(rest(sc, decode_pointer(coll)));
  } else {
    return mk_nil();
  }
}

static size_t size_checked(nanoclj_t * sc, nanoclj_val_t coll) {
  if (is_cell(coll)) {
    return seq_length(sc, decode_pointer(coll));
  } else {
    return 0;
  }
}

nanoclj_val_t nanoclj_mk_vector(nanoclj_t * sc, size_t size) {
  return mk_pointer(mk_vector(sc, size));
}

static struct nanoclj_interface vtbl = {
  intern,
  cons_checked,
  mk_integer,
  mk_real,
  def_symbol,
  mk_string,
  mk_codepoint,
  nanoclj_mk_vector,
  mk_foreign_func,
  mk_boolean,

  is_string,
  to_strview,
  is_number,
  to_long,
  to_double,
  to_int,
  is_vector,
  size_checked,
  fill_vector_checked,
  vector_elem_checked,
  set_vector_elem_checked,
  is_list,
  checked_car,
  checked_cdr,

  first_checked,
  is_empty_checked,
  rest_checked,
  
  is_symbol,
  is_keyword,

  is_closure,
  is_macro,
  is_mapentry,  
  is_environment,
  
  nanoclj_eval_string,
  def_symbol
};
#endif

nanoclj_t *nanoclj_init_new() {
  nanoclj_t *sc = malloc(sizeof(nanoclj_t));
  if (!nanoclj_init(sc)) {
    free(sc);
    return 0;
  } else {
    return sc;
  }
}

nanoclj_t *nanoclj_init_new_custom_alloc(func_alloc malloc, func_dealloc free, func_realloc realloc) {
  nanoclj_t *sc = malloc(sizeof(nanoclj_t));
  if (!nanoclj_init_custom_alloc(sc, malloc, free, realloc)) {
    free(sc);
    return 0;
  } else {
    return sc;
  }
}

bool nanoclj_init(nanoclj_t * sc) {
  return nanoclj_init_custom_alloc(sc, malloc, free, realloc);
}

static inline nanoclj_cell_t * mk_properties(nanoclj_t * sc) {
#ifdef _WIN32
  const char * term = "Windows";
#else
  const char * term = getenv("TERM_PROGRAM");
  if (!term) {
    if (getenv("MLTERM")) term = "mlterm";
    else if (getenv("XTERM_VERSION")) term = "XTerm";
    else if (getenv("KONSOLE_VERSION")) term = "Konsole";
  }
#endif
  
  const char *user_home = NULL, *user_name = NULL;
  const char *os_name = NULL, *os_version = NULL, *os_arch = NULL;
  
#ifndef _WIN32
  struct utsname buf;
  if (uname(&buf) == 0) {
    os_name = buf.sysname;
    os_version = buf.version;
    os_arch = buf.machine;
  }
  struct passwd pwd;
  struct passwd *result;
  
  size_t pw_bufsize = sysconf(_SC_GETPW_R_SIZE_MAX);
  if (pw_bufsize == -1) pw_bufsize = 16384;
  
  char * pw_buf = sc->malloc(pw_bufsize);
  if (pw_buf) {
    getpwuid_r(getuid(), &pwd, pw_buf, pw_bufsize, &result);
    if (result) {
      user_name = result->pw_name;
      user_home = result->pw_dir;
    }
  }
#endif

  char cwd_buf[FILENAME_MAX];
  const char * user_dir = getcwd(cwd_buf, FILENAME_MAX);

#ifdef _WIN32
  const char * file_separator = "\\";
  const char * path_separator = ";";
  const char * line_separator = "\r\n";
#else
  const char * file_separator = "/";
  const char * path_separator = ":";
  const char * line_separator = "\n";
#endif

  nanoclj_val_t ua = mk_string_fmt(sc, "nanoclj/%s", NANOCLJ_VERSION);

  nanoclj_cell_t * l = NULL;
  l = cons(sc, mk_string(sc, user_name), l);
  l = cons(sc, mk_string(sc, "user.name"), l);
  l = cons(sc, mk_string(sc, line_separator), l);
  l = cons(sc, mk_string(sc, "line.separator"), l);
  l = cons(sc, mk_string(sc, file_separator), l);
  l = cons(sc, mk_string(sc, "file.separator"), l);
  l = cons(sc, mk_string(sc, path_separator), l);
  l = cons(sc, mk_string(sc, "path.separator"), l);
  l = cons(sc, mk_string(sc, user_home), l);
  l = cons(sc, mk_string(sc, "user.home"), l);
  l = cons(sc, mk_string(sc, user_dir), l);
  l = cons(sc, mk_string(sc, "user.dir"), l);
  l = cons(sc, mk_string(sc, os_name), l);
  l = cons(sc, mk_string(sc, "os.name"), l);
  l = cons(sc, mk_string(sc, os_version), l);
  l = cons(sc, mk_string(sc, "os.version"), l);
  l = cons(sc, mk_string(sc, os_arch), l);
  l = cons(sc, mk_string(sc, "os.arch"), l);
  l = cons(sc, mk_nil(), l);
  l = cons(sc, mk_string(sc, "java.class.path"), l);
  l = cons(sc, mk_string(sc, term), l);
  l = cons(sc, mk_string(sc, "term"), l);
  l = cons(sc, mk_string(sc, NANOCLJ_VERSION), l);
  l = cons(sc, mk_string(sc, "nanoclj.version"), l);
  l = cons(sc, ua, l);
  l = cons(sc, mk_string(sc, "http.agent"), l);
  l = cons(sc, mk_boolean(true), l);
  l = cons(sc, mk_string(sc, "http.keepalive"), l);
  l = cons(sc, mk_int(5), l);
  l = cons(sc, mk_string(sc, "http.maxConnections"), l);
  l = cons(sc, mk_int(20), l);
  l = cons(sc, mk_string(sc, "http.maxRedirects"), l);
  
  sc->free(pw_buf);

  return mk_collection(sc, T_ARRAYMAP, l);
}

#include "nanoclj_functions.h"
 
bool nanoclj_init_custom_alloc(nanoclj_t * sc, func_alloc malloc, func_dealloc free, func_realloc realloc) {
#if USE_INTERFACE
  sc->vptr = &vtbl;
#endif
  sc->malloc = malloc;
  sc->free = free;
  sc->realloc = realloc;

  sc->context = malloc(sizeof(nanoclj_shared_context_t));
  
  sc->context->mutex = nanoclj_mutex_create();
  sc->context->gensym_cnt = 0;
  sc->context->gentypeid_cnt = T_LAST_SYSTEM_TYPE;
  sc->context->fcells = 0;
  sc->context->free_cell = NULL;
  sc->context->last_cell_seg = -1;
  sc->context->properties = NULL;
  sc->context->n_seg_reserved = 0;
  sc->context->alloc_seg = NULL;
  sc->context->cell_seg = NULL;
  
  sc->EMPTY = mk_pointer(&sc->_EMPTY);
  sc->save_inport = mk_nil();
  sc->global_env = sc->root_env = sc->envir = NULL;

  sc->pending_exception = NULL;

  sc->rdbuff = mk_bytearray(sc, 0, 0);
  sc->load_stack = mk_valarray(sc, 0, 256);

  if (alloc_cellseg(sc, FIRST_CELLSEGS) != FIRST_CELLSEGS) {
    return false;
  }
  dump_stack_initialize(sc);
  sc->code = sc->EMPTY;
#ifdef USE_RECUR_REGISTER
  sc->recur = sc->EMPTY;
#endif

  /* init sink */
  sc->sink.type = T_LIST;
  sc->sink.flags = MARK;
  _set_car(&(sc->sink), sc->EMPTY);
  _set_cdr(&(sc->sink), sc->EMPTY);
  _cons_metadata(&(sc->sink)) = NULL;
  
  /* init sc->EMPTY */
  sc->_EMPTY.type = T_NIL;
  sc->_EMPTY.flags = T_GC_ATOM | MARK;
  _set_car(&(sc->_EMPTY), sc->EMPTY);
  _set_cdr(&(sc->_EMPTY), sc->EMPTY);
  _cons_metadata(&(sc->_EMPTY)) = NULL;
  
  sc->oblist = oblist_initial_value(sc);
  
  assign_syntax(sc, "fn", OP_LAMBDA);
  assign_syntax(sc, "quote", OP_QUOTE);
  assign_syntax(sc, "var", OP_VAR);
  assign_syntax(sc, "def", OP_DEF0);
  assign_syntax(sc, "if", OP_IF0);
  assign_syntax(sc, "do", OP_DO);
  assign_syntax(sc, "let", OP_LET0);
  assign_syntax(sc, "cond", OP_COND0);
  assign_syntax(sc, "lazy-seq", OP_LAZYSEQ);
  assign_syntax(sc, "and", OP_AND0);
  assign_syntax(sc, "or", OP_OR0);
  assign_syntax(sc, "macro", OP_MACRO0);
  assign_syntax(sc, "try", OP_TRY);
  assign_syntax(sc, "loop", OP_LOOP);
  assign_syntax(sc, "thread", OP_THREAD);

  /* initialization of global nanoclj_val_ts to special symbols */
  sc->LAMBDA = def_symbol(sc, "fn");
  sc->DO = def_symbol(sc, "do");
  sc->QUOTE = def_symbol(sc, "quote");
  sc->DEREF = def_symbol(sc, "deref");
  sc->VAR = def_symbol(sc, "var");
  sc->QQUOTE = def_symbol(sc, "quasiquote");
  sc->UNQUOTE = def_symbol(sc, "unquote");
  sc->UNQUOTESP = def_symbol(sc, "unquote-splicing");

  sc->ARG1 = def_symbol(sc, "%1");
  sc->ARG2 = def_symbol(sc, "%2");
  sc->ARG3 = def_symbol(sc, "%3");
  sc->ARG_REST = def_symbol(sc, "%&");

  sc->TAG_HOOK = def_symbol(sc, "*default-data-reader-fn*");
  sc->COMPILE_HOOK = def_symbol(sc, "*compile-hook*");
  sc->IN_SYM = def_symbol(sc, "*in*");
  sc->OUT_SYM = def_symbol(sc, "*out*");
  sc->ERR = def_symbol(sc, "*err*");
  sc->CURRENT_NS = def_symbol(sc, "*ns*");
  sc->ENV = def_symbol(sc, "*env*");
  sc->CELL_SIZE = def_symbol(sc, "*cell-size*");
  sc->WINDOW_SIZE = def_symbol(sc, "*window-size*");
  sc->WINDOW_SCALE_F = def_symbol(sc, "*window-scale-factor*");
  sc->MOUSE_POS = def_symbol(sc, "*mouse-pos*");
  sc->THEME = def_symbol(sc, "*theme*");

  sc->RECUR = def_symbol(sc, "recur");
  sc->AMP = def_symbol(sc, "&");
  sc->UNDERSCORE = def_symbol(sc, "_");
  sc->AS = def_keyword(sc, "as");
  sc->DOC = def_keyword(sc, "doc");
  sc->WIDTH = def_keyword(sc, "width");
  sc->HEIGHT = def_keyword(sc, "height");
  sc->CHANNELS = def_keyword(sc, "channels");
  sc->GRAPHICS = def_keyword(sc, "graphics");
  sc->WATCHES = def_keyword(sc, "watches");
  sc->NAME = def_keyword(sc, "name");
  sc->TYPE = def_keyword(sc, "type");
  sc->LINE = def_keyword(sc, "line");
  sc->COLUMN = def_keyword(sc, "column");
  sc->FILE = def_keyword(sc, "file");
  sc->NS = def_keyword(sc, "ns");
  sc->INLINE = def_keyword(sc, "inline");
  sc->BLOCK = def_keyword(sc, "block");
  sc->GRAY = def_keyword(sc, "gray");
  sc->RGB = def_keyword(sc, "rgb");
  sc->RGBA = def_keyword(sc, "rgba");
  sc->PDF = def_keyword(sc, "pdf");
  
  sc->SORTED_SET = def_symbol(sc, "sorted-set");
  sc->ARRAY_MAP = def_symbol(sc, "array-map");
  sc->DOT = def_symbol(sc, ".");
  sc->CATCH = def_symbol(sc, "catch");
  sc->FINALLY = def_symbol(sc, "finally");

  sc->EMPTYVEC = mk_vector(sc, 0);

  /* Init root namespace clojure.core */
  sc->root_env = sc->global_env = sc->envir = def_namespace(sc, "clojure.core", __FILE__);

  sc->types = mk_valarray(sc, 0, 1024);

  size_t n = sizeof(dispatch_table) / sizeof(dispatch_table[0]);
  for (size_t i = 0; i < n; i++) {
    if (dispatch_table[i].name != 0) {
      assign_proc(sc, sc->root_env, (enum nanoclj_opcode)i, &(dispatch_table[i]));
    }
  }

  /* Java types */

  sc->Object = mk_class(sc, "java.lang.Object", gentypeid(sc), sc->global_env);
  nanoclj_cell_t * Number = mk_class(sc, "java.lang.Number", gentypeid(sc), sc->Object);
  sc->Throwable = mk_class(sc, "java.lang.Throwable", gentypeid(sc), sc->Object);
  nanoclj_cell_t * Exception = mk_class(sc, "java.lang.Exception", gentypeid(sc), sc->Throwable);
  nanoclj_cell_t * RuntimeException = mk_class(sc, "java.lang.RuntimeException", T_RUNTIME_EXCEPTION, Exception);
  nanoclj_cell_t * IllegalArgumentException = mk_class(sc, "java.lang.IllegalArgumentException", T_ILLEGAL_ARG_EXCEPTION, RuntimeException);
  nanoclj_cell_t * NumberFormatException = mk_class(sc, "java.lang.NumberFormatException", T_NUM_FMT_EXCEPTION, IllegalArgumentException);
  nanoclj_cell_t * OutOfMemoryError = mk_class(sc, "java.lang.OutOfMemoryError", gentypeid(sc), sc->Throwable);
  nanoclj_cell_t * NullPointerException = mk_class(sc, "java.lang.NullPointerException", gentypeid(sc), RuntimeException);
  nanoclj_cell_t * ClassCastException = mk_class(sc, "java.lang.ClassCastException", T_CLASS_CAST_EXCEPTION, RuntimeException);
  nanoclj_cell_t * IllegalStateException = mk_class(sc, "java.lang.IllegalStateException", T_ILLEGAL_STATE_EXCEPTION, RuntimeException);
  nanoclj_cell_t * AFn = mk_class(sc, "clojure.lang.AFn", gentypeid(sc), sc->Object);  
  
  mk_class(sc, "java.lang.Class", T_CLASS, AFn); /* non-standard parent */
  mk_class(sc, "java.lang.String", T_STRING, sc->Object);
  mk_class(sc, "java.lang.Boolean", T_BOOLEAN, sc->Object);
  mk_class(sc, "java.math.BigInteger", T_BIGINT, Number);
  mk_class(sc, "java.lang.Double", T_REAL, Number);
  mk_class(sc, "java.lang.Long", T_LONG, Number);
  mk_class(sc, "java.io.Reader", T_READER, sc->Object);
  mk_class(sc, "java.io.Writer", T_WRITER, sc->Object);
  mk_class(sc, "java.io.InputStream", T_INPUT_STREAM, sc->Object);
  mk_class(sc, "java.io.OutputStream", T_OUTPUT_STREAM, sc->Object);
  mk_class(sc, "java.util.zip.GZIPInputStream", T_GZIP_INPUT_STREAM, sc->Object);
  mk_class(sc, "java.util.zip.GZIPOutputStream", T_GZIP_OUTPUT_STREAM, sc->Object);
  mk_class(sc, "java.io.File", T_FILE, sc->Object);
  mk_class(sc, "java.util.Date", T_DATE, sc->Object);
  mk_class(sc, "java.util.UUID", T_UUID, sc->Object);
  mk_class(sc, "java.util.regex.Pattern", T_REGEX, sc->Object);
  mk_class(sc, "java.lang.ArithmeticException", T_ARITHMETIC_EXCEPTION, RuntimeException);
  
  /* Clojure types */
  nanoclj_cell_t * AReference = mk_class(sc, "clojure.lang.AReference", gentypeid(sc), sc->Object);
  nanoclj_cell_t * Obj = mk_class(sc, "clojure.lang.Obj", gentypeid(sc), sc->Object);
  nanoclj_cell_t * ASeq = mk_class(sc, "clojure.lang.ASeq", gentypeid(sc), Obj);
  nanoclj_cell_t * PersistentVector = mk_class(sc, "clojure.lang.PersistentVector", T_VECTOR, AFn);
  
  mk_class(sc, "clojure.lang.PersistentTreeSet", T_SORTED_SET, AFn);
  mk_class(sc, "clojure.lang.PersistentArrayMap", T_ARRAYMAP, AFn);
  mk_class(sc, "clojure.lang.Symbol", T_SYMBOL, AFn);  
  mk_class(sc, "clojure.lang.Keyword", T_KEYWORD, AFn); /* non-standard parent */
  mk_class(sc, "clojure.lang.BigInt", T_BIGINT, Number);
  mk_class(sc, "clojure.lang.Ratio", T_RATIO, Number);
  mk_class(sc, "clojure.lang.Delay", T_DELAY, sc->Object);
  mk_class(sc, "clojure.lang.LazySeq", T_LAZYSEQ, Obj);
  mk_class(sc, "clojure.lang.Cons", T_LIST, ASeq);
  mk_class(sc, "clojure.lang.Namespace", T_ENVIRONMENT, AReference);
  mk_class(sc, "clojure.lang.Var", T_VAR, AReference);
  mk_class(sc, "clojure.lang.MapEntry", T_MAPENTRY, PersistentVector);
  nanoclj_cell_t * PersistentQueue = mk_class(sc, "clojure.lang.PersistentQueue", T_QUEUE, Obj);
  mk_class(sc, "clojure.lang.ArityException", T_ARITY_EXCEPTION, Exception);
  
  /* nanoclj types */
  nanoclj_cell_t * Closure = mk_class(sc, "nanoclj.core.Closure", T_CLOSURE, AFn);
  mk_class(sc, "nanoclj.core.Procedure", T_PROC, AFn);
  mk_class(sc, "nanoclj.core.Macro", T_MACRO, Closure);
  mk_class(sc, "nanoclj.core.ForeignFunction", T_FOREIGN_FUNCTION, AFn);
  mk_class(sc, "nanoclj.core.ForeignObject", T_FOREIGN_OBJECT, AFn);

  mk_class(sc, "nanoclj.core.Codepoint", T_CODEPOINT, sc->Object);
  mk_class(sc, "nanoclj.core.CharArray", T_CHAR_ARRAY, sc->Object);
  sc->Image = mk_class(sc, "nanoclj.core.Image", T_IMAGE, sc->Object);
  sc->Audio = mk_class(sc, "nanoclj.core.Audio", T_AUDIO, sc->Object);
  mk_class(sc, "nanoclj.core.Tensor", T_TENSOR, sc->Object);
  mk_class(sc, "nanoclj.core.EmptyList", T_NIL, sc->Object);
  sc->Graph = mk_class(sc, "nanoclj.core.Graph", T_GRAPH, sc->Object);

  sc->OutOfMemoryError = mk_exception(sc, OutOfMemoryError, "Out of memory");
  sc->NullPointerException = mk_exception(sc, NullPointerException, "Null pointer exception");

  nanoclj_val_t EMPTY = def_symbol(sc, "EMPTY");
  intern(sc, PersistentQueue, EMPTY, mk_pointer(mk_collection(sc, T_QUEUE, NULL)));
  
  intern(sc, sc->global_env, sc->MOUSE_POS, mk_nil());

  register_functions(sc);

  sc->window_scale_factor = 1.0;
  sc->window_lines = sc->window_columns = 0;

  nanoclj_termdata_t td = get_termdata(stdin, stdout);
  sc->fg_color = td.fg;
  sc->bg_color = td.bg;
  sc->term_graphics = td.gfx;
  sc->term_colors = td.color;
  
  sc->context->properties = mk_properties(sc);
  sc->tensor_ctx = mk_tensor_context();
  
  if (sc->bg_color.red < 128 && sc->bg_color.green < 128 && sc->bg_color.blue < 128) {
    intern(sc, sc->global_env, sc->THEME, def_keyword(sc, "dark"));
  } else {
    intern(sc, sc->global_env, sc->THEME, def_keyword(sc, "bright"));
  }
  
  return sc->pending_exception == NULL;
}

void nanoclj_set_input_port_file(nanoclj_t * sc, FILE * fin) {
  intern(sc, sc->global_env, sc->IN_SYM, port_rep_from_file(sc, T_READER, fin, NULL));
}

void nanoclj_set_output_port_file(nanoclj_t * sc, FILE * fout) {
  nanoclj_val_t p = port_rep_from_file(sc, T_WRITER, fout, NULL);
  intern(sc, sc->root_env, sc->OUT_SYM, p);
  update_window_info(sc, decode_pointer(p));
}

void nanoclj_set_output_port_callback(nanoclj_t * sc,
				      void (*text) (const char *, size_t, void *),
				      void (*color) (double, double, double, void *),
				      void (*restore) (void *),
				      void (*image) (imageview_t, void *)
				      ) {
  nanoclj_val_t p = mk_writer_from_callback(sc, text, color, restore, image);
  intern(sc, sc->root_env, sc->OUT_SYM, p);
  intern(sc, sc->root_env, sc->WINDOW_SIZE, mk_nil());
}

void nanoclj_set_error_port_callback(nanoclj_t * sc, void (*text) (const char *, size_t, void *)) {
  nanoclj_val_t p = mk_writer_from_callback(sc, text, NULL, NULL, NULL);
  intern(sc, sc->global_env, sc->ERR, p);
}

void nanoclj_set_error_port_file(nanoclj_t * sc, FILE * fout) {
  intern(sc, sc->global_env, sc->ERR, port_rep_from_file(sc, T_WRITER, fout, NULL));
}

void nanoclj_set_external_data(nanoclj_t * sc, void *p) {
  sc->ext_data = p;
}

void nanoclj_set_object_invoke_callback(nanoclj_t * sc, nanoclj_val_t (*func) (nanoclj_t *, void *, nanoclj_cell_t *)) {
  sc->object_invoke_callback = func;
}

void nanoclj_deinit(nanoclj_t * sc) {
  sc->oblist = NULL;
  sc->root_env = sc->global_env = NULL;
  dump_stack_free(sc);
  sc->envir = NULL;
  sc->code = mk_nil();
  sc->args = NULL;
  sc->value = mk_nil();
#ifdef USE_RECUR_REGISTER
  sc->recur = mk_nil();
#endif
  sc->save_inport = mk_nil();
  
  sc->load_stack->size = 0;

  gc(sc, NULL, NULL, NULL);
}

void nanoclj_deinit_shared(nanoclj_t * sc, nanoclj_shared_context_t * ctx) {
  nanoclj_mutex_destroy(&(ctx->mutex));
  for (int i = 0; i <= sc->context->last_cell_seg; i++) {
    sc->free(ctx->alloc_seg[i]);
  }
  ctx->last_cell_seg = -1;
  sc->free(ctx);
  sc->context = NULL;
}

bool nanoclj_load_named_file(nanoclj_t * sc, const char *filename) {
  dump_stack_reset(sc);

  nanoclj_val_t p = port_from_filename(sc, T_READER, mk_strview(filename));

  sc->envir = sc->global_env;
  valarray_push(sc, sc->load_stack, p);
  sc->args = NULL;
  
  Eval_Cycle(sc, OP_T0LVL);

  return sc->pending_exception == NULL;
}

nanoclj_val_t nanoclj_eval_string(nanoclj_t * sc, const char *cmd, size_t len) {
  dump_stack_reset(sc);

  nanoclj_val_t p = port_from_string(sc, T_READER, (strview_t){ cmd, len });
  
  sc->envir = sc->global_env;
  valarray_push(sc, sc->load_stack, p);
  sc->args = NULL;
  
  Eval_Cycle(sc, OP_T0LVL);

  return sc->value;
}

/* "func" and "args" are assumed to be already eval'ed. */
nanoclj_val_t nanoclj_call(nanoclj_t * sc, nanoclj_val_t func, nanoclj_val_t args) {
  save_from_C_call(sc);
  sc->envir = sc->global_env;
  sc->args = seq(sc, decode_pointer(args));
  sc->code = func;
  Eval_Cycle(sc, OP_APPLY);  
  return sc->value;
}

#if !NANOCLJ_STANDALONE
void nanoclj_register_foreign_func(nanoclj_t * sc, nanoclj_registerable * sr) {
  intern(sc, sc->global_env, def_symbol(sc, sr->name), mk_foreign_func(sc, sr->f, 0, -1));
}

void nanoclj_register_foreign_func_list(nanoclj_t * sc,
    nanoclj_registerable * list, int count) {
  int i;
  for (i = 0; i < count; i++) {
    nanoclj_register_foreign_func(sc, list + i);
  }
}
#endif

nanoclj_val_t nanoclj_eval(nanoclj_t * sc, nanoclj_val_t obj) {
  return eval(sc, decode_pointer(obj));
}

/* ========== Main ========== */

#if NANOCLJ_STANDALONE

int main(int argc, const char **argv) {
  if (argc == 2 && strcmp(argv[1], "--help") == 0) {
    printf("nanoclj %s\n", NANOCLJ_VERSION);
    printf("Usage: %s [switches] [--] [programfile] [arguments]\n", argv[0]);
    return 1;
  }
  if (argc == 2 && strcmp(argv[1], "--legal") == 0) {
    legal();
    return 1;
  }
  
  http_init();

  nanoclj_t sc;
  if (!nanoclj_init(&sc)) {
    fprintf(stderr, "Could not initialize!\n");
    return 2;
  }
  
  nanoclj_set_input_port_file(&sc, stdin);
  nanoclj_set_output_port_file(&sc, stdout);
  nanoclj_set_error_port_file(&sc, stderr);
#if USE_DL
  intern(&sc, sc.global_env, def_symbol(&sc, "load-extension"),
	 mk_foreign_func(&sc, scm_load_ext, 0, -1));
#endif

  nanoclj_cell_t * args = NULL;
  for (int i = argc - 1; i >= 2; i--) {
    nanoclj_val_t value = mk_string(&sc, argv[i]);
    args = cons(&sc, value, args);
  }
  intern(&sc, sc.global_env, def_symbol(&sc, "*command-line-args*"), mk_pointer(args));
  
  if (nanoclj_load_named_file(&sc, InitFile)) {
    if (argc >= 2) {
      if (nanoclj_load_named_file(&sc, argv[1])) {
	/* Call -main if it exists */
	nanoclj_val_t main = def_symbol(&sc, "-main");
	nanoclj_cell_t * x = find_slot_in_env(&sc, sc.envir, main, true);
	if (x) {
	  nanoclj_val_t v = slot_value_in_env(x);
	  nanoclj_call(&sc, v, sc.EMPTY);
	}
      }
    } else {
      const char * expr = "(clojure.repl/repl)";
      nanoclj_eval_string(&sc, expr, strlen(expr));
    }
  }

  int rv = 0;
  if (sc.pending_exception) {
    nanoclj_val_t name_v;
    if (get_elem(&sc, _cons_metadata(get_type_object(&sc, mk_pointer(sc.pending_exception))), sc.NAME, &name_v)) {
      strview_t sv = to_strview(name_v);
      fprintf(stderr, "%.*s\n", (int)sv.size, sv.ptr);
    } else {
      fprintf(stderr, "Unknown exception\n");
    }
    strview_t sv = to_strview(_car(sc.pending_exception));
    fprintf(stderr, "%.*s\n", (int)sv.size, sv.ptr);
    rv = 1;
  }

  nanoclj_deinit(&sc);
  nanoclj_deinit_shared(&sc, sc.context);

  return rv;
}

#endif
