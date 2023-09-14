/* This version of nanoclj was branched at 21.03 from TinyScheme R7
 * which was branched at 1.42 from https://sourceforge.net/p/tinyscheme
 * There was reference to authors of MiniScheme from which TinyScheme
 * was created long ago, but nowadays it is somewhat misleading.
 * Many people put efforts to this thing, so please look at the
 * comprehensive list of contributors here:
 * http://tinyscheme.sourceforge.net/credits.html
 */

#define _NANOCLJ_SOURCE
#include "nanoclj_priv.h"

// #define VECTOR_ARGS
// #define RETAIN_ALLOCS 1

#ifdef _WIN32

#if defined(_MSC_VER) && _MSC_VER < 1900
#define snprintf _snprintf  // Microsoft headers use underscores in some names
#endif

#else /* _WIN32 */

#include <unistd.h>

#endif /* _WIN32 */

#if USE_DL
#include "dynload.h"
#endif

#include <math.h>
#include <limits.h>
#include <ctype.h>
#include <stdarg.h>
#include <assert.h>
#include <signal.h>
#include <pthread.h>
#include <sys/utsname.h>
#include <pwd.h>

#include <utf8proc.h>
#include <sixel/sixel.h>

#include "clock.h"
#include "murmur3.h"
#include "legal.h"
#include "term.h"
#include "nanoclj_utf8.h"

#define kNIL		  UINT64_C(18444492273895866368)
#define kTRUE		  UINT64_C(9221683186994511873)
#define kFALSE		  UINT64_C(9221683186994511872)

/* Masks for NaN packing */
#define MASK_SIGN         0x8000000000000000
#define MASK_EXPONENT     0x7ff0000000000000
#define MASK_QUIET        0x0008000000000000
#define MASK_TYPE         0x0007000000000000
#define MASK_SIGNATURE    0xffff000000000000
#define MASK_PAYLOAD_PTR  0x0000ffffffffffff
#define MASK_PAYLOAD_INT  0x00000000ffffffff

/* Masks for primitive types */
#define MASK_TYPE_NAN     0x0000000000000000
#define MASK_TYPE_TYPE    0x0001000000000000
#define MASK_TYPE_BOOLEAN 0x0002000000000000
#define MASK_TYPE_INTEGER 0x0003000000000000
#define MASK_TYPE_CHAR    0x0004000000000000
#define MASK_TYPE_PROC    0x0005000000000000
#define MASK_TYPE_KEYWORD 0x0006000000000000
#define MASK_TYPE_SYMBOL  0x0007000000000000

/* Constant short encoded values */
#define kNaN   (MASK_EXPONENT | MASK_QUIET)

/* Signatures for primitive types */
#define SIGNATURE_NAN     kNaN
// #define SIGNATURE_TYPE    (kNaN | MASK_TYPE_TYPE)
#define SIGNATURE_BOOLEAN (kNaN | MASK_TYPE_BOOLEAN)
#define SIGNATURE_INTEGER (kNaN | MASK_TYPE_INTEGER)
#define SIGNATURE_CHAR    (kNaN | MASK_TYPE_CHAR)
#define SIGNATURE_PROC    (kNaN | MASK_TYPE_PROC)
#define SIGNATURE_KEYWORD (kNaN | MASK_TYPE_KEYWORD)
#define SIGNATURE_SYMBOL  (kNaN | MASK_TYPE_SYMBOL)
#define SIGNATURE_POINTER (kNaN | MASK_SIGN)

/* Parsing tokens */
#define TOK_EOF		(-1)
#define TOK_LPAREN  	0
#define TOK_RPAREN  	1
#define TOK_RCURLY  	2
#define TOK_RSQUARE 	3
#define TOK_DOT     	4
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

#define BACKQUOTE '`'
#define DELIMITERS  "()[]{}\";\f\t\v\n\r, "

/*
 *  Basic memory allocation units
 */

#define OBJ_LIST_SIZE 727

#define VERSION "0.2.0"

#include <string.h>
#include <stdlib.h>

#define str_eq(X,Y) (!strcmp((X),(Y)))

#ifndef InitFile
#define InitFile "init.clj"
#endif

#ifndef FIRST_CELLSEGS
#define FIRST_CELLSEGS 3
#endif

#ifndef STRBUFF_INITIAL_SIZE
#define STRBUFF_INITIAL_SIZE 128
#endif
#ifndef STRBUFF_MAX_SIZE
#define STRBUFF_MAX_SIZE 65536
#endif
#ifndef AUXBUFF_SIZE
#define AUXBUFF_SIZE 256
#endif

enum nanoclj_types {
  T_CELL = -1,
  T_NIL = 0,
  T_BOOLEAN = 1,
  T_STRING = 2,
  T_INTEGER = 3,
  T_LONG = 4,
  T_REAL = 5,
  T_SYMBOL = 6,
  T_PROC = 7,
  T_LIST = 8,
  T_CLOSURE = 9,
  T_EXCEPTION = 10,
  T_FOREIGN_FUNCTION = 11,
  T_CHARACTER = 12,
  T_READER = 13,
  T_WRITER = 14,
  T_VECTOR = 15,
  T_MACRO = 16,
  T_LAZYSEQ = 17,
  T_ENVIRONMENT = 18,
  T_CLASS = 19,
  T_KEYWORD = 20,
  T_MAPENTRY = 21,
  T_ARRAYMAP = 22,
  T_SORTED_SET = 23,
  T_VAR = 24,
  T_FOREIGN_OBJECT = 25,
  T_BIGINT = 26,
  T_CHAR_ARRAY = 27,
  T_INPUT_STREAM = 28,
  T_OUTPUT_STREAM = 29,
  T_RATIO = 30,
  T_DELAY = 31,
  T_IMAGE = 32,
  T_CANVAS = 33,
  T_AUDIO = 34,
  T_FILE = 35,
  T_MAX_TYPE
};

typedef struct {
  const char * ptr;
  size_t size;
} strview_t;

typedef struct {
  int_fast16_t syntax;
  const char * name;
  size_t name_size;
  uint32_t hash;
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

#define T_TYPE	       256	/* 0000000100000000 */
#define T_DYNAMIC      512      /* 0000001000000000 */
#define T_SEQUENCE    1024	/* 0000010000000000 */
#define T_REALIZED    2048	/* 0000100000000000 */
#define T_REVERSE     4096      /* 0001000000000000 */
#define T_SMALL       8192      /* 0010000000000000 */
#define T_GC_ATOM    16384      /* 0100000000000000 */    /* only for gc */
#define CLR_GC_ATOM  49151      /* 1011111111111111 */    /* only for gc */
#define MARK         32768      /* 1000000000000000 */
#define UNMARK       32767      /* 0111111111111111 */

static int_fast16_t prim_type(nanoclj_val_t value) {
  uint64_t signature = value.as_long & MASK_SIGNATURE;
  
  /* Short encoded types */
  switch (signature) {
  case SIGNATURE_BOOLEAN: return T_BOOLEAN;
  case SIGNATURE_INTEGER: return T_INTEGER;
  case SIGNATURE_CHAR:    return T_CHARACTER;
  case SIGNATURE_PROC:    return T_PROC;
  case SIGNATURE_KEYWORD: return T_KEYWORD;
  case SIGNATURE_SYMBOL:  return T_SYMBOL;
  case SIGNATURE_POINTER: return T_CELL;
  }

  return T_REAL;
}

static inline bool is_real(nanoclj_val_t value) {
  return prim_type(value) == T_REAL;
}

static inline bool is_cell(nanoclj_val_t v) {
  return (v.as_long & MASK_SIGNATURE) == SIGNATURE_POINTER;
}

static inline bool is_primitive(nanoclj_val_t v) {
  return (v.as_long & MASK_SIGNATURE) != SIGNATURE_POINTER;
}

static inline bool is_symbol(nanoclj_val_t v) {
  return (v.as_long & MASK_SIGNATURE) == SIGNATURE_SYMBOL;
}

static inline bool is_keyword(nanoclj_val_t v) {
  return (v.as_long & MASK_SIGNATURE) == SIGNATURE_KEYWORD;
}

static inline bool is_boolean(nanoclj_val_t v) {
  return (v.as_long & MASK_SIGNATURE) == SIGNATURE_BOOLEAN;
}

static inline bool is_proc(nanoclj_val_t v) {
  return (v.as_long & MASK_SIGNATURE) == SIGNATURE_PROC;
}

static inline int_fast16_t _type(nanoclj_cell_t * a) {
  if (!a) return T_NIL;
  if (a->flags & T_TYPE) return T_CLASS;
  return a->type;
}

static inline nanoclj_cell_t * decode_pointer(nanoclj_val_t value) {
  return (nanoclj_cell_t *)(value.as_long & MASK_PAYLOAD_PTR);
}

static inline symbol_t * decode_symbol(nanoclj_val_t value) {
  return (symbol_t *)(value.as_long & MASK_PAYLOAD_PTR);
}

static inline int_fast16_t expand_type(nanoclj_val_t value, int_fast16_t primitive_type) {
  if (primitive_type == T_CELL) return _type(decode_pointer(value));
  return primitive_type;
}

static inline int_fast16_t type(nanoclj_val_t value) {
  int_fast16_t type = prim_type(value);
  if (type == T_CELL) {
    nanoclj_cell_t * c = decode_pointer(value);
    return _type(c);
  } else {
    return type;
  }
}

static inline bool is_list(nanoclj_val_t value) {
  if (!is_cell(value)) return false;
  nanoclj_cell_t * c = decode_pointer(value);
  uint64_t t = _type(c);
  return t == T_LIST;
}

static inline nanoclj_val_t mk_pointer(nanoclj_cell_t * ptr) {
  nanoclj_val_t v;
  v.as_long = SIGNATURE_POINTER | (uint64_t)ptr;
  return v;
}

static inline nanoclj_val_t mk_keyword(nanoclj_t * sc, const char * name, size_t size) {
  /* This is leaked intentionally for now */
  char * str = (char*)sc->malloc(size + 1);
  memcpy(str, name, size);
  str[size] = 0;

  nanoclj_val_t v;
  v.as_long = SIGNATURE_KEYWORD | (uint64_t)str;
  return v;
}

static inline nanoclj_val_t mk_nil() {
  nanoclj_val_t v;
  v.as_long = kNIL;
  return v;
}

static inline uint32_t hash_fn(const char *key, size_t key_len) {
  uint32_t hashed = 0;
  uint32_t bits_per_int = sizeof(unsigned int) * 8;

  for (size_t i = 0; i < key_len; i++) {    
    /* letters have about 5 bits in them */
    hashed = (hashed << 5) | (hashed >> (bits_per_int - 5));
    hashed ^= key[i];
  }
  return hashed;
}

static inline nanoclj_val_t mk_symbol(nanoclj_t * sc, strview_t sv, int_fast16_t syntax) {
  /* mk_symbol allocations are leaked intentionally for now */
  char * str = (char*)sc->malloc(sv.size + 1);
  memcpy(str, sv.ptr, sv.size);
  str[sv.size] = 0;

  symbol_t * s = sc->malloc(sizeof(symbol_t));
  s->name = str;
  s->name_size = sv.size;
  s->syntax = syntax;
  s->hash = hash_fn(sv.ptr, sv.size);
  
  nanoclj_val_t v;
  v.as_long = SIGNATURE_SYMBOL | (uint64_t)s;
  return v;
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

#define _is_realized(p)           (p->flags & T_REALIZED)
#define _set_realized(p)          (p->flags |= T_REALIZED)
#define _is_mark(p)               (p->flags & MARK)
#define _setmark(p)               (p->flags |= MARK)
#define _clrmark(p)               (p->flags &= UNMARK)
#define _is_reverse(p)		  (p->flags & T_REVERSE)
#define _is_sequence(p)		  (p->flags & T_SEQUENCE)
#define _set_sequence(p)	  (p->flags |= T_SEQUENCE)
#define _is_small(p)	          (p->flags & T_SMALL)
#define _set_gc_atom(p)           (p->flags |= T_GC_ATOM)
#define _clr_gc_atom(p)           (p->flags &= CLR_GC_ATOM)

#define _offset_unchecked(p)	  ((p)->_object._collection.offset)
#define _size_unchecked(p)	  ((p)->_object._collection.size)
#define _vec_store_unchecked(p)	  ((p)->_object._collection._store._vec)
#define _str_store_unchecked(p)	  ((p)->_object._collection._store._str)
#define _smalldata_unchecked(p)   (&((p)->_object._small_collection.data[0]))

#define _port_unchecked(p)	  ((p)->_object._port)
#define _image_unchecked(p)	  ((p)->_object._image)
#define _audio_unchecked(p)	  ((p)->_object._audio)
#define _canvas_unchecked(p)	  ((p)->_object._canvas)
#define _lvalue_unchecked(p)      ((p)->_object._lvalue)
#define _ff_unchecked(p)	  ((p)->_object._ff.ptr)
#define _fo_unchecked(p)	  ((p)->_object._fo.ptr)
#define _min_arity_unchecked(p)	  ((p)->_object._ff.min_arity)
#define _max_arity_unchecked(p)	  ((p)->_object._ff.max_arity)

#define _smallstrvalue_unchecked(p)      (&((p)->_object._small_string.data[0]))
#define _sosize_unchecked(p)      ((p)->so_size)

/* macros for cell operations */
#define cell_type(p)      	  (decode_pointer(p)->type)
#define cell_flags(p)      	  (decode_pointer(p)->flags)
#define is_small(p)	 	  (cell_flags(p)&T_SMALL)
#define is_gc_atom(p)             (decode_pointer(p)->flags & T_GC_ATOM)

#define smallstrvalue_unchecked(p)      (_smallstrvalue_unchecked(decode_pointer(p)))
#define sosize_unchecked(p)       (_sosize_unchecked(decode_pointer(p)))

#define offset_unchecked(p)	  (_offset_unchecked(decode_pointer(p)))
#define size_unchecked(p)	  (_size_unchecked(decode_pointer(p)))
#define vec_store_unchecked(p)	  (_vec_store_unchecked(decode_pointer(p)))
#define str_store_unchecked(p)	  (_str_store_unchecked(decode_pointer(p)))
#define lvalue_unchecked(p)       (_lvalue_unchecked(decode_pointer(p)))
#define port_unchecked(p)	  (_port_unchecked(decode_pointer(p)))
#define image_unchecked(p)	  (_image_unchecked(decode_pointer(p)))
#define audio_unchecked(p)	  (_audio_unchecked(decode_pointer(p)))
#define canvas_unchecked(p)	  (_canvas_unchecked(decode_pointer(p)))

static inline int32_t decode_integer(nanoclj_val_t value) {
  return (int32_t)value.as_long & MASK_PAYLOAD_INT;
}

static inline bool is_string(nanoclj_val_t p) {
  if (!is_cell(p)) return false;
  nanoclj_cell_t * c = decode_pointer(p);
  return _type(c) == T_STRING;
}

static inline bool is_char_array(nanoclj_val_t p) {
  if (!is_cell(p)) return false;
  nanoclj_cell_t * c = decode_pointer(p);
  return _type(c) == T_CHAR_ARRAY;
}
static inline bool is_vector(nanoclj_val_t p) {
  if (!is_cell(p)) return false;
  nanoclj_cell_t * c = decode_pointer(p);
  return _type(c) == T_VECTOR;
}
static inline bool is_image(nanoclj_val_t p) {
  if (!is_cell(p)) return false;
  nanoclj_cell_t * c = decode_pointer(p);
  return _type(c) == T_IMAGE;
}
static inline bool is_canvas(nanoclj_val_t p) {
  if (!is_cell(p)) return false;
  nanoclj_cell_t * c = decode_pointer(p);
  return _type(c) == T_CANVAS;  
}
static inline bool is_exception(nanoclj_val_t p) {
  if (!is_cell(p)) return false;
  nanoclj_cell_t * c = decode_pointer(p);
  return _type(c) == T_EXCEPTION;
}

static inline bool is_seqable_type(int_fast16_t type_id) {
  switch (type_id) {
  case T_NIL:
  case T_LIST:
  case T_LAZYSEQ:
  case T_STRING:
  case T_CHAR_ARRAY:
  case T_FILE:
  case T_READER:
  case T_VECTOR:
  case T_ARRAYMAP:
  case T_SORTED_SET:
  case T_CLASS:
    return true;
  }
  return false;
}

static inline bool is_coll_type(int_fast16_t type_id) {
  switch (type_id) {
  case T_LIST:
  case T_VECTOR:
  case T_MAPENTRY:
  case T_ARRAYMAP:
  case T_SORTED_SET:
  case T_LAZYSEQ:
    return true;
  }
  return false;
}

static inline bool is_coll(nanoclj_val_t p) {
  if (!is_cell(p)) return false;
  nanoclj_cell_t * c = decode_pointer(p);
  return is_coll_type(_type(c));  
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
static inline size_t _get_size(nanoclj_cell_t * c) {
  return _is_small(c) ? _sosize_unchecked(c) : _size_unchecked(c);
}

static inline size_t get_size(nanoclj_val_t p) {
  if (!is_cell(p)) {
    return 0;
  } else {
    return _get_size(decode_pointer(p));
  }
}

static void fill_vector(nanoclj_cell_t * vec, nanoclj_val_t elem) {
  size_t num = _get_size(vec);
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
    if (_is_small(c)) {
      return _so_vector_metadata(c);
    }
  case T_LIST:
  case T_CLOSURE:
  case T_MACRO:
  case T_CLASS:
  case T_ENVIRONMENT:
    return _cons_metadata(c);
  }
  return NULL;
}

static inline void set_metadata(nanoclj_cell_t * c, nanoclj_cell_t * meta) {
  switch (_type(c)) {
  case T_VECTOR:
  case T_ARRAYMAP:
  case T_SORTED_SET:
  case T_VAR:
    if (_is_small(c)) {
      _so_vector_metadata(c) = meta;
    }
    break;
  case T_LIST:
  case T_CLOSURE:
  case T_MACRO:
    _cons_metadata(c) = meta;
    break;
  }
}

static inline bool is_integral_type(int_fast16_t type) {
  switch (type) {
  case T_BOOLEAN:
  case T_INTEGER:
  case T_CHARACTER:
  case T_PROC:
    return true;
  }
  return false;
}

static inline bool is_number(nanoclj_val_t p) {
  switch (type(p)) {
  case T_INTEGER:
  case T_LONG:
  case T_REAL:
  case T_RATIO:
  case T_BIGINT:
    return true;
  default:
    return false;
  }
}
static inline bool is_integer(nanoclj_val_t p) {
  switch (type(p)) {
  case T_INTEGER:
  case T_LONG:
    return true;
  default:
    return false;
  }
}

static inline char * _strvalue(nanoclj_cell_t * s) {
  if (_is_small(s)) {
    return _smallstrvalue_unchecked(s);
  } else {
    return _str_store_unchecked(s)->data + _offset_unchecked(s);
  }
}

static inline const char * strvalue(nanoclj_val_t p) {
  if (is_cell(p)) {
    nanoclj_cell_t * c = decode_pointer(p);
    if (c && (_type(c) == T_STRING || _type(c) == T_CHAR_ARRAY || _type(c) == T_FILE)) {
      return _strvalue(c);
    }
  }
  return "";
}

static inline long long to_long_w_def(nanoclj_val_t p, long long def) {
  switch (prim_type(p)) {
  case T_INTEGER:
  case T_PROC:
  case T_CHARACTER:
    return decode_integer(p);
  case T_REAL:
    return (long long)p.as_double;
  case T_CELL:
    {
      nanoclj_cell_t * c = decode_pointer(p);
      switch (_type(c)) {
      case T_LONG:
	return _lvalue_unchecked(c);
      case T_RATIO:
	return to_long_w_def(vector_elem(c, 0), 0) / to_long_w_def(vector_elem(c, 1), 1);
      case T_VECTOR:
      case T_SORTED_SET:
      case T_ARRAYMAP:
	return _get_size(c);
      case T_CLASS:
	return c->type;
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
  case T_CHARACTER:
  case T_PROC:
  case T_INTEGER:
    return decode_integer(p);
  case T_REAL:
    return (int32_t)p.as_double;
  case T_CELL:
    {
      nanoclj_cell_t * c = decode_pointer(p);
      switch (_type(c)) {
      case T_LONG:
	return _lvalue_unchecked(c);
      case T_RATIO:
	return to_long(vector_elem(c, 0)) / to_long(vector_elem(c, 1));
      case T_VECTOR:
      case T_SORTED_SET:
      case T_ARRAYMAP:
	return _get_size(c);
      case T_CLASS:
	return c->type;
      }
    }     
  }
  return 0;
}

static inline double to_double(nanoclj_val_t p) {
  switch (prim_type(p)) {
  case T_INTEGER:
    return (double)decode_integer(p);
  case T_REAL:
    return p.as_double;
  case T_CELL: {
    nanoclj_cell_t * c = decode_pointer(p);
    switch (_type(c)) {
    case T_LONG: return (double)lvalue_unchecked(p);
    case T_RATIO: return to_double(vector_elem(c, 0)) / to_double(vector_elem(c, 1));
    }
  }
  }
  return NAN;
}

static inline bool is_reader(nanoclj_val_t p) {
  if (!is_cell(p)) return false;
  nanoclj_cell_t * c = decode_pointer(p);
  return _type(c) == T_READER;
}
static inline bool is_writer(nanoclj_val_t p) {
  if (!is_cell(p)) return false;
  nanoclj_cell_t * c = decode_pointer(p);
  return _type(c) == T_WRITER;
}
static inline bool is_binary(nanoclj_port_t * pt) {
  return pt->kind & port_binary;
}

static inline bool is_mapentry(nanoclj_val_t p) {
  if (!is_cell(p)) return false;
  nanoclj_cell_t * c = decode_pointer(p);
  return _type(c) == T_MAPENTRY;
}

static inline bool is_foreign_function(nanoclj_val_t p) {
  if (!is_cell(p)) return false;
  nanoclj_cell_t * c = decode_pointer(p);
  return _type(c) == T_FOREIGN_FUNCTION;
}

static inline bool is_foreign_object(nanoclj_val_t p) {
  if (!is_cell(p)) return false;
  nanoclj_cell_t * c = decode_pointer(p);
  return _type(c) == T_FOREIGN_OBJECT;
}

static inline bool is_closure(nanoclj_val_t p) {
  if (!is_cell(p)) return false;
  nanoclj_cell_t * c = decode_pointer(p);
  return _type(c) == T_CLOSURE;
}
static inline bool is_macro(nanoclj_val_t p) {
  if (!is_cell(p)) return false;
  nanoclj_cell_t * c = decode_pointer(p);
  return _type(c) == T_MACRO;
}
static inline nanoclj_val_t closure_code(nanoclj_cell_t * p) {
  return _car(p);
}
static inline nanoclj_cell_t * closure_env(nanoclj_cell_t * p) {
  return decode_pointer(_cdr(p));
}

static inline bool is_lazyseq(nanoclj_val_t p) {
  if (!is_cell(p)) return false;
  nanoclj_cell_t * c = decode_pointer(p);
  return _type(c) == T_LAZYSEQ;
}

static inline bool is_delay(nanoclj_val_t p) {
  if (!is_cell(p)) return false;
  nanoclj_cell_t * c = decode_pointer(p);
  return _type(c) == T_DELAY;
}

static inline bool is_environment(nanoclj_val_t p) {
  if (!is_cell(p)) return false;
  nanoclj_cell_t * c = decode_pointer(p);
  return _type(c) == T_ENVIRONMENT || _type(c) == T_CLASS;
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

static void putstr(nanoclj_t * sc, const char *s, nanoclj_val_t port);
static nanoclj_val_t get_err_port(nanoclj_t * sc);
static nanoclj_val_t get_in_port(nanoclj_t * sc);
static void Eval_Cycle(nanoclj_t * sc, enum nanoclj_opcodes op);

static inline const char* get_version() {
  return VERSION;
}

static inline strview_t _to_strview(nanoclj_cell_t * c) {
  switch (_type(c)) {
  case T_STRING:
  case T_CHAR_ARRAY:
  case T_FILE:
    if (_is_small(c)) {
      return (strview_t){ _smallstrvalue_unchecked(c), _sosize_unchecked(c) };
    } else {
      return (strview_t){ _str_store_unchecked(c)->data + _offset_unchecked(c), _size_unchecked(c) };
    }
  case T_READER:{
    nanoclj_port_t * pt = _port_unchecked(c);
    if (pt->kind & port_string) {
      return (strview_t){
	pt->rep.string.data.data,
	pt->rep.string.data.size
      };
    }
  }
    break;
  case T_WRITER:{
    nanoclj_port_t * pt = _port_unchecked(c);
    if (pt->kind & port_string) {
      return (strview_t){
	pt->rep.string.data.data,
	pt->rep.string.curr - pt->rep.string.data.data
      };
    }
    break;
  }    
  }
  return (strview_t){ "", 0 };
}

static inline strview_t to_strview(nanoclj_val_t x) {
  switch (prim_type(x)) {
  case T_BOOLEAN:
    return decode_integer(x) == 0 ? (strview_t){ "false", 5 } : (strview_t){ "true", 4 };
    
  case T_SYMBOL:{
    symbol_t * s = decode_symbol(x);
    return (strview_t){ s->name, s->name_size };
  }
  case T_KEYWORD:{
    const char * s = (const char *)decode_pointer(x);
    return (strview_t){ s, strlen(s) };
  }
  case T_CELL:
    return _to_strview(decode_pointer(x));
  }  
  return (strview_t){ "", 0 };
}

static inline char * alloc_cstr(nanoclj_t * sc, strview_t sv) {
  char * s = (char *)sc->malloc(sv.size + 1);
  memcpy(s, sv.ptr, sv.size);
  s[sv.size] = 0;
  return s;
}

static inline int_fast16_t syntaxnum(nanoclj_val_t p) {
  if (is_symbol(p)) {
    symbol_t * s = decode_symbol(p);
    return s->syntax;
  }
  return 0;
}

/* allocate new cell segment */
static inline int alloc_cellseg(nanoclj_t * sc, int n) {
  nanoclj_val_t p;
  for (int k = 0; k < n; k++) {
    if (sc->last_cell_seg >= CELL_NSEGMENT - 1) {
      return k;
    }
    char * cp = (char *) sc->malloc(CELL_SEGSIZE * sizeof(nanoclj_cell_t));
    if (cp == 0) {
      return k;
    }
    long i = ++sc->last_cell_seg;
    sc->alloc_seg[i] = cp;    
    /* insert new segment in address order */
    nanoclj_val_t newp = mk_pointer((nanoclj_cell_t *)cp);
    sc->cell_seg[i] = newp;
    while (i > 0 && sc->cell_seg[i - 1].as_long > sc->cell_seg[i].as_long) {
      p = sc->cell_seg[i];
      sc->cell_seg[i] = sc->cell_seg[i - 1];
      sc->cell_seg[--i] = p;
    }
    sc->fcells += CELL_SEGSIZE;
    nanoclj_val_t last = mk_pointer(decode_pointer(newp) + CELL_SEGSIZE - 1);
    for (p = newp; p.as_long <= last.as_long; p = mk_pointer(decode_pointer(p) + 1)) {
      cell_type(p) = 0;
      cell_flags(p) = 0;
      set_cdr(p, mk_pointer(decode_pointer(p) + 1));
      set_car(p, sc->EMPTY);
    }
    /* insert new cells in address order on free list */
    if (sc->free_cell == &(sc->_EMPTY) || decode_pointer(p) < sc->free_cell) {
      set_cdr(last, mk_pointer(sc->free_cell));
      sc->free_cell = decode_pointer(newp);
    } else {
      nanoclj_cell_t * p = sc->free_cell;
      while (_cdr(p).as_long != sc->EMPTY.as_long && newp.as_long > _cdr(p).as_long)
        p = decode_pointer(_cdr(p));

      set_cdr(last, _cdr(p));
      _set_cdr(p, newp);
    }
  }

  return n;
}

static inline void port_close(nanoclj_t * sc, nanoclj_port_t * pt) {
  pt->kind &= ~port_input;
  pt->kind &= ~port_output;
  if (pt->kind & port_file) {
    /* Cleanup is here so (close-*-port) functions could work too */
    pt->rep.stdio.curr_line = 0;
    
    if (pt->rep.stdio.filename) {
      sc->free(pt->rep.stdio.filename);
    }
    FILE * fh = pt->rep.stdio.file;
    if (fh != stdout && fh != stderr && fh != stdin) {
      fclose(fh);
    }
  } else if (pt->kind & port_string) {
    sc->free(pt->rep.string.data.data);
  }
  pt->kind = port_free;
}

static inline void finalize_cell(nanoclj_t * sc, nanoclj_cell_t * a) {
  if (_is_small(a)) {
    return;
  }
  switch (_type(a)) {
  case T_STRING:
  case T_CHAR_ARRAY:
  case T_FILE:{
    nanoclj_byte_array_t * s = _str_store_unchecked(a);
    if (s) {
      s->refcnt--;
      if (s->refcnt == 0) {
	sc->free(s->data);
	sc->free(s);
      }
    }
  }
    break;
  case T_VECTOR:
  case T_ARRAYMAP:
  case T_SORTED_SET:{
    nanoclj_vector_t * s = _vec_store_unchecked(a);
    if (s) {
      s->refcnt--;
      if (s->refcnt == 0) {
	sc->free(s->data);
	sc->free(s);
      }
    }
  }
    break;
  case T_READER:
  case T_WRITER:{
    nanoclj_port_t * pt = _port_unchecked(a);
    port_close(sc, pt);    
    sc->free(pt);
  }
    break;
  case T_IMAGE:{
    nanoclj_image_t * image = _image_unchecked(a);
    sc->free(image->data);
    sc->free(image);
  }
    break;
  case T_AUDIO:{
    nanoclj_audio_t * audio = _audio_unchecked(a);
    sc->free(audio->data);
    sc->free(audio);
  }
    break;
  case T_CANVAS:
#if NANOCLJ_HAS_CANVAS
    finalize_canvas(sc, _canvas_unchecked(a));
    _canvas_unchecked(a) = NULL;
#endif
    break;
  }
}

#include "nanoclj_gc.h"

/* get new cell.  parameter a, b is marked by gc. */

static inline nanoclj_cell_t * get_cell_x(nanoclj_t * sc, uint32_t type_id, uint16_t flags, nanoclj_cell_t * a, nanoclj_cell_t * b, nanoclj_cell_t * c) {
  if (sc->no_memory) {
    return &(sc->_sink);
  }

  if (sc->free_cell == &(sc->_EMPTY)) {
    const int min_to_be_recovered = sc->last_cell_seg * 8;
    gc(sc, a, b, c);
    if (sc->fcells < min_to_be_recovered || sc->free_cell == &(sc->_EMPTY)) {
      /* if only a few recovered, get more to avoid fruitless gc's */
      if (!alloc_cellseg(sc, 1) && sc->free_cell == &(sc->_EMPTY)) {
        sc->no_memory = 1;
        return decode_pointer(sc->sink);
      }
    }
  }

  nanoclj_cell_t * x = sc->free_cell;
  sc->free_cell = decode_pointer(_cdr(x));
  --sc->fcells;

  x->type = type_id;
  x->flags = flags;
  
  return x;
}

/* To retain recent allocs before interpreter knows about them -
   Tehom */

static inline void retain(nanoclj_t * sc, nanoclj_cell_t * recent) {
  nanoclj_cell_t * holder = get_cell_x(sc, T_LIST, 0, recent, NULL, NULL);
  _set_car(holder, mk_pointer(recent));
  _set_cdr(holder, car(sc->sink));
  _cons_metadata(holder) = NULL;
  set_car(sc->sink, mk_pointer(holder));
}

static inline nanoclj_cell_t * get_cell(nanoclj_t * sc, uint32_t type, uint16_t flags,
					nanoclj_val_t head, nanoclj_cell_t * tail, nanoclj_cell_t * meta) {
  if (!tail) tail = &(sc->_EMPTY);
  nanoclj_cell_t * cell = get_cell_x(sc, type, flags, is_cell(head) ? decode_pointer(head) : NULL, tail, meta);
  _set_car(cell, head);
  _set_cdr(cell, mk_pointer(tail));
  _cons_metadata(cell) = meta;

#if RETAIN_ALLOCS
  retain(sc, cell);
#endif
  
  return cell;
}

static inline nanoclj_val_t mk_reader(nanoclj_t * sc, nanoclj_port_t * p) {
  nanoclj_cell_t * x = get_cell_x(sc, T_READER, T_GC_ATOM, NULL, NULL, NULL);
  _port_unchecked(x) = p;

#if RETAIN_ALLOCS
  retain(sc, x);
#endif

  return mk_pointer(x);
}

static inline nanoclj_val_t mk_writer(nanoclj_t * sc, nanoclj_port_t * p) {
  nanoclj_cell_t * x = get_cell_x(sc, T_WRITER, T_GC_ATOM, NULL, NULL, NULL);
  _port_unchecked(x) = p;

#if RETAIN_ALLOCS
  retain(sc, x);
#endif

  return mk_pointer(x);
}

static inline nanoclj_val_t mk_foreign_func(nanoclj_t * sc, foreign_func f) {
  nanoclj_cell_t * x = get_cell_x(sc, T_FOREIGN_FUNCTION, T_GC_ATOM, NULL, NULL, NULL);

  _ff_unchecked(x) = f;
  _min_arity_unchecked(x) = 0;
  _max_arity_unchecked(x) = 0x7fffffff;

#if RETAIN_ALLOCS
  retain(sc, x);
#endif

  return mk_pointer(x);
}

static inline nanoclj_val_t mk_foreign_func_with_arity(nanoclj_t * sc, foreign_func f, int min_arity, int max_arity) {
  nanoclj_cell_t * x = get_cell_x(sc, T_FOREIGN_FUNCTION, T_GC_ATOM, NULL, NULL, NULL);

  _ff_unchecked(x) = f;
  _min_arity_unchecked(x) = min_arity;
  _max_arity_unchecked(x) = max_arity;

#if RETAIN_ALLOCS
  retain(sc, x);
#endif

  return mk_pointer(x);
}

static inline nanoclj_val_t mk_foreign_object(nanoclj_t * sc, void * o) {
  nanoclj_cell_t * x = get_cell_x(sc, T_FOREIGN_OBJECT, T_GC_ATOM, NULL, NULL, NULL);

  _fo_unchecked(x) = o;
  _min_arity_unchecked(x) = 0;
  _max_arity_unchecked(x) = 0x7fffffff;
  
#if RETAIN_ALLOCS
  retain(sc, x);
#endif

  return mk_pointer(x);
}

static inline nanoclj_val_t mk_character(int c) {
  nanoclj_val_t v;
  v.as_long = SIGNATURE_CHAR | (uint32_t)c;
  return v;
}

static inline nanoclj_val_t mk_boolean(bool b) {
  nanoclj_val_t v;
  v.as_long = SIGNATURE_BOOLEAN | (b ? (uint32_t)1 : (uint32_t)0);
  return v;
}

static inline nanoclj_val_t mk_int(int num) {
  nanoclj_val_t v;
  v.as_long = SIGNATURE_INTEGER | (uint32_t)num;
  return v;
}

static inline nanoclj_val_t mk_proc(enum nanoclj_opcodes op) {
  nanoclj_val_t v;
  v.as_long = SIGNATURE_PROC | (uint32_t)op;
  return v;
}

static inline nanoclj_val_t mk_real(double n) {
  nanoclj_val_t v;
  if (isnan(n)) {
    /* Canonize all kinds of NaNs such as -NaN to ##NaN */
    v.as_double = NAN;
  } else {
    v.as_double = n;
  }
  return v;
}

static inline nanoclj_cell_t * mk_long(nanoclj_t * sc, long long num) {
  nanoclj_cell_t * x = get_cell_x(sc, T_LONG, T_GC_ATOM, NULL, NULL, NULL);
  _lvalue_unchecked(x) = num;

#if RETAIN_ALLOCS
  retain(sc, x);
#endif

  return x;
}

/* get number atom (integer) */
static inline nanoclj_val_t mk_integer(nanoclj_t * sc, long long num) {
  if (num >= INT_MIN && num <= INT_MAX) {
    return mk_int(num);
  } else {
    return mk_pointer(mk_long(sc, num));
  }
}

static inline nanoclj_val_t mk_image(nanoclj_t * sc, int32_t width, int32_t height,
				     int32_t channels, unsigned char * data) {
  size_t size = width * height * channels;
  unsigned char * r_data = sc->malloc(size);
  if (!r_data) {
    return sc->sink;
  }
  if (data) memcpy(r_data, data, size);

  nanoclj_image_t * image = (nanoclj_image_t*)sc->malloc(sizeof(nanoclj_image_t));
  if (!image) {
    /* TODO: free the previous allocations */
    return sc->sink;
  }
  image->data = r_data;
  image->width = width;
  image->height = height;
  image->channels = channels;
  
  nanoclj_cell_t * x = get_cell_x(sc, T_IMAGE, T_GC_ATOM, NULL, NULL, NULL);
  if (sc->no_memory) {
    /* TODO: free the previous allocations */
    return sc->sink;
  }
  _image_unchecked(x) = image;
  
#if RETAIN_ALLOCS
  retain(sc, x);
#endif

  return mk_pointer(x);
}

static inline nanoclj_val_t mk_audio(nanoclj_t * sc, size_t frames, int32_t channels,
				     int32_t sample_rate, float * data) {
  size_t size = frames * channels * sizeof(float);
  float * r_data = sc->malloc(size);
  if (!r_data) {
    return sc->sink;
  }
  if (data) memcpy(r_data, data, size);

  nanoclj_audio_t * audio = (nanoclj_audio_t*)sc->malloc(sizeof(nanoclj_audio_t));
  if (!audio) {
    /* TODO: release the previous allocation */
    return sc->sink;
  }
  audio->data = r_data;
  audio->frames = frames;
  audio->channels = channels;
  audio->sample_rate = sample_rate;
  
  nanoclj_cell_t * x = get_cell_x(sc, T_AUDIO, T_GC_ATOM, NULL, NULL, NULL);
  if (sc->no_memory) {
    /* TODO: release the previous allocation */
    return sc->sink;
  }
  _audio_unchecked(x) = audio;
  
#if RETAIN_ALLOCS
  retain(sc, x);
#endif

  return mk_pointer(x);
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
  case T_INTEGER:
    *den = 1;
    return decode_integer(n);
  case T_CELL:{
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

/* allocate name to string area */
static inline char *store_string(nanoclj_t * sc, size_t len, const char *str) {
  if (!len || !str) return NULL;
  char * q = (char *)sc->malloc(len + 1);
  if (q == 0) {
    sc->no_memory = 1;
    return sc->strbuff;
  }
  memcpy(q, str, len);
  q[len] = 0;
  return q;
}

static inline char * alloc_c_str(nanoclj_t * sc, strview_t sv) {
  char * buffer = (char*)sc->malloc(sv.size + 1);
  memcpy(buffer, sv.ptr, sv.size);
  buffer[sv.size] = 0;
  return buffer;
}

/* Creates a string store */
static inline nanoclj_byte_array_t * mk_string_store(nanoclj_t * sc, size_t len, size_t padding) {
  nanoclj_byte_array_t * s = (nanoclj_byte_array_t *)sc->malloc(sizeof(nanoclj_byte_array_t));
  s->data = (char *)sc->malloc((len + padding) * sizeof(char));
  s->size = len;
  s->reserved = len + padding;
  s->refcnt = 1;
  return s;
}

static inline nanoclj_cell_t * _get_string_object(nanoclj_t * sc, int32_t t, size_t offset, size_t len, nanoclj_byte_array_t * store) {
  nanoclj_cell_t * x = get_cell_x(sc, t, T_GC_ATOM, NULL, NULL, NULL);
  if (store) {
    _str_store_unchecked(x) = store;
    _size_unchecked(x) = len;
    _offset_unchecked(x) = offset;
  } else {
    x->flags |= T_SMALL;
    x->so_size = len;
  }
  
#if RETAIN_ALLOCS
  retain(sc, x);
#endif

  return x;
}

static inline nanoclj_cell_t * get_string_object(nanoclj_t * sc, int32_t t, const char *str, size_t len, size_t padding) {
  nanoclj_byte_array_t * s = 0;
  if (len + padding > NANOCLJ_SMALL_STR_SIZE) {
    s = mk_string_store(sc, len, padding);
  }
  nanoclj_cell_t * x = _get_string_object(sc, t, 0, len, s);
  if (str) {
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

static inline nanoclj_val_t mk_string_from_sv(nanoclj_t * sc, strview_t sv) {
  return mk_pointer(get_string_object(sc, T_STRING, sv.ptr, sv.size, 0));
}

static inline nanoclj_val_t mk_exception(nanoclj_t * sc, const char * text) {
  return mk_pointer(get_cell(sc, T_EXCEPTION, 0, mk_string(sc, text), NULL, NULL));
}

static inline int basic_inchar(nanoclj_port_t * pt) {
  if (pt->kind & port_file) {
    return fgetc(pt->rep.stdio.file);
  } else if (pt->kind & port_string) {
    if (pt->rep.string.curr == pt->rep.string.data.data + pt->rep.string.data.size) {
      return EOF;
    } else {
      return (unsigned char)(*pt->rep.string.curr++);
    }
  } else {
    return EOF;
  }
}

static inline int utf8_inchar(nanoclj_port_t *pt) {
  int c = basic_inchar(pt);
  int bytes, i;
  char buf[4];
  if (c == EOF || c < 0x80) {
    return c;
  }
  buf[0] = (char) c;
  bytes = (c < 0xE0) ? 2 : ((c < 0xF0) ? 3 : 4);
  for (i = 1; i < bytes; i++) {
    c = basic_inchar(pt);
    if (c == EOF) {
      return EOF;
    }
    buf[i] = (char) c;
  }
  return utf8_decode(buf);
}

/* get new character from input file */
static inline int inchar(nanoclj_port_t * pt) {
  int c;
  if (pt->backchar[1] >= 0) {
    c = pt->backchar[1];
    pt->backchar[1] = -1;
    return c;
  } else if (pt->backchar[0] >= 0) {
    c = pt->backchar[0];
    pt->backchar[0] = -1;
    return c;
  }
  if (pt->kind & port_saw_EOF) {
    return EOF;
  }
  if (is_binary(pt)) {
    c = basic_inchar(pt);
  } else {
    c = utf8_inchar(pt);
  }
  
  if (c == EOF) {
    /* Instead, set port_saw_EOF */
    pt->kind |= port_saw_EOF;
  } else if (c == '\n' && pt->kind & port_file) {
    pt->rep.stdio.curr_line++;
  }

  return c;
}

/* back character to input buffer */
static inline void backchar(int c, nanoclj_port_t * pt) {
  if (pt->backchar[0] == -1) {
    pt->backchar[0] = c;
  } else {
    pt->backchar[1] = c;
  }
}

/* Medium level cell allocation */

/* get new cons cell */
static inline nanoclj_cell_t * cons(nanoclj_t * sc, nanoclj_val_t head, nanoclj_cell_t * tail) {
  return get_cell(sc, T_LIST, 0, head, tail, NULL);
}

/* Creates a vector store with double the size requested */
static inline nanoclj_vector_t * mk_vector_store(nanoclj_t * sc, size_t len, size_t reserve) {
  nanoclj_vector_t * s = (nanoclj_vector_t *)malloc(sizeof(nanoclj_vector_t));
  s->data = (nanoclj_val_t*)sc->malloc(reserve * sizeof(nanoclj_val_t));
  s->size = len;
  s->reserved = reserve;
  s->refcnt = 1;
  return s;
}

static inline nanoclj_cell_t * _get_vector_object(nanoclj_t * sc, int_fast16_t t, size_t offset, size_t size, nanoclj_vector_t * store) {
  nanoclj_cell_t * main_cell = get_cell_x(sc, t, T_GC_ATOM, NULL, NULL, NULL);
  if (sc->no_memory) {
    /* TODO: free the store */
    return &(sc->_sink);
  }

  if (store) {
    main_cell->flags = T_GC_ATOM;
    _size_unchecked(main_cell) = size;
    _vec_store_unchecked(main_cell) = store;
    _offset_unchecked(main_cell) = offset;
  } else {
    main_cell->flags |= T_SMALL;
    main_cell->so_size = size;
  }
  
#if RETAIN_ALLOCS
  retain(sc, main_cell);
#endif

  return main_cell;
}

static inline nanoclj_cell_t * get_vector_object(nanoclj_t * sc, int_fast16_t t, size_t size) {
  nanoclj_vector_t * store = NULL;
  if (size > NANOCLJ_SMALL_VEC_SIZE) store = mk_vector_store(sc, size, size);
  return _get_vector_object(sc, t, 0, size, store);
}

static inline void resize_vector(nanoclj_t * sc, nanoclj_cell_t * vec, size_t new_size) {
  size_t old_size = _get_size(vec);
  if (new_size <= NANOCLJ_SMALL_VEC_SIZE) {
    if (!_is_small(vec)) {
      /* TODO: release the old store */
      vec->flags = T_SMALL | T_GC_ATOM;
    }
    vec->so_size = new_size;    
  } else {
    if (_is_small(vec)) {
      _vec_store_unchecked(vec) = mk_vector_store(sc, new_size, new_size);
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
}

static inline nanoclj_val_t mk_mapentry(nanoclj_t * sc, nanoclj_val_t key, nanoclj_val_t val) {
  nanoclj_cell_t * vec = get_vector_object(sc, T_MAPENTRY, 2);
  set_vector_elem(vec, 0, key);
  set_vector_elem(vec, 1, val);
  return mk_pointer(vec);
}

static inline nanoclj_val_t mk_ratio(nanoclj_t * sc, nanoclj_val_t num, nanoclj_val_t den) {  
  nanoclj_cell_t * vec = get_vector_object(sc, T_RATIO, 2);
  set_vector_elem(vec, 0, num);
  set_vector_elem(vec, 1, den);
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

  /* TODO: check for overflows and use BigInt if necessary */
  if (exponent < 0) {
    return mk_ratio_long(sc, numerator, 1ULL << -exponent);
  } else {
    return mk_integer(sc, numerator << exponent);
  }
}

static inline dump_stack_frame_t * s_add_frame(nanoclj_t * sc) {
  /* enough room for the next frame? */
  if (sc->dump >= sc->dump_size) {
    sc->dump_size *= 2;
    fprintf(stderr, "reallocing dump stack (%zu)\n", sc->dump_size);
    sc->dump_base = (dump_stack_frame_t *)sc->realloc(sc->dump_base, sizeof(dump_stack_frame_t) * sc->dump_size);
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
  sc->retcode = 0;

  Eval_Cycle(sc, OP_EVAL);
  return sc->value;
}

/* Sequence handling */

static inline nanoclj_cell_t * remove_prefix(nanoclj_t * sc, nanoclj_cell_t * str, size_t n) {
  const char * old_ptr = _strvalue(str);
  const char * new_ptr = old_ptr;
  const char * end_ptr = old_ptr + _get_size(str);
  while (n > 0 && new_ptr < end_ptr) {
    new_ptr = utf8_next(new_ptr);
    n--;
  }  
  size_t new_size = end_ptr - new_ptr;
  
  if (_is_small(str)) {
    return get_string_object(sc, str->type, new_ptr, new_size, 0);
  } else {
    size_t new_offset = new_ptr - old_ptr;
    nanoclj_byte_array_t * s = _str_store_unchecked(str);
    s->refcnt++;
    return _get_string_object(sc, str->type, _offset_unchecked(str) + new_offset, new_size, s);
  }
}

static inline nanoclj_cell_t * remove_suffix(nanoclj_t * sc, nanoclj_cell_t * str, size_t n) {
  const char * start = _strvalue(str);
  const char * end = start + _get_size(str);
  const char * new_ptr = end;
  while (n > 0 && new_ptr > start) {
    new_ptr = utf8_prev(new_ptr);
    n--;
  }
  if (_is_small(str)) {
    return get_string_object(sc, str->type, start, new_ptr - start, 0);
  } else {
    return str;
  }
}

static inline nanoclj_cell_t * subvec(nanoclj_t * sc, nanoclj_cell_t * vec, size_t offset, size_t len) {
  if (_is_small(vec)) {
    nanoclj_cell_t * new_vec = get_vector_object(sc, vec->type, len);
    for (size_t i = 0; i < len; i++) {
      set_vector_elem(new_vec, i, vector_elem(vec, offset + i));
    }
    return new_vec;
  } else {
    nanoclj_vector_t * s = _vec_store_unchecked(vec);
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

/* Creates a reverse sequence of a vector or string */
static inline nanoclj_cell_t * rseq(nanoclj_t * sc, nanoclj_cell_t * coll) {
  size_t size = _get_size(coll);
  bool is_string = _type(coll) == T_STRING || _type(coll) == T_CHAR_ARRAY || _type(coll) == T_FILE;
  if (!size) {
    return NULL;
  } else if (is_string) {
    if (_is_small(coll)) {
      nanoclj_cell_t * new_str = get_string_object(sc, coll->type, NULL, size, 0);
      _set_sequence(new_str);
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
    nanoclj_cell_t * new_vec = get_vector_object(sc, _type(coll) | T_SEQUENCE, size);
    for (size_t i = 0; i < size; i++) {
      set_vector_elem(new_vec, size - 1 - i, vector_elem(coll, i));
    }
    return new_vec;
  } else {
    nanoclj_vector_t * s = _vec_store_unchecked(coll);
    size_t old_size = _size_unchecked(coll);
    size_t old_offset = _offset_unchecked(coll);

    s->refcnt++;
    
    nanoclj_cell_t * new_vec = _get_vector_object(sc, _type(coll), old_offset, old_size, s);
    new_vec->flags |= T_SEQUENCE | T_REVERSE;
    return new_vec;
  }
}

/* Returns nil or not-empty sequence */
static inline nanoclj_cell_t * seq(nanoclj_t * sc, nanoclj_cell_t * coll) {
  if (!coll) {
    return NULL;
  }
  if (_type(coll) == T_LAZYSEQ) {
    if (!_is_realized(coll)) {
      nanoclj_cell_t * code = cons(sc, sc->DEREF, cons(sc, mk_pointer(coll), NULL));
      coll = decode_pointer(eval(sc, code));
    } else if (is_nil(_car(coll)) && is_nil(_cdr(coll))) {
      return NULL;
    }
  }

  if (_is_sequence(coll)) {
    return coll;
  }

  switch (_type(coll)) {
  case T_NIL:
  case T_LONG:
  case T_MAPENTRY:
  case T_CLOSURE:
  case T_MACRO:
    return NULL;
  case T_STRING:
  case T_CHAR_ARRAY:
  case T_FILE:
    if (_get_size(coll) == 0) {
      return NULL;
    } else {
      nanoclj_cell_t * s = remove_prefix(sc, coll, 0);
      _set_sequence(s);
      return s;
    }
    break;
  case T_VECTOR:
  case T_ARRAYMAP:
  case T_SORTED_SET:
    if (_get_size(coll) == 0) {
      return NULL;
    } else {
      nanoclj_cell_t * s = subvec(sc, coll, 0, _get_size(coll));
      _set_sequence(s);
      return s;
    }
  }  
  
  return coll;
}

static inline bool is_empty(nanoclj_t * sc, nanoclj_cell_t * coll) {
  if (!coll) {
    return true;
  } else if (_is_sequence(coll)) {
    return false;
  } else if (_type(coll) == T_LAZYSEQ) {
    if (!_is_realized(coll)) {
      nanoclj_cell_t * code = cons(sc, sc->DEREF, cons(sc, mk_pointer(coll), NULL));
      coll = decode_pointer(eval(sc, code));
      if (!coll) {
	return true;
      }
    } else if (is_nil(_car(coll)) && is_nil(_cdr(coll))) {
      return true;
    }
  }

  switch (_type(coll)) {
  case T_NIL:
    return true;
  case T_STRING:
  case T_CHAR_ARRAY:
  case T_FILE:
  case T_VECTOR:
  case T_ARRAYMAP:
  case T_SORTED_SET:
  case T_MAPENTRY:
  case T_RATIO:
    return _get_size(coll) == 0;
  }  
  
  return false;
}

static inline nanoclj_cell_t * rest(nanoclj_t * sc, nanoclj_cell_t * coll) {
  int typ = _type(coll);

  if (typ == T_LAZYSEQ) {
    if (!_is_realized(coll)) {
      nanoclj_cell_t * code = cons(sc, sc->DEREF, cons(sc, mk_pointer(coll), NULL));
      coll = decode_pointer(eval(sc, code));
    } else if (is_nil(_car(coll)) && is_nil(_cdr(coll))) {
      return &(sc->_EMPTY);
    }
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
    if (_get_size(coll) >= 2) {
      nanoclj_cell_t * s;
      if (_is_reverse(coll)) {
	s = subvec(sc, coll, 0, _get_size(coll) - 1);
      } else {
	s = subvec(sc, coll, 1, _get_size(coll) - 1);
      }
      _set_sequence(s);
      return s;
    }
    break;
  case T_STRING:
  case T_CHAR_ARRAY:
  case T_FILE:
    if (_get_size(coll) >= 2) {
      nanoclj_cell_t * s;
      if (_is_reverse(coll)) {
	s = remove_suffix(sc, coll, 1);
      } else {
	s = remove_prefix(sc, coll, 1);
      }
      _set_sequence(s);
      return s;
    }
    break;
  }

  return &(sc->_EMPTY);
}

static inline nanoclj_cell_t * next(nanoclj_t * sc, nanoclj_cell_t * coll) {
  nanoclj_cell_t * r = rest(sc, coll);
  if (_type(r) == T_LAZYSEQ) {
    if (!_is_realized(r)) {
      nanoclj_cell_t * code = cons(sc, sc->DEREF, cons(sc, mk_pointer(r), NULL));
      r = decode_pointer(eval(sc, code));
    } else if (is_nil(_car(r)) && is_nil(_cdr(r))) {
      return NULL;
    }
  }
  if (r == &(sc->_EMPTY)) {
    return NULL;
  }
  return r;
}

static inline nanoclj_val_t first(nanoclj_t * sc, nanoclj_cell_t * coll) {
  if (coll == NULL) {
    return mk_nil();
  }

  if (_type(coll) == T_LAZYSEQ) {
    if (!_is_realized(coll)) {
      nanoclj_cell_t * code = cons(sc, sc->DEREF, cons(sc, mk_pointer(coll), NULL));
      coll = decode_pointer(eval(sc, code));
    } else if (is_nil(_car(coll)) && is_nil(_cdr(coll))) {
      return mk_nil();
    }
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
    if (_get_size(coll) > 0) {
      if (_is_reverse(coll)) {
	return vector_elem(coll, _get_size(coll) - 1);
      } else {
	return vector_elem(coll, 0);
      }
    }
    break;
  case T_STRING:
  case T_CHAR_ARRAY:
  case T_FILE:
    if (_get_size(coll)) {
      if (_is_reverse(coll)) {
	const char * start = _strvalue(coll);
	const char * end = start + _get_size(coll);
	const char * last = utf8_prev(end);
	return mk_character(utf8_decode(last));
      } else {
	return mk_character(utf8_decode(_strvalue(coll)));
      }
    }
    break;
  }

  return mk_nil();
}

static nanoclj_val_t second(nanoclj_t * sc, nanoclj_cell_t * a) {
  if (a) {
    int t = _type(a);
    if (t == T_VECTOR || t == T_RATIO || t == T_ARRAYMAP || t == T_SORTED_SET || t == T_MAPENTRY || t == T_VAR) {
      if (_get_size(a)) {
	return vector_elem(a, 1);
      }
    } else if (t != T_NIL) {
      return first(sc, rest(sc, a));
    }
  }
  return mk_nil();
}

static inline bool equals(nanoclj_t * sc, nanoclj_val_t a0, nanoclj_val_t b0) {
  if (a0.as_long == b0.as_long) {
    return true;
  } else if (is_primitive(a0) && is_primitive(b0)) {
    return false;
  } else if (is_cell(a0) && is_cell(b0)) {
    nanoclj_cell_t * a = decode_pointer(a0), * b = decode_pointer(b0);
    int t_a = _type(a), t_b = _type(b);
    if (t_a == t_b) {
      switch (t_a) {
      case T_STRING:
      case T_CHAR_ARRAY:
      case T_FILE:{
	strview_t sv1 = _to_strview(a), sv2 = _to_strview(b);
	return sv1.size == sv2.size && strncmp(sv1.ptr, sv2.ptr, sv1.size) == 0;
      }
      case T_LONG:
	return _lvalue_unchecked(a) == _lvalue_unchecked(b);
      case T_VECTOR:
      case T_SORTED_SET:
      case T_RATIO:
      case T_VAR:
      case T_MAPENTRY:{
	size_t l = _get_size(a);
	if (l == _get_size(b)) {
	  for (size_t i = 0; i < l; i++) {
	    if (!equals(sc, vector_elem(a, i), vector_elem(b, i))) {
	      return 0;
	    }
	  }
	  return 1;
	}
      }
      case T_ARRAYMAP:{
	size_t l = _get_size(a);
	if (l == _get_size(b)) {
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
	// case T_CLASS:
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
  } else {
    int t_a = type(a0), t_b = type(b0);
    if ((t_a == T_INTEGER && t_b == T_LONG) || (t_a == T_LONG && t_b == T_INTEGER)) {
      return to_long(a0) == to_long(b0);
    }
  }
  
  return false;
}

static inline bool get_elem(nanoclj_t * sc, nanoclj_cell_t * coll, nanoclj_val_t key, nanoclj_val_t * result) {
  if (!coll) {
    return false;
  }
  long long index;
  switch (_type(coll)) {
  case T_VECTOR:
  case T_RATIO:
  case T_MAPENTRY:
  case T_VAR:
    index = to_long_w_def(key, -1);
    if (index >= 0 && index < _get_size(coll)) {
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
      const char * end = str + _get_size(coll);
      
      for (; index > 0 && str < end; index--) {
	str = utf8_next(str);
      }
      
      if (index == 0 && str < end) {
	if (result) *result = mk_character(utf8_decode(str));
	return true;
      }
    }
    break;
        
  case T_ARRAYMAP:{
    size_t size = _get_size(coll);
    for (int i = 0; i < size; i++) {
      nanoclj_cell_t * entry = decode_pointer(vector_elem(coll, i));
      if (equals(sc, key, vector_elem(entry, 0))) {
	if (result) *result = vector_elem(entry, 1);
	return true;
      }    
    }
  }
    break;

  case T_SORTED_SET:{
    size_t size = _get_size(coll);
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
    } else if (key.as_long == sc->CHANNELS.as_long) {
      if (result) *result = mk_int(image->channels);
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
  }
  return false;
}

/* =================== Canvas ====================== */

#ifdef WIN32
#include "nanoclj_gdi.h"
#else
#include "nanoclj_cairo.h"
#endif

/* =================== HTTP ======================== */

#include "nanoclj_curl.h"

/* ========== Environment implementation  ========== */

/*
 * In this implementation, each frame of the environment may be
 * a hash table: a vector of alists hashed by variable name.
 * In practice, we use a vector only for the initial frame;
 * subsequent frames are too small and transient for the lookup
 * speed to out-weigh the cost of making a new vector.
 */

static inline void new_frame_in_env(nanoclj_t * sc, nanoclj_cell_t * old_env) {
  nanoclj_val_t new_frame;

  /* The interaction-environment has about 300 variables in it. */
  if (!old_env) {
    nanoclj_cell_t * vec = get_vector_object(sc, T_VECTOR, OBJ_LIST_SIZE);
    fill_vector(vec, mk_nil());
    new_frame = mk_pointer(vec);
  } else {
    new_frame = sc->EMPTY;
  }

  sc->envir = get_cell(sc, T_ENVIRONMENT, 0, new_frame, old_env, NULL);
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
    if (_get_size(vec) == 0) resize_vector(sc, vec, OBJ_LIST_SIZE);
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
  if (a.as_long == kNaN || b.as_long == kNaN) {
    return false;
  } else if (a.as_long == b.as_long) {
    return true;
  }
  int type_a = prim_type(a), type_b = prim_type(b);
  if (type_a == T_REAL || type_b == T_REAL) {
    return to_double(a) == to_double(b);
  } else if (type_a != T_CELL && type_b != T_CELL) {
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
  }
  int type_a = prim_type(a), type_b = prim_type(b);
  if (is_nil(a)) {
    return -1;
  } else if (is_nil(b)) {
    return +1;
  } else if (type_a == T_REAL || type_b == T_REAL) {
    double ra = to_double(a), rb = to_double(b);
    if (ra < rb) return -1;
    else if (ra > rb) return +1;
    return 0;
  } else if (is_integral_type(type_a) && is_integral_type(type_b)) {
    int ia = decode_integer(a), ib = decode_integer(b);
    if (ia < ib) return -1;
    else if (ia > ib) return +1;
    return 0;
  } else if (type_a == T_KEYWORD && type_b == T_KEYWORD) {
    return strcmp((const char *)decode_pointer(a), (const char *)decode_pointer(b));
  } else if (type_a == T_SYMBOL && type_b == T_SYMBOL) {
    return strcmp(decode_symbol(a)->name, decode_symbol(b)->name);
  } else if (type_a == T_CELL && type_b == T_CELL) {
    nanoclj_cell_t * a2 = decode_pointer(a), * b2 = decode_pointer(b);
    type_a = _type(a2);
    type_b = _type(b2);
    if (type_a == T_NIL) {
      return -1;
    } else if (type_b == T_NIL) {
      return +1;
    } else if (type_a == type_b) {
      switch (type_a) {
      case T_STRING:
      case T_CHAR_ARRAY:
      case T_FILE:{
	strview_t sv1 = _to_strview(a2), sv2 = _to_strview(b2);
	const char * p1 = sv1.ptr, * p2 = sv2.ptr;
	const char * end1 = sv1.ptr + sv1.size, * end2 = sv2.ptr + sv2.size;
	while ( 1 ) {
	  if (p1 >= end1 && p2 >= end2) return 0;
	  else if (p1 >= end1) return -1;
	  else if (p2 >= end2) return 1;
	  int d = utf8_decode(p1) - utf8_decode(p2);
	  if (d) return d;
	  p1 = utf8_next(p1), p2 = utf8_next(p2);
	}
      }
      case T_LONG:{
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
      case T_VAR:{
	size_t la = _get_size(a2), lb = _get_size(b2);
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
	size_t la = _get_size(a2), lb = _get_size(b2);
	if (la < lb) return -1;
	else if (la > lb) return +1;
	else {
	  /* TODO: implement map comparison */
	  return 0;
	}
      }
	
      case T_LIST:
      case T_CLOSURE:
      case T_LAZYSEQ:
      case T_MACRO:
      case T_ENVIRONMENT:
	// case T_CLASS:
	{
	int r = compare(_car(a2), _car(b2));
	if (r) return r;
	return compare(_cdr(a2), _cdr(b2));
      }
      }
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
  return 0;
}

static inline int _compare(const void * a, const void * b) {
  return compare(*(const nanoclj_val_t*)a, *(const nanoclj_val_t*)b);
}

static inline void sort_vector_in_place(nanoclj_cell_t * vec) {
  size_t s = _get_size(vec);
  if (s > 0) {
    if (_is_small(vec)) {
      qsort(_smalldata_unchecked(vec), s, sizeof(nanoclj_val_t), _compare);
    } else {
      qsort(_vec_store_unchecked(vec)->data + _offset_unchecked(vec), s, sizeof(nanoclj_val_t), _compare);
    }
  }
}

static int hasheq(nanoclj_val_t v) { 
  switch (prim_type(v)) {
  case T_BOOLEAN:
  case T_CHARACTER:
  case T_PROC:
    return murmur3_hash_int(decode_integer(v));
    
  case T_INTEGER:
    /* Use long hash for integers so that (hash 1) matches Clojure */
    return murmur3_hash_long(decode_integer(v));
    
  case T_CELL:{
    nanoclj_cell_t * c = decode_pointer(v);
    if (!c) return 0;
    
    switch (_type(c)) {
    case T_LONG:
      return murmur3_hash_long(_lvalue_unchecked(c));

    case T_STRING:
    case T_CHAR_ARRAY:
    case T_FILE:{
      strview_t sv = _to_strview(c);
      int hashcode = 0, m = 1;
      for (int i = (int)sv.size - 1; i >= 0; i--) {
	hashcode += sv.ptr[i] * m;
	m *= 31;
      }
      return murmur3_hash_int(hashcode);
    }

    case T_RATIO: /* Is this correct? */
    case T_MAPENTRY:
    case T_VECTOR:
    case T_SORTED_SET:
    case T_VAR:{
      uint32_t hash = 1;
      size_t n = _get_size(c);
      for (size_t i = 0; i < n; i++) {
	hash = 31 * hash + (uint32_t)hasheq(vector_elem(c, i));	
      }
      return murmur3_hash_coll(hash, n);
    }

    case T_ARRAYMAP:{
      uint32_t hash = 0;
      size_t n = _get_size(c);
      for (size_t i = 0; i < n; i++) {
	hash += (uint32_t)hasheq(vector_elem(c, i));
      }
      return murmur3_hash_coll(hash, n);
    }

    case T_LIST:{
      uint32_t hash = 31 + hasheq(_car(c));
      size_t n = 1;
      for ( nanoclj_val_t x = _cdr(c) ; is_list(x); x = cdr(x), n++) {
	hash = 31 * hash + (uint32_t)hasheq(car(x));
      }
      return murmur3_hash_coll(hash, n);
    }
}
  }
  }
  return 0;
}

static inline void ok_to_freely_gc(nanoclj_t * sc) {
  set_car(sc->sink, sc->EMPTY);
}

/* ========== oblist implementation  ========== */

static inline nanoclj_cell_t * oblist_initial_value(nanoclj_t * sc) {
  nanoclj_cell_t * vec = get_vector_object(sc, T_VECTOR, OBJ_LIST_SIZE);
  fill_vector(vec, mk_nil());
  return vec;
}

static inline nanoclj_val_t oblist_add_item(nanoclj_t * sc, strview_t sv, nanoclj_val_t v) {
  int location = hash_fn(sv.ptr, sv.size) % _size_unchecked(sc->oblist);
  set_vector_elem(sc->oblist, location, mk_pointer(cons(sc, v, decode_pointer(vector_elem(sc->oblist, location)))));
  return v;
}

static inline nanoclj_val_t oblist_find_item(nanoclj_t * sc, uint32_t type, const char *name, size_t len) {
  int location = hash_fn(name, len) % _size_unchecked(sc->oblist);
  nanoclj_val_t x = vector_elem(sc->oblist, location);
  if (!is_nil(x)) {
    for (; x.as_long != sc->EMPTY.as_long; x = cdr(x)) {
      nanoclj_val_t y = car(x);
      int ty = prim_type(y);
      if (type == ty) {
	if (ty == T_SYMBOL) {
	  symbol_t * s = decode_symbol(y);
	  if (len == s->name_size && strncmp(name, s->name, len) == 0) {
	    return y;
	  }
	} else {
	  strview_t sv = to_strview(y);
	  if (sv.size == len && strncmp(name, sv.ptr, len) == 0) {
	    return y;
	  }
	}
      }
    }
  }
  return mk_nil();
}

static inline nanoclj_val_t mk_vector(nanoclj_t * sc, size_t len) {
  return mk_pointer(get_vector_object(sc, T_VECTOR, len));
}

static inline nanoclj_val_t mk_vector_2d(nanoclj_t * sc, double a, double b) {
  nanoclj_cell_t * vec = get_vector_object(sc, T_VECTOR, 2);
  set_vector_elem(vec, 0, mk_real(a));
  set_vector_elem(vec, 1, mk_real(b));
  return mk_pointer(vec);
}

/* Copies a vector so that the copy can be mutated */
static inline nanoclj_cell_t * copy_vector(nanoclj_t * sc, nanoclj_cell_t * vec) {
  size_t len = _get_size(vec);
  if (_is_small(vec)) {
    nanoclj_cell_t * new_vec = _get_vector_object(sc, _type(vec), 0, len, NULL);
    nanoclj_val_t * data = _smalldata_unchecked(vec);
    nanoclj_val_t * new_data = _smalldata_unchecked(new_vec);
    memcpy(new_data, data, len * sizeof(nanoclj_val_t));
    return new_vec;
  } else {
    nanoclj_vector_t * s = mk_vector_store(sc, len, 2 * len);
    memcpy(s->data, _vec_store_unchecked(vec)->data + _offset_unchecked(vec), len * sizeof(nanoclj_val_t));
    return _get_vector_object(sc, _type(vec), 0, len, s);
  }
}

static inline size_t char_to_utf8(int c, char *p) {
  if (c == EOF) {
    return 0;
  } else {
    if (c < 0 || c > 0x10FFFF) {
      fprintf(stderr, "char_to_utf8: bad char %d\n", c);
      c = '?';
    }

    unsigned char *s = (unsigned char *)p;
 
    if (c < 0x80) {
      *s++ = (unsigned char) c;
      return 1;
    } else {
      size_t bytes = (c < 0x800) ? 2 : ((c < 0x10000) ? 3 : 4);
      size_t len = bytes;
      s[0] = 0x80;
      while (--bytes) {
	s[0] |= (0x80 >> bytes);
	s[bytes] = (unsigned char) (0x80 | (c & 0x3F));
	c >>= 6;
      }
      s[0] |= (unsigned char) c;
      return len;
    }
  }
}

static inline nanoclj_cell_t * conjoin(nanoclj_t * sc, nanoclj_cell_t * coll, nanoclj_val_t new_value) {
  if (!coll) coll = &(sc->_EMPTY);
	       
  int t = _type(coll);
  if (_is_sequence(coll) || t == T_NIL || t == T_LIST || t == T_LAZYSEQ) {
    return get_cell(sc, T_LIST, 0, new_value, coll, NULL);
  } else if (t == T_VECTOR || t == T_ARRAYMAP || t == T_SORTED_SET) {
    size_t old_size = _get_size(coll);
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
      nanoclj_vector_t * s = _vec_store_unchecked(coll);
      size_t old_offset = _offset_unchecked(coll);
    
      if (!(old_offset + old_size == s->size && s->size < s->reserved)) {
	nanoclj_vector_t * old_s = s;
	s = mk_vector_store(sc, old_size, 2 * old_size);
	memcpy(s->data, old_s->data + old_offset, old_size * sizeof(nanoclj_val_t));    
      } else {
	s->refcnt++;
      }
      
      s->data[s->size++] = new_value;
      return _get_vector_object(sc, t, old_offset, s->size, s);
    }
  } else if (t == T_STRING || t == T_CHAR_ARRAY || t == T_FILE) {
    size_t old_size = _get_size(coll);
    size_t input_len = char_to_utf8(decode_integer(new_value), sc->strbuff);
    if (_is_small(coll)) {
      nanoclj_cell_t * new_coll = get_string_object(sc, t, NULL, old_size + input_len, 0);
      const char * data = _smallstrvalue_unchecked(coll);
      char * new_data;
      if (_is_small(new_coll)) {
	new_data = _smallstrvalue_unchecked(new_coll);
      } else {
	new_data = _str_store_unchecked(new_coll)->data;
      }
      memcpy(new_data, data, old_size * sizeof(char));
      memcpy(new_data + old_size, sc->strbuff, input_len);
      return new_coll;
    } else {
      nanoclj_byte_array_t * s = _str_store_unchecked(coll);
      size_t old_offset = _offset_unchecked(coll);
      if (!(old_offset + old_size == s->size && s->size + input_len <= s->reserved)) {
	nanoclj_byte_array_t * old_s = s;
	s = mk_string_store(sc, old_size, old_size);
	memcpy(s->data, old_s->data + old_offset, old_size);
      } else {
	s->refcnt++;
      }

      memcpy(s->data + s->size, sc->strbuff, input_len);
      s->size += input_len;
      
      return _get_string_object(sc, t, old_offset, old_size + input_len, s);
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

/* get new keyword */
static inline nanoclj_val_t def_keyword_from_sv(nanoclj_t * sc, strview_t sv) {
  /* first check oblist */
  nanoclj_val_t x = oblist_find_item(sc, T_KEYWORD, sv.ptr, sv.size);
  if (!is_nil(x)) {
    return x;
  } else {
    return oblist_add_item(sc, sv, mk_keyword(sc, sv.ptr, sv.size));    
  }
}

/* get new symbol */
static inline nanoclj_val_t def_symbol_from_sv(nanoclj_t * sc, strview_t sv) {
  /* first check oblist */
  nanoclj_val_t x = oblist_find_item(sc, T_SYMBOL, sv.ptr, sv.size);
  if (!is_nil(x)) {
    return x;
  } else {
    return oblist_add_item(sc, sv, mk_symbol(sc, sv, 0));
  }
}

static inline nanoclj_val_t def_keyword(nanoclj_t * sc, const char *name) {
  return def_keyword_from_sv(sc, (strview_t){ name, strlen(name) });
}

static inline nanoclj_val_t def_symbol(nanoclj_t * sc, const char *name) {
  return def_symbol_from_sv(sc, (strview_t){ name, strlen(name) });
}

static inline nanoclj_val_t intern(nanoclj_t * sc, nanoclj_cell_t * envir, nanoclj_val_t symbol, nanoclj_val_t value) {
  nanoclj_cell_t * var = find_slot_in_env(sc, envir, symbol, false);
  if (var) set_slot_in_env(sc, var, value, NULL);
  else var = new_slot_spec_in_env(sc, envir, symbol, value, NULL);
  return mk_pointer(var);
}

static inline nanoclj_val_t intern_symbol(nanoclj_t * sc, nanoclj_cell_t * envir, nanoclj_val_t symbol) {
  nanoclj_cell_t * var = find_slot_in_env(sc, envir, symbol, false);
  if (!var) var = new_slot_spec_in_env(sc, envir, symbol, mk_nil(), NULL);
  return mk_pointer(var);
}

static inline nanoclj_cell_t * mk_type(nanoclj_t * sc, int type_id, nanoclj_cell_t * parent_type, nanoclj_cell_t * meta) {
  if (sc->types->size <= (size_t)type_id) {
    for (size_t i = sc->types->size; i < (size_t)type_id; i++) {
      sc->types->data[i] = mk_nil();
    }
    sc->types->size = type_id + 1;
  }
  nanoclj_cell_t * vec = get_vector_object(sc, T_VECTOR, OBJ_LIST_SIZE);
  fill_vector(vec, mk_nil());
  nanoclj_cell_t * t = get_cell(sc, type_id, T_TYPE, mk_pointer(vec), parent_type, meta);
  sc->types->data[type_id] = mk_pointer(t);
  return t;
}

static inline nanoclj_cell_t * mk_named_type(nanoclj_t * sc, const char * name, int type_id, nanoclj_cell_t * parent_type) {
  nanoclj_cell_t * md = mk_arraymap(sc, 1);
  set_vector_elem(md, 0, mk_mapentry(sc, sc->NAME, mk_string(sc, name)));
  nanoclj_cell_t * t = mk_type(sc, type_id, parent_type, md);
  intern(sc, sc->global_env, def_symbol(sc, name), mk_pointer(t));
  return t;
}

/* get new symbol or keyword. % is aliased to %1 */
static inline nanoclj_val_t def_symbol_or_keyword(nanoclj_t * sc, const char *name) {
  if (name[0] == ':') return def_keyword(sc, name + 1);
  else if (name[0] == '%' && name[1] == 0) return def_symbol(sc, "%1");
  else return def_symbol(sc, name);
}

static inline nanoclj_cell_t * def_namespace_with_sym(nanoclj_t *sc, nanoclj_val_t sym) {
  nanoclj_cell_t * md = mk_arraymap(sc, 1);
  set_vector_elem(md, 0, mk_mapentry(sc, sc->NAME, mk_string_from_sv(sc, to_strview(sym))));
  nanoclj_cell_t * vec = get_vector_object(sc, T_VECTOR, OBJ_LIST_SIZE);
  fill_vector(vec, mk_nil());

  nanoclj_cell_t * ns = get_cell(sc, T_ENVIRONMENT, 0, mk_pointer(vec), sc->root_env, md);
  intern(sc, sc->global_env, sym, mk_pointer(ns));
  return ns;
}

static inline nanoclj_cell_t * def_namespace(nanoclj_t * sc, const char *name) {
  return def_namespace_with_sym(sc, def_symbol(sc, name));
}

static inline nanoclj_val_t gensym(nanoclj_t * sc, const char * prefix, size_t prefix_len) {
  nanoclj_val_t x;
  char name[256];

  for (; sc->gensym_cnt < LONG_MAX; sc->gensym_cnt++) {
    size_t len = snprintf(name, 256, "%.*s%ld", (int)prefix_len, prefix, sc->gensym_cnt);
    
    /* first check oblist */
    x = oblist_find_item(sc, T_SYMBOL, name, len);
    if (!is_nil(x)) {
      continue;
    } else {
      strview_t sv = { name, len };
      return oblist_add_item(sc, sv, mk_symbol(sc, sv, 0));
    }
  }

  return mk_nil();
}

static inline int gentypeid(nanoclj_t * sc) {
  return sc->gentypeid_cnt++;
}

/* make symbol or number literal from string */
static inline nanoclj_val_t mk_primitive(nanoclj_t * sc, char *q) {
  bool has_dec_point = false;
  bool has_fp_exp = false;
  bool has_hex_prefix = false;
  bool has_octal_prefix = false;
  char *div = 0;
  char *p;
  int sign = 1;
  
  /* Parse namespace qualifier such as Math/sin */
  if (!isdigit(q[0]) && q[0] != '-' && (p = strchr(q, '/')) != 0 && p != q) {
    *p = 0;
    return mk_pointer(cons(sc, sc->SLASH_HOOK,
			   cons(sc,
				mk_pointer(cons(sc,
						sc->QUOTE,
						cons(sc, mk_primitive(sc, p + 1), NULL))),
				cons(sc, def_symbol(sc, q), NULL))));
  }

  p = q;
  char c = *p++;
  if ((c == '+') || (c == '-')) {
    if (c == '-') sign = -1;
    c = *p++;
    if (c == '.') {
      has_dec_point = true;
      c = *p++;
    }
    if (!isdigit(c)) {
      return def_symbol_or_keyword(sc, q);
    }
  } else if (c == '.') {
    has_dec_point = true;
    c = *p++;
    if (!isdigit(c)) {
      return def_symbol_or_keyword(sc, q);
    }
  } else if (!isdigit(c)) {
    return def_symbol_or_keyword(sc, q);
  }

  if (!has_dec_point && c == '0') {
    if (*p == 'x') {
      has_hex_prefix = true;
      p++;
      q = p;
    } else if (isdigit(*p)) {
      has_octal_prefix = true;
      q = p;
    }
  }

  for (; (c = *p) != 0; ++p) {
    if (has_hex_prefix) {
      if (isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) {
	continue;
      } else {
	return mk_nil();
      }
    } else if (has_octal_prefix) {
      if (c >= '0' && c <= '7') {
	continue;
      } else {
	return mk_nil();
      }      
    } else if (!isdigit(c)) {
      if (c == '.') {
        if (!has_dec_point) {
          has_dec_point = 1;
          continue;
        }
      } else if (c == '/') {
	if (!div) {
	  div = p;
	  continue;
	}
      } else if ((c == 'e') || (c == 'E')) {
        if (!has_fp_exp) {
          has_fp_exp = 1;
          has_dec_point = 1;    /* decimal point illegal further */
          p++;
          if ((*p == '-') || (*p == '+') || isdigit(*p)) {
            continue;
          }
        }
      }
      return def_symbol_or_keyword(sc, q);
    }
  }
  if (div) {
    *div = 0;
    long long num = atoll(q), den = atoll(div + 1);
    if (den != 0) {
      num = normalize(atoll(q), atoll(div + 1), &den);
    } else if (num != 0) {
      num = num < 0 ? -1 : +1;
    }
    if (den == 1) {
      return mk_integer(sc, num);
    } else {
      return mk_ratio_long(sc, num, den);
    }
  }
  if (has_hex_prefix) {
    return mk_integer(sc, sign * strtoll(q, (char **)NULL, 16));
  }
  if (has_octal_prefix) {
    return mk_integer(sc, sign * strtoll(q, (char **)NULL, 8));
  }
  if (has_dec_point) {
    return mk_real(atof(q));
  }
  return mk_integer(sc, atoll(q));
}

static inline nanoclj_val_t mk_char_const(nanoclj_t * sc, char *name) {
  if (*name == '\\') {   /* \w (character) */
    int c = 0;
    if (str_eq(name + 1, "space")) {
      c = ' ';
    } else if (str_eq(name + 1, "newline")) {
      c = '\n';
    } else if (str_eq(name + 1, "return")) {
      c = '\r';
    } else if (str_eq(name + 1, "tab")) {
      c = '\t';
    } else if (str_eq(name + 1, "formfeed")) {
      c = '\f';
    } else if (str_eq(name + 1, "backspace")) {
      c = '\b';
    } else if (name[1] == 'u' && name[2] != 0) {
      int c1 = 0;
      if (sscanf(name + 2, "%x", (unsigned int *) &c1) == 1) {
        c = c1;
      } else {
        return mk_nil();
      }   
    } else if (name[2] == 0) {
      c = name[1];
    } else if (*(utf8_next(name + 1)) == 0) {
      c = utf8_decode(name + 1);
    } else {
      return mk_nil();
    }
    return mk_character(c);
  } else {
    return mk_nil();
  }
}

static inline nanoclj_val_t mk_sharp_const(nanoclj_t * sc, char *name) {
  if (str_eq(name, "Inf")) {
    return mk_real(INFINITY);
  } else if (str_eq(name, "-Inf")) {
    return mk_real(-INFINITY);
  } else if (str_eq(name, "NaN")) {
    return mk_real(NAN);
  } else if (str_eq(name, "Eof")) {
    return mk_character(EOF);
  } else {
    return mk_nil();
  }
}

/* ========== Routines for Reading ========== */

static inline bool file_push(nanoclj_t * sc, strview_t sv) {
  if (sc->file_i == MAXFIL - 1) {
    return 0;
  }
  char * filename = alloc_c_str(sc, sv);
  FILE * fh = fopen(filename, "r");
  
  if (fh) {
    sc->file_i++;
    sc->load_stack[sc->file_i].kind = port_file | port_input;
    sc->load_stack[sc->file_i].backchar[0] = sc->load_stack[sc->file_i].backchar[1] = -1;
    sc->load_stack[sc->file_i].rep.stdio.file = fh;
    sc->nesting_stack[sc->file_i] = 0;
    port_unchecked(sc->loadport) = sc->load_stack + sc->file_i;

    sc->load_stack[sc->file_i].rep.stdio.curr_line = 0;
    sc->load_stack[sc->file_i].rep.stdio.filename = filename;
  } else {
    sc->free(filename);
  }
  return fh != 0;
}

static inline void file_pop(nanoclj_t * sc) {
  if (sc->file_i != 0) {
    sc->nesting = sc->nesting_stack[sc->file_i];
    port_close(sc, port_unchecked(sc->loadport));
    sc->file_i--;
    port_unchecked(sc->loadport) = sc->load_stack + sc->file_i;
  }
}

static inline nanoclj_port_t * mk_port(nanoclj_t * sc, int kind) {
  nanoclj_port_t * pt = sc->malloc(sizeof(nanoclj_port_t));
  if (!pt) return NULL;
  pt->backchar[0] = pt->backchar[1] = -1;
  pt->kind = kind;
  return pt;
}

static inline nanoclj_port_t *port_rep_from_file(nanoclj_t * sc, FILE * f, int prop) {
  nanoclj_port_t *pt = mk_port(sc, port_file | prop);
  if (!pt) return NULL;
  pt->rep.stdio.file = f;
  pt->rep.stdio.filename = NULL;
  return pt;
}

static inline int http_open_thread(nanoclj_t * sc, strview_t sv) {
  int pipefd[2];
  if (pipe(pipefd) == -1) {
    return 0;
  }

  http_load_t * d = malloc(sizeof(http_load_t));
  d->url = alloc_cstr(sc, sv);
  d->useragent = NULL;
  d->http_keepalive = true;
  d->http_max_connections = 0;
  d->http_max_redirects = 0;
  d->fd = pipefd[1];
  d->malloc = sc->malloc;
  d->free = sc->free;
  d->realloc = sc->realloc;

  nanoclj_val_t v;
  if (get_elem(sc, sc->properties, mk_string(sc, "http.agent"), &v)) {
    d->useragent = alloc_cstr(sc, to_strview(v));
  }
  if (get_elem(sc, sc->properties, mk_string(sc, "http.keepalive"), &v)) {
    d->http_keepalive = to_int(v);
  }
  if (get_elem(sc, sc->properties, mk_string(sc, "http.maxConnections"), &v)) {
    d->http_max_connections = to_int(v);
  }
  if (get_elem(sc, sc->properties, mk_string(sc, "http.maxRedirects"), &v)) {
    d->http_max_redirects = to_int(v);
  }
  
  pthread_t thread;
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  pthread_create(&thread, &attr, http_load, d);
  
  return pipefd[0];
}

static inline nanoclj_val_t port_from_filename(nanoclj_t * sc, strview_t sv, int prop) {
  FILE * f = NULL;
  const char * mode;
  if (prop == (port_input | port_output)) {
    mode = "a+";
  } else if (prop == port_output) {
    mode = "w";
  } else {
    mode = "r";
  }

  char * filename = NULL;
  if ((sv.size >= 7 && strncmp(sv.ptr, "http://", 7) == 0) ||
      (sv.size >= 8 && strncmp(sv.ptr, "https://", 8) == 0)) {
    int fd = http_open_thread(sc, sv);
    f = fdopen(fd, mode);
  } else {
    filename = store_string(sc, sv.size, sv.ptr);
    f = fopen(filename, mode);
  }
  
  nanoclj_val_t r = mk_nil();
  if (f) {
    nanoclj_port_t * pt = port_rep_from_file(sc, f, prop);
    pt->rep.stdio.filename = filename;
    pt->rep.stdio.curr_line = 0;
    
    if (prop & port_input) {
      r = mk_reader(sc, pt);
    } else {
      r = mk_writer(sc, pt);
    }
  }
  return r;
}

static inline nanoclj_val_t port_from_file(nanoclj_t * sc, FILE * f, int prop) {
  nanoclj_port_t *pt = port_rep_from_file(sc, f, prop);
  if (!pt) {
    return mk_nil();
  } else if (prop & port_input) {
    return mk_reader(sc, pt);
  } else {
    return mk_writer(sc, pt);
  }
}

static inline nanoclj_val_t port_from_callback(nanoclj_t * sc,
					       void (*text) (const char *, size_t, void *),
					       void (*color) (double, double, double, void *),
					       void (*restore) (void *),
					       void (*image) (nanoclj_image_t*, void*),
					       int prop) {
  nanoclj_port_t * pt = mk_port(sc, port_callback | prop);
  if (!pt) return mk_nil();
  pt->rep.callback.text = text;
  pt->rep.callback.color = color;
  pt->rep.callback.restore = restore;
  pt->rep.callback.image = image;
  return mk_writer(sc, pt);
}

static inline nanoclj_val_t port_from_string(nanoclj_t * sc, strview_t sv, int prop) {
  nanoclj_port_t * pt = mk_port(sc, port_string | prop);
  if (!pt) return mk_nil();

  char * buffer = (char *)sc->malloc(sv.size);
  memcpy(buffer, sv.ptr, sv.size);
  
  pt->rep.string.curr = buffer;
  pt->rep.string.data.data = buffer;
  pt->rep.string.data.size = sv.size;
  
  if (prop & port_input) {
    return mk_reader(sc, pt);
  } else {
    return mk_writer(sc, pt);
  }
}

#define INITIAL_STRING_LENGTH 256

static inline nanoclj_val_t port_from_scratch(nanoclj_t * sc) {
  nanoclj_port_t * pt = mk_port(sc, port_string | port_output);
  if (!pt) return mk_nil();

  char * start = sc->malloc(INITIAL_STRING_LENGTH);
  if (!start) {
    sc->free(pt);
    return mk_nil();
  }
  
  pt->rep.string.curr = start;
  pt->rep.string.data.data = start;
  pt->rep.string.data.size = INITIAL_STRING_LENGTH;

  return mk_writer(sc, pt);
}

static inline int realloc_port_string(nanoclj_t * sc, nanoclj_port_t * p) {
  char *start = p->rep.string.data.data;
  size_t new_size = 2 * p->rep.string.data.size;
  char *str = sc->malloc(new_size);
  if (str) {
    memcpy(str, start, p->rep.string.data.size);
    p->rep.string.curr -= start - str;
    p->rep.string.data.data = str;
    p->rep.string.data.size = new_size;
    sc->free(start);
    return 1;
  } else {
    return 0;
  }
}

static inline void putchars(nanoclj_t * sc, const char *s, size_t len, nanoclj_val_t out) {
  assert(s);

  if (is_writer(out)) {
    nanoclj_port_t * pt = port_unchecked(out);
    if (pt->kind & port_file) {
      fwrite(s, 1, len, pt->rep.stdio.file);
    } else if (pt->kind & port_callback) {
      pt->rep.callback.text(s, len, sc->ext_data);
    } else if (pt->kind & port_string) {
      for (; len; len--) {
	if (pt->rep.string.curr != pt->rep.string.data.data + pt->rep.string.data.size) {
	  *pt->rep.string.curr++ = *s++;
	} else if (realloc_port_string(sc, pt)) {
	  *pt->rep.string.curr++ = *s++;
	}
      }
    }
  } else if (is_canvas(out)) {
#if NANOCLJ_HAS_CANVAS
    void * canvas = canvas_unchecked(out);
    canvas_show_text(sc, canvas, (strview_t){ s, len });
#endif
  }
}

static inline void putstr(nanoclj_t * sc, const char *s, nanoclj_val_t dest_port) {
  putchars(sc, s, strlen(s), dest_port);
}

static inline void putcharacter(nanoclj_t * sc, int c, nanoclj_val_t out) {
  if (c != EOF) {
    size_t len = char_to_utf8(c, sc->strbuff);
    putchars(sc, sc->strbuff, len, out);
  }
}

static inline void set_color(nanoclj_t * sc, nanoclj_cell_t * vec, nanoclj_val_t out) {
  if (is_canvas(out)) {
#if NANOCLJ_HAS_CANVAS
    canvas_set_color(canvas_unchecked(out),
		     to_double(vector_elem(vec, 0)),
		     to_double(vector_elem(vec, 1)),
		     to_double(vector_elem(vec, 2)));
#endif
  } else if (is_writer(out)) {
    nanoclj_port_t * pt = port_unchecked(out);
    if (pt->kind & port_file) {
#ifndef WIN32    
      FILE * fh = pt->rep.stdio.file;
      set_term_color(fh,
		     to_double(vector_elem(vec, 0)),
		     to_double(vector_elem(vec, 1)),
		     to_double(vector_elem(vec, 2)),
		     sc->term_colors
		     );
#endif
    } else if (pt->kind & port_callback) {
      if (pt->rep.callback.color) {
	pt->rep.callback.color(to_double(vector_elem(vec, 0)),
			       to_double(vector_elem(vec, 1)),
			       to_double(vector_elem(vec, 2)),
			       sc->ext_data
			       );
      }
    }
  }
}

static inline int check_strbuff_size(nanoclj_t * sc, char **p) {
  char *t;
  int len = *p - sc->strbuff;
  if (len + 4 < sc->strbuff_size) {
    return 1;
  }
  sc->strbuff_size *= 2;
  if (sc->strbuff_size >= STRBUFF_MAX_SIZE) {
    sc->strbuff_size /= 2;
    return 0;
  }
  t = sc->malloc(sc->strbuff_size);
  memcpy(t, sc->strbuff, len);
  *p = t + len;
  sc->free(sc->strbuff);
  sc->strbuff = t;
  return 1;
}

/* check c is in chars */
static inline bool is_one_of(char *s, int c) {
  if (c == EOF)
    return true;
  while (*s)
    if (*s++ == c)
      return true;
  return false;
}

/* read characters up to delimiter, but cater to character constants */
static inline char *readstr_upto(nanoclj_t * sc, char *delim) {
  char *p = sc->strbuff;
  nanoclj_port_t * inport = port_unchecked(get_in_port(sc));
  bool found_delim = false;
  
  while (1) {
    int c = inchar(inport);
    if (c == EOF) break;
    
    if (check_strbuff_size(sc, &p)) {
      p += char_to_utf8(c, p);
    }
    if (is_one_of(delim, c)) {
      found_delim = true;
      break;
    }
  }

  /* Check if the delimiter was escaped */
  if (p == sc->strbuff + 2 && p[-2] == '\\') {
    *p = 0;
  } else if (found_delim) { /* Return the delimiter */
    backchar(p[-1], inport);
    *--p = '\0';
  } else {
    *p = 0;
  }
  return sc->strbuff;
}

/* read string expression "xxx...xxx" */
static inline nanoclj_val_t readstrexp(nanoclj_t * sc) {
  char *p = sc->strbuff;
  int c;
  int c1 = 0;
  enum { st_ok, st_bsl, st_x1, st_x2, st_oct1, st_oct2 } state = st_ok;

  nanoclj_port_t * inport = port_unchecked(get_in_port(sc));

  for (;;) {
    c = inchar(inport);
    if (c == EOF || !check_strbuff_size(sc, &p)) {
      return (nanoclj_val_t)kFALSE;
    }
    switch (state) {
    case st_ok:
      switch (c) {
      case '\\':
        state = st_bsl;
        break;
      case '"':
        return mk_pointer(get_string_object(sc, T_STRING, sc->strbuff, p - sc->strbuff, 0));
      default:
        p += char_to_utf8(c, p);
        break;
      }
      break;
    case st_bsl:
      switch (c) {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
        state = st_oct1;
        c1 = c - '0';
        break;
      case 'x':
      case 'X':
        state = st_x1;
        c1 = 0;
        break;
      case 'n':
        *p++ = '\n';
        state = st_ok;
        break;
      case 't':
        *p++ = '\t';
        state = st_ok;
        break;
      case 'r':
        *p++ = '\r';
        state = st_ok;
        break;
      case '"':
        *p++ = '"';
        state = st_ok;
        break;
      default:
        *p++ = c;
        state = st_ok;
        break;
      }
      break;
    case st_x1:
    case st_x2:
      c = toupper(c);
      if (c >= '0' && c <= 'F') {
        if (c <= '9') {
          c1 = (c1 << 4) + c - '0';
        } else {
          c1 = (c1 << 4) + c - 'A' + 10;
        }
        if (state == st_x1) {
          state = st_x2;
        } else {
          *p++ = c1;
          state = st_ok;
        }
      } else {
        return (nanoclj_val_t)kFALSE;
      }
      break;
    case st_oct1:
    case st_oct2:
      if (c < '0' || c > '7') {
        *p++ = c1;
        backchar(c, inport);
        state = st_ok;
      } else {
        if (state == st_oct2 && c1 >= 32)
          return (nanoclj_val_t)kFALSE;

        c1 = (c1 << 3) + (c - '0');

        if (state == st_oct1)
          state = st_oct2;
        else {
          *p++ = c1;
          state = st_ok;
        }
      }
      break;

    }
  }
}

/* skip white space characters, returns the first non-whitespace character */
static inline int skipspace(nanoclj_t * sc, nanoclj_port_t * inport) {
  int c = 0;
  do {
    c = inchar(inport);
  } while (isspace(c));

  return c;
}

/* get token */
static inline int token(nanoclj_t * sc, nanoclj_port_t * inport) {
  int c;
  switch (c = skipspace(sc, inport)) {
  case EOF:
    return (TOK_EOF);
  case '(':
    return (TOK_LPAREN);
  case ')':
    return (TOK_RPAREN);
  case ']':
    return (TOK_RSQUARE);
  case '}':
    return (TOK_RCURLY);
  case '.':
    c = inchar(inport);
    if (is_one_of(" \n\t", c)) {
      return (TOK_DOT);
    } else {
      backchar(c, inport);
      backchar('.', inport);
      return TOK_PRIMITIVE;
    }
  case '\'':
    return (TOK_QUOTE);
  case '@':
    return (TOK_DEREF);
  case ';':
    while ((c = inchar(inport)) != '\n' && c != EOF) { }
    if (c == EOF) {
      return (TOK_EOF);
    } else {
      return (token(sc, inport));
    }
  case '"':
    return (TOK_DQUOTE);
  case BACKQUOTE:
    return (TOK_BQUOTE);
  case ',':
    if ((c = inchar(inport)) == '@') {
      return (TOK_ATMARK);
    } else if (is_one_of("\n\r ", c)) {
      backchar(c, inport);
      return (token(sc, inport));
    } else {
      backchar(c, inport);
      return (TOK_COMMA);
    }
  case '[':
    return (TOK_VEC);

  case '{':
    return (TOK_MAP);

  case '\\':
    backchar('\\', inport);
    if (1) { // next_char != ' ') {
      return (TOK_CHAR_CONST);
    } else {
      return TOK_PRIMITIVE;
    }
    
  case '#':
    c = inchar(inport);
    if (c == '(') {
      return (TOK_FN);
    } else if (c == '{') {
      return (TOK_SET);
    } else if (c == '"') {
      return (TOK_REGEX);
    } else if (c == '_') {
      return (TOK_IGNORE);
    } else if (c == '\'') {
      return (TOK_VAR);
    } else if (c == '#') {
      return (TOK_SHARP_CONST);
    } else if (c == '!') {
      /* This is a shebang line, so skip it */
      while ((c = inchar(inport)) != '\n' && c != EOF) { }

      if (c == EOF) {
        return (TOK_EOF);
      } else {
        return (token(sc, inport));
      }
    } else {
      backchar(c, inport);
      return (TOK_TAG);
    }
  default:
    backchar(c, inport);
    return TOK_PRIMITIVE;
  }
}

/* ========== Routines for Printing ========== */

static inline void print_slashstring(nanoclj_t * sc, strview_t sv, nanoclj_val_t out) {
  assert(is_writer(out));

  const char * p = sv.ptr;
  int c, d;
  putcharacter(sc, '"', out);
  const char * end = p + sv.size;
  while (p < end) {
    c = utf8_decode(p);
    p = utf8_next(p);
    
    if (c == '"' || c < ' ' || c == '\\' || (c >= 0x7f && c <= 0xa0)) {
      putcharacter(sc, '\\', out);
      switch (c) {
      case '"':
        putcharacter(sc, '"', out);
        break;
      case '\n':
        putcharacter(sc, 'n', out);
        break;
      case '\t':
        putcharacter(sc, 't', out);
        break;
      case '\r':
        putcharacter(sc, 'r', out);
        break;
      case '\\':
        putcharacter(sc, '\\', out);
        break;
      default:{
	putchars(sc, "u00", 3, out);
	d = c / 16;
	putcharacter(sc, d + (d < 10 ? '0' : 'A' - 10), out);
	d = c % 16;
	putcharacter(sc, d + (d < 10 ? '0' : 'A' - 10), out);
      }
      }
    } else {
      putcharacter(sc, c, out);
    }
  }
  putcharacter(sc, '"', out);
}

#if NANOCLJ_SIXEL
static int sixel_write(char *data, int size, void *priv) {
  return fwrite(data, 1, size, (FILE *)priv);
}
static inline void print_image_sixel(unsigned char * data, int width, int height) {
  sixel_dither_t * dither;
  sixel_dither_new(&dither, -1, NULL);
  sixel_dither_initialize(dither, data, width, height, SIXEL_PIXELFORMAT_RGB888, SIXEL_LARGE_NORM, SIXEL_REP_CENTER_BOX, SIXEL_QUALITY_HIGHCOLOR);
  
  sixel_output_t * output = NULL;
  sixel_output_new(&output, sixel_write, stdout, NULL);
  
  /* convert pixels into sixel format and write it to output context */
  sixel_encode(data,
	       width,
	       height,
	       8,
	       dither,
	       output);

  sixel_output_destroy(output);
  sixel_dither_destroy(dither);
}
#endif

static inline void print_image(nanoclj_t * sc, nanoclj_image_t * img, nanoclj_val_t out) {
  assert(is_writer(out));
  if (!is_writer(out)) {
    return;
  }

  nanoclj_port_t * pt = port_unchecked(out);

  if (pt->kind & port_callback) {
    if (pt->rep.callback.image) {
      pt->rep.callback.image(img, sc->ext_data);
    }
  } else {
    if (sc->sixel_term && (pt->kind & port_file) && pt->rep.stdio.file == stdout) {      
#if NANOCLJ_SIXEL
      if (img->channels == 4) {
	unsigned char * tmp = (unsigned char *)sc->malloc(3 * img->width * img->height);
	for (unsigned int i = 0; i < img->width * img->height; i++) {
	  tmp[3 * i + 0] = img->data[4 * i + 0];
	  tmp[3 * i + 1] = img->data[4 * i + 1];
	  tmp[3 * i + 2] = img->data[4 * i + 2];
	}
	print_image_sixel(tmp, img->width, img->height);
	sc->free(tmp);
	return;
      } else if (img->channels == 3) {
	print_image_sixel(img->data, img->width, img->height);
	return;
      }
#endif      
    }

    char * p = sc->strbuff;
    size_t len = snprintf(sc->strbuff, sc->strbuff_size, "#<Image %d %d %d>", img->width, img->height, img->channels);
    putchars(sc, p, 3, out);
  }
}

static inline nanoclj_cell_t * get_type_object(nanoclj_t * sc, nanoclj_val_t v) {
  int type_id = type(v);
  if (type_id >= 0 && (size_t)type_id < sc->types->size) {
    nanoclj_cell_t * t = decode_pointer(sc->types->data[type_id]);
    if (t) return t;
  }
  return NULL;
}

/* Uses internal buffer unless string pointer is already available */
static inline void print_primitive(nanoclj_t * sc, nanoclj_val_t l, int print_flag, nanoclj_val_t out) {
  const char *p = 0;
  int plen = -1;

  switch (prim_type(l)) {
  case T_INTEGER:
    p = sc->strbuff;
    plen = sprintf(sc->strbuff, "%d", (int)decode_integer(l));
    break;
  case T_REAL:
    if (isnan(l.as_double)) {
      p = "##NaN";
    } else if (isinf(l.as_double)) {
      p = l.as_double > 0 ? "##Inf" : "##-Inf";
    } else {
      p = sc->strbuff;
      plen = sprintf(sc->strbuff, "%.15g", l.as_double);
    }
    break;
  case T_CHARACTER:    
    if (!print_flag) {
      p = sc->strbuff;
      plen = char_to_utf8(decode_integer(l), sc->strbuff);
    } else {
      int c = decode_integer(l);
      switch (c) {
      case -1:
	p = "##Eof";
	break;
      case ' ':
        p = "\\space";
        break;
      case '\n':
        p = "\\newline";
        break;
      case '\r':
        p = "\\return";
        break;
      case '\t':
        p = "\\tab";
        break;
      case '\f':
	p = "\\formfeed";
	break;
      case '\b':
	p = "\\backspace";
	break;
      default:
	p = sc->strbuff;
      	if (c < 32 || (c >= 0x7f && c <= 0xa0)) {
          plen = sprintf(sc->strbuff, "\\u%04x", c);
        } else {
	  sc->strbuff[0] = '\\';
	  plen = 1 + char_to_utf8(c, sc->strbuff + 1);
	}
        break;
      }
    }
    break;
  case T_BOOLEAN:
  case T_SYMBOL:{
    strview_t sv = to_strview(l);
    p = sv.ptr;
    plen = sv.size;
  }
    break;
  case T_KEYWORD:{
    strview_t sv = to_strview(l);
    p = sc->strbuff;
    plen = snprintf(sc->strbuff, sc->strbuff_size, ":%s", sv.ptr);
  }
    break;
  case T_CELL:{
    nanoclj_cell_t * c = decode_pointer(l);
    if (c == NULL) {
      p = "nil";    
    } else {
      switch (_type(c)) {
      case T_NIL:
	p = "()";
	break;
      case T_CLASS:
	nanoclj_val_t name_v;
	if (get_elem(sc, _cons_metadata(c), sc->NAME, &name_v)) {
	  strview_t name = to_strview(name_v);
	  p = name.ptr;
	  plen = name.size;	  
	}
	break;
      case T_LONG:
	p = sc->strbuff;
	plen = sprintf(sc->strbuff, "%lld", _lvalue_unchecked(c));
	break;
      case T_STRING:
	if (!print_flag) {
	  p = _strvalue(c);
	  plen = _get_size(c);
	  break;
	} else {
	  print_slashstring(sc, to_strview(l), out);
	  return;
	}
      case T_WRITER:
	if (_port_unchecked(c)->kind & port_string) {
	  nanoclj_port_t * pt = port_unchecked(l);
	  p = pt->rep.string.data.data;
	  plen = pt->rep.string.curr - pt->rep.string.data.data;
	} 
	break;
      case T_ENVIRONMENT:{
	nanoclj_val_t name_v;
	strview_t name;
	if (get_elem(sc, _cons_metadata(c), sc->NAME, &name_v)) {
	  name = to_strview(name_v);	  
	} else {
	  name = (strview_t){ "", 0 };
	}
	p = sc->strbuff;
	plen = snprintf(sc->strbuff, sc->strbuff_size, "#<Namespace %.*s>", (int)name.size, name.ptr);
      }
	break;
      case T_RATIO:
	p = sc->strbuff;
	plen = snprintf(sc->strbuff, sc->strbuff_size, "%lld/%lld", to_long(vector_elem(c, 0)), to_long(vector_elem(c, 1)));
	break;
      case T_FILE:
	p = sc->strbuff;
	plen = snprintf(sc->strbuff, sc->strbuff_size, "#<File %.*s>", (int)_get_size(c), _strvalue(c)); 
	break;
      case T_IMAGE:
	print_image(sc, _image_unchecked(c), out);
	return;
      case T_CANVAS:{
#if NANOCLJ_HAS_CANVAS
	nanoclj_val_t image = canvas_create_image(sc, canvas_unchecked(l));
	if (is_writer(out)) {
	  nanoclj_port_t * pt = _port_unchecked(decode_pointer(out)); 
	  if (pt->kind & port_file) {
	    FILE * fh = pt->rep.stdio.file;
	    int x, y;
	    if (fh == stdout) {
	      sc->active_element = l;
	      sc->active_element_target = out;
	      if (0 && get_cursor_position(stdin, fh, &x, &y)) {
		sc->active_element_x = x;
		sc->active_element_y = y;
	      }
	    } else {
	      sc->active_element = mk_nil();
	    }
	  }
	}

	print_image(sc, image_unchecked(image), out);
	return;
      }
#endif
      }
    }
  }
    break;
  }
  
  if (!p) {
    nanoclj_val_t name_v;
    if (get_elem(sc, _cons_metadata(get_type_object(sc, l)), sc->NAME, &name_v)) {
      strview_t sv = to_strview(name_v);
      p = sc->strbuff;
      plen = snprintf(sc->strbuff, sc->strbuff_size, "#object[%.*s]", (int)sv.size, sv.ptr);
    } else {
      p = "#object";
    }
  }
  
  if (plen < 0) {
    plen = strlen(p);
  }
  
  putchars(sc, p, plen, out);
}

static inline size_t seq_length(nanoclj_t * sc, nanoclj_cell_t * a) {
  if (!a) {
    return 0;
  }
  size_t i = 0;  
  switch (_type(a)) {
  case T_NIL:
    return 0;

  case T_LONG:
    return 1;
        
  case T_VECTOR:
  case T_ARRAYMAP:
  case T_SORTED_SET:
  case T_RATIO:
  case T_STRING:
  case T_CHAR_ARRAY:
  case T_FILE:
  case T_MAPENTRY:
  case T_VAR:
    return _get_size(a);

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
  if (sc->no_memory) {
    return NULL;
  }
  
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
  return coll;
}

static inline nanoclj_val_t mk_format_va(nanoclj_t * sc, char *fmt, ...) {
  /* Determine required size */
  va_list ap;
  va_start(ap, fmt);
  int n = vsnprintf(NULL, 0, fmt, ap);
  va_end(ap);

  nanoclj_cell_t * r = NULL;
  if (n >= 0) {
    /* Reserve 1-byte padding for the 0-marker */
    r = get_string_object(sc, T_STRING, NULL, n, 1);

    if (n > 0) {
      va_start(ap, fmt);
      vsnprintf(_strvalue(r), n + 1, fmt, ap);
      va_end(ap);
    }
  }
  sc->free(fmt);
  return mk_pointer(r);
}

static inline nanoclj_val_t mk_format(nanoclj_t * sc, strview_t fmt0, nanoclj_cell_t * args) {
  char * fmt = sc->malloc(2 * fmt0.size + 1);
  size_t fmt_size = 0;
  for (size_t i = 0; i < fmt0.size; i++) {
    if (i + 1 < fmt0.size && fmt0.ptr[i] == '%' && fmt0.ptr[i + 1] == 's') {
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
  
  for (; args; args = next(sc, args), n_args++, m *= 4) {
    nanoclj_val_t arg = first(sc, args);
    
    switch (type(arg)) {
    case T_INTEGER:
    case T_LONG:
    case T_PROC:
    case T_BOOLEAN:
      switch (n_args) {
      case 0: arg0l = to_long(arg); break;
      case 1: arg1l = to_long(arg); break;
      }
      plan += 1 * m;
      break;
    case T_CHARACTER:
      switch (n_args) {
      case 0: arg0l = decode_integer(arg); break;
      case 1: arg1l = decode_integer(arg); break;
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
    case T_STRING:
    case T_CHAR_ARRAY:
    case T_FILE:
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
  
  switch (plan) {
  case 1: return mk_format_va(sc, fmt, arg0l);
  case 2: return mk_format_va(sc, fmt, arg0d);
  case 3: return mk_format_va(sc, fmt, (int)arg0s.size, arg0s.ptr);
    
  case 5: return mk_format_va(sc, fmt, arg0l, arg1l);
  case 6: return mk_format_va(sc, fmt, arg0d, arg1l);
  case 7: return mk_format_va(sc, fmt, (int)arg0s.size, arg0s.ptr, arg1l);
    
  case 9: return mk_format_va(sc, fmt, arg0l, arg1d);
  case 10: return mk_format_va(sc, fmt, arg0d, arg1d);
  case 11: return mk_format_va(sc, fmt, (int)arg0s.size, arg0s.ptr, arg1d);

  case 13: return mk_format_va(sc, fmt, arg0l, arg1s);
  case 14: return mk_format_va(sc, fmt, arg0d, arg1s);
  case 15: return mk_format_va(sc, fmt, (int)arg0s.size, arg0s.ptr, (int)arg1s.size, arg1s.ptr);

  default: return mk_format_va(sc, fmt);
  }
}

/* ========== Routines for Evaluation Cycle ========== */

/* reverse list --- in-place */
static inline nanoclj_cell_t * reverse_in_place(nanoclj_t * sc, nanoclj_val_t term, nanoclj_cell_t * list) {
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
  return slot_value_in_env(find_slot_in_env(sc, sc->envir, sc->OUT, true));
}

static nanoclj_val_t get_in_port(nanoclj_t * sc) {
  return slot_value_in_env(find_slot_in_env(sc, sc->envir, sc->IN, true));
}

static inline bool _Error_1(nanoclj_t * sc, const char *s, size_t len) {
  const char *str = s;
  
  char sbuf[AUXBUFF_SIZE];

  /* make sure error is not in REPL */
  if (sc->load_stack[sc->file_i].kind & port_file &&
      sc->load_stack[sc->file_i].rep.stdio.file != stdin) {      
    /* we started from line 0 */
    int ln = sc->load_stack[sc->file_i].rep.stdio.curr_line + 1;
    const char *fname = sc->load_stack[sc->file_i].rep.stdio.filename;

    /* should never happen */
    if (!fname) fname = "<unknown>";

    len = snprintf(sbuf, AUXBUFF_SIZE, "(%s : %i) %.*s", fname, ln, (int)len, str);
    str = (const char *) sbuf;
  }

  nanoclj_val_t error_str = mk_pointer(get_string_object(sc, T_STRING, str, len, 0));
  sc->args = conjoin(sc, sc->EMPTYVEC, error_str);
  sc->op = OP_ERR0;
  return true;
}

#define Error_0(sc,s)    return _Error_1(sc, s, strlen(s))

/* Too small to turn into function */
#define  BEGIN     do {
#define  END  } while (0)
#define s_goto(sc,a) BEGIN                                  \
    sc->op = (a);                                           \
    return true; END

#define s_return(sc,a) return _s_return(sc,a)
  
static inline void s_save(nanoclj_t * sc, enum nanoclj_opcodes op,
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

static inline void dump_stack_reset(nanoclj_t * sc) {
  /* in this implementation, sc->dump is the number of frames on the stack */
  sc->dump = 0;
}

static inline void dump_stack_initialize(nanoclj_t * sc) {
  sc->dump_size = 256;
  sc->dump_base = (dump_stack_frame_t *)sc->malloc(sizeof(dump_stack_frame_t) * sc->dump_size);
  dump_stack_reset(sc);
}

static inline void dump_stack_free(nanoclj_t * sc) {
  free(sc->dump_base);
  sc->dump_base = NULL;
  sc->dump = 0;
  sc->dump_size = 0;
}

#define s_retbool(tf)    s_return(sc, (tf) ? (nanoclj_val_t)kTRUE : (nanoclj_val_t)kFALSE)

static inline bool destructure(nanoclj_t * sc, nanoclj_cell_t * binding, nanoclj_cell_t * y, size_t num_args, bool first_level) {
  size_t n = _get_size(binding);

  y = seq(sc, y);
  
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
	  nanoclj_cell_t * y2 = next(sc, y); /* TODO: what is this? */
	  if (!destructure(sc, decode_pointer(e), y2, 0, false)) {
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
	} else {
	  nanoclj_cell_t * y3 = decode_pointer(y2);
	  if (!destructure(sc, decode_pointer(e), y3, 0, false)) {
	    return false;
	  }
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

/* Constructs an object by type, args are seqed */
static inline nanoclj_val_t construct_by_type(nanoclj_t * sc, int type_id, nanoclj_cell_t * args) {
  nanoclj_val_t x, y;
  nanoclj_cell_t * z;
  
  switch (type_id) {
  case T_NIL:
    return sc->EMPTY;
    
  case T_BOOLEAN:
    return is_true(first(sc, args)) ? (nanoclj_val_t)kTRUE : (nanoclj_val_t)kFALSE;

  case T_STRING:
  case T_CHAR_ARRAY:
  case T_FILE:{
    strview_t sv = to_strview(first(sc, args));
    return mk_pointer(get_string_object(sc, type_id, sv.ptr, sv.size, 0));
  }
  case T_VECTOR:
  case T_ARRAYMAP:
  case T_SORTED_SET:
    return mk_pointer(mk_collection(sc, type_id, args));

  case T_CLOSURE:
    x = first(sc, args);
    if (is_cell(x) && first(sc, decode_pointer(x)).as_long == sc->LAMBDA.as_long) {
      x = mk_pointer(rest(sc, decode_pointer(x)));
    }
    args = next(sc, args);
    z = args ? decode_pointer(first(sc, args)) : sc->envir;
    /* make closure. first is code. second is environment */
    return mk_pointer(get_cell(sc, T_CLOSURE, 0, x, z, NULL));
    
  case T_INTEGER:
    return mk_int(to_int(first(sc, args)));

  case T_LONG:
    /* Casts the argument to long (or int if it is sufficient) */
    return mk_integer(sc, to_long(first(sc, args)));
  
  case T_REAL:
    return mk_real(to_double(first(sc, args)));

  case T_CHARACTER:
    return mk_character(to_int(first(sc, args)));

  case T_CLASS:
    return mk_pointer(mk_type(sc, to_int(first(sc, args)), rest(sc, args), NULL));

  case T_ENVIRONMENT:{
    nanoclj_cell_t * parent = NULL;
    if (args) {
      parent = decode_pointer(first(sc, args));
    }
    nanoclj_cell_t * vec = get_vector_object(sc, T_VECTOR, OBJ_LIST_SIZE);
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
    return def_symbol_from_sv(sc, to_strview(first(sc, args)));
    
  case T_KEYWORD:
    return def_keyword_from_sv(sc, to_strview(first(sc, args)));

  case T_LAZYSEQ:
  case T_DELAY:
    /* make closure. first is code. second is environment */
    return mk_pointer(get_cell(sc, type_id, 0, mk_pointer(cons(sc, sc->EMPTY, decode_pointer(first(sc, args)))), sc->envir, NULL));

  case T_READER:
    x = first(sc, args);
    if (is_string(x)) {
      return port_from_filename(sc, to_strview(x), port_input);
    } else if (is_char_array(x)) {
      return port_from_string(sc, to_strview(x), port_input);
    }
    break;
    
  case T_WRITER:
    if (!args) {
      return port_from_scratch(sc);
    } else {
      x = first(sc, args);
      if (is_string(x)) {
	return port_from_filename(sc, to_strview(x), port_output);
      }
    }
    break;

  case T_CANVAS:
#if NANOCLJ_HAS_CANVAS
    return mk_canvas(sc,
		     (int)(to_double(first(sc, args)) * sc->window_scale_factor),
		     (int)(to_double(second(sc, args)) * sc->window_scale_factor));
#endif

  case T_IMAGE:
    if (!args) {
      return mk_image(sc, 0, 0, 0, NULL);      
    } else {
      x = first(sc, args);
      if (is_canvas(x)) {
#if NANOCLJ_HAS_CANVAS
	return canvas_create_image(sc, canvas_unchecked(x));
#endif
      }
    }
  }
  
  return mk_nil();
}

static char * dispatch_table[] = {
#define _OP_DEF(A,OP) A,
#include "nanoclj_opdf.h"
  0
};

static inline bool unpack_args_0(nanoclj_t * sc) {
  return is_empty(sc, sc->args);
}

static inline bool unpack_args_0_plus(nanoclj_t * sc, nanoclj_cell_t ** arg_next) {
  nanoclj_cell_t * c = next(sc, sc->args);
  *arg_next = c;
  return c != NULL;  
}

static inline bool unpack_args_1(nanoclj_t * sc, nanoclj_val_t * arg0) {
  if (!is_empty(sc, sc->args)) {
    *arg0 = first(sc, sc->args);
    return next(sc, sc->args) == NULL;
  }
  return false;
}

static inline bool unpack_args_1_plus(nanoclj_t * sc, nanoclj_val_t * arg0, nanoclj_cell_t ** arg_next) {
  if (!is_empty(sc, sc->args)) {
    *arg0 = first(sc, sc->args);
    *arg_next = next(sc, sc->args);
    return true;
  }
  return false;
}

static inline bool unpack_args_2(nanoclj_t * sc, nanoclj_val_t * arg0, nanoclj_val_t * arg1) {
  if (!is_empty(sc, sc->args)) {
    *arg0 = first(sc, sc->args);
    nanoclj_cell_t * r = next(sc, sc->args);
    if (r) {
      *arg1 = first(sc, r);
      return next(sc, r) == NULL;
    }
  }
  return false;
}

static inline bool unpack_args_3(nanoclj_t * sc, nanoclj_val_t * arg0, nanoclj_val_t * arg1, nanoclj_val_t * arg2) {
  if (!is_empty(sc, sc->args)) {
    *arg0 = first(sc, sc->args);
    nanoclj_cell_t * r = next(sc, sc->args);
    if (r) {
      *arg1 = first(sc, r);
      r = next(sc, r);
      if (r) {
	*arg2 = first(sc, r);
	return next(sc, r) == NULL;
      }
    }
  }
  return false;
}

static inline bool unpack_args_5(nanoclj_t * sc, nanoclj_val_t * arg0, nanoclj_val_t * arg1,
				 nanoclj_val_t * arg2, nanoclj_val_t * arg3, nanoclj_val_t * arg4) {
  if (!is_empty(sc, sc->args)) {
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
	    return next(sc, r) == NULL;
	  }
	}
      }
    }
  }
  return false;
}

static inline bool unpack_args_2_plus(nanoclj_t * sc, nanoclj_val_t * arg0, nanoclj_val_t * arg1, nanoclj_cell_t ** arg_next) {
  if (!is_empty(sc, sc->args)) {
    *arg0 = first(sc, sc->args);
    nanoclj_cell_t * r = next(sc, sc->args);
    if (r) {
      *arg1 = first(sc, r);
      *arg_next = next(sc, r);
      return true;
    }
  }
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

static inline const char *procname(enum nanoclj_opcodes op) {
  const char *name = dispatch_table[(int)op];
  if (name == 0) {
    name = "ILLEGAL!";
  }
  return name;
}

/* Executes and opcode, and returns true if execution should continue */
static inline bool opexe(nanoclj_t * sc, enum nanoclj_opcodes op) {
  nanoclj_val_t x, y;  
  nanoclj_cell_t * meta, * z;
  int syn;
  nanoclj_val_t params0;
  nanoclj_cell_t * params;
  nanoclj_val_t arg0, arg1, arg2, arg3, arg4;
  nanoclj_cell_t * arg_next;
  
#if 0
  fprintf(stderr, "opexe %d %s\n", (int)sc->op, procname(sc->op));
#endif

  if (sc->nesting != 0) {
    sc->nesting = 0;
    sc->retcode = -1;
    Error_0(sc, "Error - unmatched parentheses");
  }

  switch (op) {
  case OP_LOAD:                /* load */
    if (!unpack_args_1(sc, &arg0)) {
      Error_0(sc, "Error - Invalid arity");
    } else {
      strview_t sv = to_strview(arg0);
      if (!file_push(sc, sv)) {
	sprintf(sc->strbuff, "Error - unable to open %.*s", (int)sv.size, sv.ptr);
	Error_0(sc, sc->strbuff);
      } else {
	sc->args = mk_long(sc, sc->file_i);
	s_goto(sc, OP_T0LVL);
      }
    }

  case OP_T0LVL:               /* top level */
      /* If we reached the end of file, this loop is done. */
    if (port_unchecked(sc->loadport)->kind & port_saw_EOF) {
      if (sc->global_env != sc->root_env) {
#if 0
	fprintf(stderr, "restoring root env\n");
#endif
	sc->envir = sc->global_env = sc->root_env;
      }
      
      if (sc->file_i == 0) {
#if 0
        sc->args = sc->EMPTY;
        s_goto(sc, OP_QUIT);
#else
	return false;
#endif
      } else {
        file_pop(sc);
	s_return(sc, sc->value); 
      }
      /* NOTREACHED */
    }
    
    /* Set up another iteration of REPL */
    sc->nesting = 0;
    sc->save_inport = get_in_port(sc);
    intern(sc, sc->root_env, sc->IN, sc->loadport);
      
    s_save(sc, OP_T0LVL, NULL, sc->EMPTY);
    s_save(sc, OP_T1LVL, NULL, sc->EMPTY);

    sc->args = NULL;
    s_goto(sc, OP_READ);

  case OP_T1LVL:               /* top level */
    sc->code = sc->value;
    intern(sc, sc->root_env, sc->IN, sc->save_inport);
    s_goto(sc, OP_EVAL);

  case OP_READ:                /* read */
    x = get_in_port(sc);
    if (!is_reader(x)) {
      Error_0(sc, "Error - not a reader");
    } else {
      nanoclj_port_t * pt = port_unchecked(x);
      sc->tok = token(sc, pt);
      if (sc->tok == TOK_EOF) {
	s_return(sc, mk_nil());
      }
      s_goto(sc, OP_RDSEXPR);
    }

  case OP_READM:               /* .read */
    if (!unpack_args_1(sc, &arg0)) {
      Error_0(sc, "Error - Invalid arity");
    } else if (!is_reader(arg0)) {
      Error_0(sc, "Error - not a reader");
    } else {
      s_return(sc, mk_character(inchar(port_unchecked(arg0))));
    }
    
  case OP_GENSYM:
    if (unpack_args_0_plus(sc, &arg_next)) {
      strview_t sv = to_strview(first(sc, arg_next));
      s_return(sc, gensym(sc, sv.ptr, sv.size));
    } else {
      s_return(sc, gensym(sc, "G__", 3));
    }
    
  case OP_EVAL:                /* main part of evaluation */
#if USE_TRACING
    if (sc->tracing) {
      /*s_save(sc,OP_VALUEPRINT,sc->EMPTY,sc->EMPTY); */
      s_save(sc, OP_REAL_EVAL, sc->args, sc->code);
      sc->args = decode_pointer(sc->code);
      putstr(sc, "\nEval: ", get_err_port(sc));
      s_goto(sc, OP_P0LIST);
    }
    /* fall through */
  case OP_REAL_EVAL:
#endif
    switch (prim_type(sc->code)) {
    case T_SYMBOL:
      if (sc->code.as_long == sc->NS.as_long) { /* special symbols */
	s_return(sc, mk_pointer(sc->global_env));
      } else if (sc->code.as_long == sc->ENV.as_long) {
	s_return(sc, mk_pointer(sc->envir));
#ifdef USE_RECUR_REGISTER
      } else if (sc->code.as_long == sc->RECUR.as_long) {
	s_return(sc, sc->recur);
#endif
      } else {
	nanoclj_cell_t * c = find_slot_in_env(sc, sc->envir, sc->code, true);
	if (c) {
	  s_return(sc, slot_value_in_env(c));
	} else {
	  symbol_t * s = decode_symbol(sc->code);
	  snprintf(sc->strbuff, sc->strbuff_size, "Use of undeclared Var %s", s->name);
	  Error_0(sc, sc->strbuff);
	}
      }

    case T_CELL:{
      nanoclj_cell_t * code_cell = decode_pointer(sc->code);
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
	if (_get_size(code_cell) > 0) {
	  s_save(sc, OP_E0VEC, mk_long(sc, 0), sc->code);
	  sc->code = vector_elem(code_cell, 0);
	  s_goto(sc, OP_EVAL);
	}
      }
      }
    }
    }
    /* Default */
    s_return(sc, sc->code);

  case OP_E0VEC:{	       /* eval vector element */
    nanoclj_cell_t * vec = decode_pointer(sc->code);
    long long i = _lvalue_unchecked(sc->args);
    set_vector_elem(vec, i, sc->value);
    if (i + 1 < _get_size(vec)) {
      s_save(sc, OP_E0VEC, mk_long(sc, i + 1), sc->code);
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
	sc->args = reverse_in_place(sc, sc->EMPTY, sc->args);
      }
      sc->code = first(sc, sc->args);
      sc->args = next(sc, sc->args);
      s_goto(sc, OP_APPLY);
    }

#if USE_TRACING
  case OP_TRACING:{
    if (!unpack_args_1(sc, &arg0)) {
      Error_0(sc, "Error - Invalid arity");
    }

    int tr = sc->tracing;
    sc->tracing = to_int(arg0);
    s_return(sc, mk_int(sc, tr));
  }
#endif

  case OP_APPLY:               /* apply 'code' to 'args' */
#if USE_TRACING
    if (sc->tracing) {
      s_save(sc, OP_REAL_APPLY, sc->args, sc->code);
      /*  sc->args=cons(sc,sc->code,sc->args); */
      putstr(sc, "\nApply to: ", get_err_port(sc));
      s_goto(sc, OP_P0LIST);
    }
    /* fall through */
  case OP_REAL_APPLY:
#endif
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

    case T_CELL:{      
      nanoclj_cell_t * code_cell = decode_pointer(sc->code);
      switch (_type(code_cell)) {
      case T_CLASS:
	s_return(sc, construct_by_type(sc, code_cell->type, seq(sc, sc->args)));
	
      case T_FOREIGN_FUNCTION:{
	/* Keep nested calls from GC'ing the arglist */
	retain(sc, sc->args);
	if (_min_arity_unchecked(code_cell) > 0 || _max_arity_unchecked(code_cell) < 0x7fffffff) {
	  int n = seq_length(sc, sc->args);
	  if (n < _min_arity_unchecked(code_cell) || n > _max_arity_unchecked(code_cell)) {
	    Error_0(sc, "Error - Wrong number of args passed (1)");
	  }
	}
	x = _ff_unchecked(code_cell)(sc, mk_pointer(sc->args));
	if (is_exception(x)) {
	  strview_t sv = to_strview(car(x));
	  return _Error_1(sc, sv.ptr, sv.size);
	} else {
	  s_return(sc, x);
	}
      }
      case T_FOREIGN_OBJECT:{
	/* Keep nested calls from GC'ing the arglist */
	retain(sc, sc->args);
	x = sc->object_invoke_callback(sc, _fo_unchecked(code_cell), mk_pointer(sc->args));
	s_return(sc, x);
      }
      case T_VECTOR:
      case T_ARRAYMAP:
      case T_SORTED_SET:
      case T_IMAGE:
      case T_AUDIO:
	if (!sc->args || sc->args == &(sc->_EMPTY)) {
	  Error_0(sc, "Error - Invalid arity");
	}
	if (get_elem(sc, decode_pointer(sc->code), first(sc, sc->args), &x)) {
	  s_return(sc, x);
	} else {
	  nanoclj_cell_t * r = next(sc, sc->args);
	  if (r) {
	    s_return(sc, first(sc, r));
	  } else {
	    Error_0(sc, "Error - No item in collection");
	  }
	}
      case T_CLOSURE:
      case T_MACRO:
      case T_LAZYSEQ:
      case T_DELAY:
	/* Should not accept lazyseq or delay */
	/* make environment */
	new_frame_in_env(sc, closure_env(code_cell));
	x = closure_code(code_cell);
	meta = _cons_metadata(code_cell);
	
	if (meta && _type(meta) == T_STRING) {
	  /* if the function has a name, bind it */
	  new_slot_in_env(sc, mk_pointer(meta), sc->code);
	}
	
#ifndef USE_RECUR_REGISTER
	new_slot_in_env(sc, sc->RECUR, sc->code);
#else
	sc->recur = sc->code;
#endif

	params0 = car(x);
	params = decode_pointer(params0);

	if (_type(params) == T_VECTOR) { /* Clojure style arguments */	  
	  if (!destructure(sc, params, sc->args, seq_length(sc, sc->args), true)) {
	    Error_0(sc, "Error - Wrong number of args passed (2)");	   
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
	    Error_0(sc, "Error - Wrong number of args passed (3)");
	  }
	} else {
	  x = car(x);
	  nanoclj_cell_t * yy = sc->args;
	  for ( ; is_list(x); x = cdr(x), yy = next(sc, yy)) {
	    if (!yy) {
	      Error_0(sc, "Error - not enough arguments (3)");
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
	    Error_0(sc, "Error - syntax error in closure: not a symbol");
	  }
	  sc->code = cdr(closure_code(code_cell));
	}
	sc->args = NULL;
	s_goto(sc, OP_DO);
      }

      fprintf(stderr, "failed to handle apply for type %ld\n", _type(code_cell));
      break;
    }
    default:
      fprintf(stderr, "unknown prim type %ld\n", prim_type(sc->code));
    }
    Error_0(sc, "Error - illegal function");    
    
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
    
    nanoclj_val_t name;
    if (has_name) {
      name = car(x);
      x = cdr(x);
    } else {
      name = mk_nil();
    }
    
    /* make closure. first is code. second is environment */
    s_return(sc, mk_pointer(get_cell(sc, T_CLOSURE, 0, x, sc->envir, decode_pointer(name))));
  }
   
  case OP_QUOTE:               /* quote */
    s_return(sc, car(sc->code));

  case OP_VAR:                 /* var */
    x = car(sc->code);
    if (is_symbol(x)) {
      s_return(sc, mk_pointer(find_slot_in_env(sc, sc->envir, x, true)));
    } else {
      Error_0(sc, "Error - Not a symbol");
    }

  case OP_RESOLVE:                /* resolve */
    if (!unpack_args_1_plus(sc, &arg0, &arg_next)) {
      Error_0(sc, "Error - Invalid arity");
    } else if (arg_next) {
      s_return(sc, mk_pointer(find_slot_in_env(sc, decode_pointer(arg0), first(sc, arg_next), false)));
    } else {
      s_return(sc, mk_pointer(find_slot_in_env(sc, sc->envir, arg0, true)));
    }

  case OP_INTERN:
    if (!unpack_args_2_plus(sc, &arg0, &arg1, &arg_next)) {
      Error_0(sc, "Error - Invalid arity");
    } else if (!is_environment(arg0) || !is_symbol(arg1)) {
      Error_0(sc, "Error - Invalid types");
    } else { /* arg0: namespace, arg1: symbol */
      nanoclj_cell_t * ns = decode_pointer(arg0);
      if (arg_next) {
	s_return(sc, intern(sc, ns, arg1, first(sc, arg_next)));
      } else {
	s_return(sc, intern_symbol(sc, ns, arg1));
      }
    }
    
  case OP_DEF0:{                /* define */    
    x = car(sc->code);
    nanoclj_cell_t * meta = NULL;
    if (caddr(sc->code).as_long != sc->EMPTY.as_long) {
      meta = mk_arraymap(sc, 1);
      set_vector_elem(meta, 0, mk_mapentry(sc, sc->DOC, cadr(sc->code)));
      sc->code = caddr(sc->code);
    } else {
      y = mk_nil();
      sc->code = cadr(sc->code);
    }

    if (!is_symbol(x)) {
      Error_0(sc, "Error - variable is not a symbol");
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
      Error_0(sc, "Error - Invalid arity");
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
    nanoclj_cell_t * input_vec = decode_pointer(car(sc->code));
    if (_type(input_vec) != T_VECTOR) {
      Error_0(sc, "Syntax error");
    }
    nanoclj_cell_t * body = decode_pointer(cdr(sc->code));

    size_t n = _get_size(input_vec) / 2;

    nanoclj_cell_t * values = 0;
    nanoclj_cell_t * args = get_vector_object(sc, T_VECTOR, n);

    for (int i = (int)n - 1; i >= 0; i--) {
      set_vector_elem(args, i, vector_elem(input_vec, 2 * i + 0));
      values = cons(sc, vector_elem(input_vec, 2 * i + 1), values);
    }

    body = cons(sc, mk_pointer(args), body);
    body = cons(sc, sc->LAMBDA, body);
    
    sc->code = mk_pointer(cons(sc, mk_pointer(body), values));
    sc->args = NULL;
    s_goto(sc, OP_EVAL);
  }

  case OP_LET0:                /* let */
    x = car(sc->code);
    
    if (is_vector(x)) {       /* Clojure style */
      nanoclj_cell_t * vec = decode_pointer(x);
      new_frame_in_env(sc, sc->envir);
      s_save(sc, OP_LET1_VEC, mk_long(sc, 0), sc->code);
      
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
    long long i = _lvalue_unchecked(sc->args);
    new_slot_in_env(sc, vector_elem(vec, i), sc->value);
    
    if (i + 2 < _get_size(vec)) {
      s_save(sc, OP_LET1_VEC, mk_long(sc, i + 2), sc->code);
      
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
	Error_0(sc, "Error - Bad syntax of binding spec in let");
      }
      s_save(sc, OP_LET1, sc->args, cdr(sc->code));
      sc->code = cadar(sc->code);
      sc->args = NULL;
      s_goto(sc, OP_EVAL);
    } else {                    /* end */
      sc->args = reverse_in_place(sc, sc->EMPTY, sc->args);
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
          Error_0(sc, "Error - Bad syntax of binding in let");
        if (!is_list(car(x)))
          Error_0(sc, "Error - Bad syntax of binding in let");
        sc->args = cons(sc, caar(x), sc->args);
      }
      /* make closure. first is code. second is environment */
      x = mk_pointer(get_cell(sc, T_CLOSURE, 0, mk_pointer(cons(sc, mk_pointer(reverse_in_place(sc, sc->EMPTY, sc->args)),
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
      Error_0(sc, "Error - syntax error in cond");
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
          Error_0(sc, "Error - syntax error in cond");
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
      s_return(sc, (nanoclj_val_t)kTRUE);
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
      Error_0(sc, "Error - variable is not a symbol");
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
    x = eval(sc, decode_pointer(x));
    s_return(sc, x);
      
  case OP_PAPPLY:              /* apply* */
    if (!unpack_args_2(sc, &arg0, &arg1)) {
      Error_0(sc, "Error - Invalid arity");
    }
    sc->code = arg0;
    sc->args = decode_pointer(arg1);
    s_goto(sc, OP_APPLY);

  case OP_PEVAL:               /* eval */
    if (!unpack_args_1_plus(sc, &arg0, &arg_next)) {
      Error_0(sc, "Error - Invalid arity");
    } else {
      if (arg_next) {
	sc->envir = decode_pointer(first(sc, arg_next));
      }
      sc->code = arg0;
      s_goto(sc, OP_EVAL);
    }

  case OP_RATIONALIZE:
    if (!unpack_args_1(sc, &arg0)) {
      Error_0(sc, "Error - Invalid arity");
    }
    x = arg0;
    if (is_real(x)) {
      if (x.as_double == 0) {
	Error_0(sc, "Divide by zero");
      } else {
	s_return(sc, mk_ratio_from_double(sc, arg0));
      }
    } else {
      s_return(sc, arg0);
    }
    
  case OP_INC:
    if (!unpack_args_1(sc, &arg0)) {
      Error_0(sc, "Error - Invalid arity");
    }
    switch (prim_type(arg0)) {
    case T_INTEGER: s_return(sc, mk_integer(sc, decode_integer(arg0) + 1));
    case T_REAL: s_return(sc, mk_real(arg0.as_double + 1.0));
    case T_CELL: {
      nanoclj_cell_t * c = decode_pointer(arg0);
      switch (_type(c)) {
      case T_LONG: {
	long long v = _lvalue_unchecked(c);
	if (v < LLONG_MAX) {
	  s_return(sc, mk_integer(sc, v + 1));
	} else {
	  Error_0(sc, "Error - Integer overflow");
	}
      }
      case T_RATIO:{
	long long num = to_long(vector_elem(c, 0)), den = to_long(vector_elem(c, 1)), res;
	if (__builtin_saddll_overflow(num, den, &res) == false) {
	  s_return(sc, mk_ratio_long(sc, res, den));
	} else {
	  Error_0(sc, "Error - Integer overflow");
	}
      }
      }
    }
    }
    s_return(sc, mk_nil());

  case OP_DEC:
    if (!unpack_args_1(sc, &arg0)) {
      Error_0(sc, "Error - Invalid arity");
    }
    switch (prim_type(arg0)) {
    case T_INTEGER: s_return(sc, mk_integer(sc, decode_integer(arg0) - 1));
    case T_REAL: s_return(sc, mk_real(arg0.as_double - 1.0));
    case T_CELL: {
      nanoclj_cell_t * c = decode_pointer(arg0);
      switch (_type(c)) {
      case T_LONG:{
	long long v = _lvalue_unchecked(c);
	if (v > LLONG_MIN) {
	  s_return(sc, mk_integer(sc, v - 1));
	} else {
	  Error_0(sc, "Error - Integer overflow");
	}
      case T_RATIO:{
	long long num = to_long(vector_elem(c, 0)), den = to_long(vector_elem(c, 1)), res;
	if (__builtin_ssubll_overflow(num, den, &res) == false) {
	  s_return(sc, mk_ratio_long(sc, res, den));
	} else {
	  Error_0(sc, "Error - Integer overflow");
	}
      }
      }    
      }
    }
    }
    s_return(sc, mk_nil());

  case OP_ADD:                 /* add */
    if (!unpack_args_2(sc, &arg0, &arg1)) {
      Error_0(sc, "Error - Invalid arity");
    } else {
      int tx = prim_type(arg0), ty = prim_type(arg1);
      if (tx == T_REAL || ty == T_REAL) {
	s_return(sc, mk_real(to_double(arg0) + to_double(arg1)));
      } else if (tx == T_INTEGER && ty == T_INTEGER) {
	s_return(sc, mk_integer(sc, (long long)decode_integer(arg0) + (long long)decode_integer(arg1)));
      } else {
	tx = expand_type(arg0, tx);
	ty = expand_type(arg1, ty);
	
	if (tx == T_RATIO || ty == T_RATIO) {
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
	    Error_0(sc, "Error - Integer overflow");
	  }
	}
      }
    }
    
  case OP_SUB:                 /* minus */
    if (!unpack_args_2(sc, &arg0, &arg1)) {
      Error_0(sc, "Error - Invalid arity");
    } else {
      int tx = prim_type(arg0), ty = prim_type(arg1);
      if (tx == T_REAL || ty == T_REAL) {
	s_return(sc, mk_real(to_double(arg0) - to_double(arg1)));
      } else if (tx == T_INTEGER && ty == T_INTEGER) {
	s_return(sc, mk_integer(sc, (long long)decode_integer(arg0) - (long long)decode_integer(arg1)));
      } else {
	tx = expand_type(arg0, tx);
	ty = expand_type(arg1, ty);
	
	if (tx == T_RATIO || ty == T_RATIO) {
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
	    Error_0(sc, "Error - Integer overflow");
	  }
	}
      }
    }
  
  case OP_MUL:                 /* multiply */
    if (!unpack_args_2(sc, &arg0, &arg1)) {
      Error_0(sc, "Error - Invalid arity");
    } else {
      int tx = prim_type(arg0), ty = prim_type(arg1);
      if (tx == T_REAL || ty == T_REAL) {
	s_return(sc, mk_real(to_double(arg0) * to_double(arg1)));
      } else if (tx == T_INTEGER && ty == T_INTEGER) {
	s_return(sc, mk_integer(sc, (long long)decode_integer(arg0) * (long long)decode_integer(arg1)));
      } else {
	tx = expand_type(arg0, tx);
	ty = expand_type(arg1, ty);
            
	if (tx == T_RATIO || ty == T_RATIO) {
	  long long den_x, den_y;
	  long long num_x = get_ratio(arg0, &den_x);
	  long long num_y = get_ratio(arg1, &den_y);
	  long long num, den;
	  if (__builtin_smulll_overflow(num_x, num_y, &num) ||
	      __builtin_smulll_overflow(den_x, den_y, &den)) {
	    Error_0(sc, "Error - Integer overflow");
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
	    Error_0(sc, "Error - Integer overflow");
	  } 
	  s_return(sc, mk_integer(sc, res));
	}
      }
    }
	
  case OP_DIV:                 /* divide */
    if (!unpack_args_2(sc, &arg0, &arg1)) {
      Error_0(sc, "Error - Invalid arity");
    } else {
      int tx = prim_type(arg0), ty = prim_type(arg1);
      if (tx == T_REAL || ty == T_REAL) {
	s_return(sc, mk_real(to_double(arg0) / to_double(arg1)));
      } else if (tx == T_INTEGER && ty == T_INTEGER) {
	int divisor = decode_integer(arg1);
	if (divisor == 0) {
	  Error_0(sc, "Divide by zero");
	} else {
	  int dividend = decode_integer(arg0);
	  if (dividend % divisor == 0) {
	    s_return(sc, mk_integer(sc, dividend / divisor));
	  } else {
	    long long den;
	    long long num = normalize(dividend, divisor, &den);
	    s_return(sc, mk_ratio_long(sc, num, den));
	  }
	}
      } else {
	tx = expand_type(arg0, tx);
	ty = expand_type(arg1, ty);
	
	if (tx == T_RATIO || ty == T_RATIO) {
	  long long den_x, den_y;
	  long long num_x = get_ratio(arg0, &den_x);
	  long long num_y = get_ratio(arg1, &den_y);
	  long long num, den;
	  if (__builtin_smulll_overflow(num_x, den_y, &num) ||
	      __builtin_smulll_overflow(den_x, num_y, &den)) {
	    Error_0(sc, "Error - Integer overflow");
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
	    Error_0(sc, "Divide by zero");
	  } else {
	    long long dividend = to_long(arg0);
	    if (dividend % divisor == 0) {
	      if (dividend == LLONG_MIN && divisor == -1) {
		Error_0(sc, "Error - Integer overflow");
	      } else {
		s_return(sc, mk_integer(sc, dividend / divisor));
	      }
	    } else {
	      long long den;
	      long long num = normalize(dividend, divisor, &den);
	      s_return(sc, mk_ratio_long(sc, num, den));
	    }
	  }
	}
      }
    }
    
  case OP_REM:                 /* rem */
    if (!unpack_args_2(sc, &arg0, &arg1)) {
      Error_0(sc, "Error - Invalid arity");
    } else {
      int tx = prim_type(arg0), ty = prim_type(arg1);
      if (tx == T_REAL || ty == T_REAL) {
	double a = to_double(arg0), b = to_double(arg1);
	if (b == 0) {
	  Error_0(sc, "Error - division by zero");
	}
	double res = fmod(a, b);
	/* remainder should have same sign as first operand */
	if (res > 0 && a < 0) res -= fabs(b);
	else if (res < 0 && a > 0) res += fabs(b);
	
	s_return(sc, mk_real(res));
      } else {
	long long a = to_long(arg0), b = to_long(arg1);
	if (b == 0) {
	  Error_0(sc, "Error - division by zero");
	} else {
	  long long res = a % b;
	  /* remainder should have same sign as first operand */
	  if (res > 0 && a < 0) res -= labs(b);
	  else if (res < 0 && a > 0) res += labs(b);
	  
	  s_return(sc, mk_integer(sc, res));
	}
      }
    }
    
  case OP_FIRST:                 /* first */
    if (!unpack_args_1(sc, &arg0)) {
      Error_0(sc, "Error - Invalid arity");
    } else if (!is_cell(arg0)) {
      Error_0(sc, "Error - value is not ISeqable");    
    } else {
      s_return(sc, first(sc, decode_pointer(arg0)));
    }

  case OP_SECOND:
    if (!unpack_args_1(sc, &arg0)) {
      Error_0(sc, "Error - Invalid arity");
    } else if (!is_cell(arg0)) {
      Error_0(sc, "Error - value is not ISeqable");
    } else {
      s_return(sc, second(sc, decode_pointer(arg0)));
    }

  case OP_REST:                 /* rest */
  case OP_NEXT:
    if (!unpack_args_1(sc, &arg0)) {
      Error_0(sc, "Error - Invalid arity");
    } else if (is_cell(arg0)) {
      nanoclj_cell_t * c = decode_pointer(arg0);
      if (is_seqable_type(_type(c))) {
	if (op == OP_NEXT) {
	  s_return(sc, mk_pointer(next(sc, c)));
	} else {
	  s_return(sc, mk_pointer(rest(sc, c)));
	}
      }
    }
    Error_0(sc, "Error - value is not ISeqable");

  case OP_GET:             /* get */
    if (!unpack_args_2_plus(sc, &arg0, &arg1, &arg_next)) {
      Error_0(sc, "Error - Invalid arity");
    } else if (is_cell(arg0) && get_elem(sc, decode_pointer(arg0), arg1, &x)) {
      s_return(sc, x);
    } else {
      /* Not found => return the not-found value if provided */
      s_return(sc, first(sc, arg_next));
    }

  case OP_CONTAINSP:
    if (!unpack_args_2(sc, &arg0, &arg1)) {
      Error_0(sc, "Error - Invalid arity");
    }
    s_retbool(is_cell(arg0) && get_elem(sc, decode_pointer(arg0), arg1, NULL));
    
  case OP_CONJ:             /* -conj */
    if (!unpack_args_2(sc, &arg0, &arg1)) {
      Error_0(sc, "Error - Invalid arity");
    } else if (!is_cell(arg0)) {
      Error_0(sc, "Error - No protocol method ICollection.-conj defined");      
    } else {
      nanoclj_cell_t * coll = decode_pointer(arg0);
      y = arg1;
      switch (_type(coll)) {
      case T_VECTOR:{
	size_t vector_len = _get_size(coll);

	/* If MapEntry is added to vector, it is an index value pair */
	if (type(y) == T_MAPENTRY) {
	  nanoclj_cell_t * y2 = decode_pointer(y);
	  long long index = to_long_w_def(first(sc, y2), -1);
	  nanoclj_val_t value = second(sc, y2);
	  if (index < 0 || index >= vector_len) {
	    Error_0(sc, "Error - Index out of bounds");     
	  } else {
	    nanoclj_cell_t * vec = copy_vector(sc, coll);
	    if (sc->no_memory) {
	      s_return(sc, sc->sink);
	    }
	    set_vector_elem(vec, index, value);
	    s_return(sc, mk_pointer(vec));
	  }
	} else {
	  s_return(sc, mk_pointer(conjoin(sc, coll, y)));
	}
      }
	
      case T_ARRAYMAP:{
	/* TODO: check that y is actually a vector */
	nanoclj_cell_t * p = decode_pointer(y);
	nanoclj_val_t key = first(sc, p), val = second(sc, p);
	size_t vector_len = _get_size(coll);
	for (size_t i = 0; i < vector_len; i++) {
	  nanoclj_cell_t * e = decode_pointer(vector_elem(coll, i));
	  if (equals(sc, key, vector_elem(e, 0))) {
	    nanoclj_cell_t * vec = copy_vector(sc, coll);
	    /* Rebuild the mapentry in case the input is not mapentry */
	    set_vector_elem(vec, i, mk_mapentry(sc, key, val));
	    s_return(sc, mk_pointer(vec));
	  }
	}
	/* Not found */
	s_return(sc, mk_pointer(conjoin(sc, coll, y)));
      }
      
      case T_SORTED_SET:{
	if (!get_elem(sc, coll, y, NULL)) {
	  nanoclj_cell_t * vec = conjoin(sc, coll, y);
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
	s_return(sc, mk_pointer(conjoin(sc, coll, y)));
	
      default:
	Error_0(sc, "Error - No protocol method ICollection.-conj defined");
      }
    }

  case OP_SUBVEC:
    if (!unpack_args_2_plus(sc, &arg0, &arg1, &arg_next)) {
      Error_0(sc, "Error - Invalid arity");
    } else if (!is_cell(arg0)) {
      Error_0(sc, "Error - Not a vector");
    } else {
      nanoclj_cell_t * c = decode_pointer(arg0);
      long long start = to_long(arg1);
      long long end;
      if (arg_next) {
	end = to_long(first(sc, arg_next));
      } else {
	end = _get_size(c);
      }
      if (end < start) end = start;
      s_return(sc, mk_pointer(subvec(sc, c, start, end - start)));
    }
  case OP_NOT:                 /* not */
    if (!unpack_args_1(sc, &arg0)) {
      Error_0(sc, "Error - Invalid arity");
    }
    s_retbool(is_false(arg0));
    
  case OP_EQUIV:                  /* equiv */
    if (!unpack_args_2(sc, &arg0, &arg1)) {
      Error_0(sc, "Error - Invalid arity");
    }
    s_retbool(equiv(arg0, arg1));
    
  case OP_LT:                  /* lt */
    if (!unpack_args_2(sc, &arg0, &arg1)) {
      Error_0(sc, "Error - Invalid arity");
    }
    s_retbool(compare(arg0, arg1) < 0);

  case OP_GT:                  /* gt */
    if (!unpack_args_2(sc, &arg0, &arg1)) {
      Error_0(sc, "Error - Invalid arity");
    }
    s_retbool(compare(arg0, arg1) > 0);
  
  case OP_LE:                  /* le */
    if (!unpack_args_2(sc, &arg0, &arg1)) {
      Error_0(sc, "Error - Invalid arity");
    }
    s_retbool(compare(arg0, arg1) <= 0);

  case OP_GE:                  /* ge */
    if (!unpack_args_2(sc, &arg0, &arg1)) {
      Error_0(sc, "Error - Invalid arity");
    }
    s_retbool(compare(arg0, arg1) >= 0);
    
  case OP_EQ:                  /* equals? */
    if (!unpack_args_2(sc, &arg0, &arg1)) {
      Error_0(sc, "Error - Invalid arity");
    }
    s_retbool(equals(sc, arg0, arg1));

  case OP_BIT_AND:
    if (!unpack_args_2(sc, &arg0, &arg1)) {
      Error_0(sc, "Error - Invalid arity");
    }
    s_return(sc, mk_integer(sc, to_long(arg0) & to_long(arg1)));

  case OP_BIT_OR:
    if (!unpack_args_2(sc, &arg0, &arg1)) {
      Error_0(sc, "Error - Invalid arity");
    }
    s_return(sc, mk_integer(sc, to_long(arg0) | to_long(arg1)));

  case OP_BIT_XOR:
    if (!unpack_args_2(sc, &arg0, &arg1)) {
      Error_0(sc, "Error - Invalid arity");
    }
    s_return(sc, mk_integer(sc, to_long(arg0) ^ to_long(arg1)));

  case OP_BIT_SHIFT_LEFT:
    if (!unpack_args_2(sc, &arg0, &arg1)) {
      Error_0(sc, "Error - Invalid arity");
    }
    s_return(sc, mk_integer(sc, to_long(arg0) << to_long(arg1)));

  case OP_BIT_SHIFT_RIGHT:
    if (!unpack_args_2(sc, &arg0, &arg1)) {
      Error_0(sc, "Error - Invalid arity");
    }
    s_return(sc, mk_integer(sc, to_long(arg0) >> to_long(arg1)));
    
  case OP_TYPE:                /* type */
  case OP_CLASS:
    if (!unpack_args_1(sc, &arg0)) {
      Error_0(sc, "Error - Invalid arity");
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
    
  case OP_INSTANCEP:
    if (!unpack_args_2(sc, &arg0, &arg1)) {
      Error_0(sc, "Error - Invalid arity");
    } else { /* arg0: type, arg1: object */
      if (!is_nil(arg1)) {
	nanoclj_cell_t * t0 = decode_pointer(arg0);
	nanoclj_cell_t * t1 = get_type_object(sc, arg1);
	while (t1) {
	  if (t0 == t1) {
	    s_retbool(true);
	  }
	  t1 = next(sc, t1);
	}
      }
      s_retbool(false);
    }

  case OP_IDENTICALP:                  /* identical? */
    if (!unpack_args_2(sc, &arg0, &arg1)) {
      Error_0(sc, "Error - Invalid arity");
    }
    s_retbool(arg0.as_long == arg1.as_long);

  case OP_DEREF:
    x = first(sc, sc->args);
    if (!is_cell(x)) {
      s_return(sc, x);
    } else {
      nanoclj_cell_t * c = decode_pointer(x);
      if (!c) {
	s_return(sc, mk_nil());
      }
      if (_type(c) == T_LAZYSEQ || _type(c) == T_DELAY) {
	if (!_is_realized(c)) {
	  /* Should change type to closure here */
	  s_save(sc, OP_SAVE_FORCED, NULL, x);
	  sc->code = x;
	  sc->args = NULL;
	  s_goto(sc, OP_APPLY);
	} else if (_type(c) == T_LAZYSEQ && is_nil(_car(c)) && is_nil(_cdr(c))) {
	  s_return(sc, sc->EMPTY);
	}
      }
      if (_type(c) == T_DELAY) {
	s_return(sc, _car(c));
      } else if (_type(c) == T_VAR) {
	s_return(sc, vector_elem(c, 0));
      } else {
	s_return(sc, x);
      }
    }

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
      Error_0(sc, "Error - Invalid arity");
    }
    print_primitive(sc, arg0, op == OP_PR, get_out_port(sc));
    s_return(sc, mk_nil());
  
  case OP_FORMAT:
    if (!unpack_args_1_plus(sc, &arg0, &arg_next)) {
      Error_0(sc, "Error - Invalid arity");
    }
    s_return(sc, mk_format(sc, to_strview(arg0), arg_next));

  case OP_ERR0:                /* throw */
    sc->retcode = -1;
    set_basic_style(stderr, 1);
    set_basic_style(stderr, 31);
    {
      nanoclj_val_t err = get_err_port(sc);
      assert(is_writer(err));
      strview_t sv = to_strview(first(sc, sc->args));
      putchars(sc, sv.ptr, sv.size, err);
      putchars(sc, "\n", 1, err);
    }
    reset_color(stderr);
#if 1
    s_goto(sc, OP_T0LVL);
#else
    return sc->EMPTY;
#endif
    
  case OP_CLOSE:        /* close */
    if (!unpack_args_1(sc, &arg0)) {
      Error_0(sc, "Error - Invalid arity");
    }
    switch (type(arg0)) {
    case T_READER:
    case T_WRITER:
      port_close(sc, port_unchecked(arg0));
    }
    s_return(sc, mk_nil());

    /* ========== reading part ========== */
    
  case OP_RDSEXPR:
    x = get_in_port(sc);
    if (!is_reader(x)) {
      Error_0(sc, "Error - not a reader");
    } else {
      nanoclj_port_t * inport = port_unchecked(x);

      switch (sc->tok) {
      case TOK_EOF:
	s_return(sc, mk_character(EOF));
	
      case TOK_FN:
      case TOK_LPAREN:
	if (sc->tok == TOK_FN) {
	  s_save(sc, OP_RDFN, NULL, sc->EMPTY);
	}
	sc->tok = token(sc, inport);
	if (sc->tok == TOK_RPAREN) {       /* Empty list */
	  s_return(sc, sc->EMPTY);
	} else if (sc->tok == TOK_DOT) {
	  Error_0(sc, "Error - illegal dot expression");
	} else {
	  sc->nesting_stack[sc->file_i]++;
	  s_save(sc, OP_RDLIST, NULL, sc->EMPTY);
	  s_goto(sc, OP_RDSEXPR);
	}
	
      case TOK_VEC:
	sc->tok = token(sc, inport);
	if (sc->tok == TOK_RSQUARE) {	/* Empty vector */
	  s_return(sc, mk_pointer(sc->EMPTYVEC));
	} else if (sc->tok == TOK_DOT) {
	  Error_0(sc, "Error - illegal dot expression");
	} else {
	  sc->nesting_stack[sc->file_i]++;
	  s_save(sc, OP_RDVEC_ELEMENT, sc->EMPTYVEC, sc->EMPTY);
	  s_goto(sc, OP_RDSEXPR);
	}      
	
      case TOK_SET:
      case TOK_MAP:
	if (sc->tok == TOK_SET) {
	  s_save(sc, OP_RDSET, NULL, sc->EMPTY);
	} else {
	  s_save(sc, OP_RDMAP, NULL, sc->EMPTY);
	}
	sc->tok = token(sc, inport);
	if (sc->tok == TOK_RCURLY) {     /* Empty map or set */
	  s_return(sc, sc->EMPTY);
	} else if (sc->tok == TOK_DOT) {
	  Error_0(sc, "Error - illegal dot expression");
	} else {
	  sc->nesting_stack[sc->file_i]++;
	  s_save(sc, OP_RDMAP_ELEMENT, NULL, sc->EMPTY);
	  s_goto(sc, OP_RDSEXPR);
	}
	
      case TOK_QUOTE:
	s_save(sc, OP_RDQUOTE, NULL, sc->EMPTY);
	sc->tok = token(sc, inport);
	s_goto(sc, OP_RDSEXPR);
	
      case TOK_DEREF:
	s_save(sc, OP_RDDEREF, NULL, sc->EMPTY);
	sc->tok = token(sc, inport);
	s_goto(sc, OP_RDSEXPR);
	
      case TOK_VAR:
	s_save(sc, OP_RDVAR, NULL, sc->EMPTY);
	sc->tok = token(sc, inport);
	s_goto(sc, OP_RDSEXPR);      
	
      case TOK_BQUOTE:
	sc->tok = token(sc, inport);
	if (sc->tok == TOK_VEC) {
	  s_save(sc, OP_RDQQUOTEVEC, NULL, sc->EMPTY);
	  sc->tok = TOK_LPAREN; // ?
	  s_goto(sc, OP_RDSEXPR);
	} else {
	  s_save(sc, OP_RDQQUOTE, NULL, sc->EMPTY);
	}
	s_goto(sc, OP_RDSEXPR);
	
      case TOK_COMMA:
	s_save(sc, OP_RDUNQUOTE, NULL, sc->EMPTY);
	sc->tok = token(sc, inport);
	s_goto(sc, OP_RDSEXPR);
      case TOK_ATMARK:
	s_save(sc, OP_RDUQTSP, NULL, sc->EMPTY);
	sc->tok = token(sc, inport);
	s_goto(sc, OP_RDSEXPR);
      case TOK_PRIMITIVE:
	x = mk_primitive(sc, readstr_upto(sc, DELIMITERS));
	if (is_nil(x)) {
	  Error_0(sc, "Invalid number format");
	} else {
	  s_return(sc, x);
	}
      case TOK_DQUOTE:
	x = readstrexp(sc);
	if (is_false(x)) {
	  Error_0(sc, "Error - reading string");
	}
	s_return(sc, x);
	
      case TOK_REGEX:
	x = readstrexp(sc);
	if (is_false(x)) {
	  Error_0(sc, "Error - reading regex");
	}
	s_return(sc, mk_pointer(cons(sc, sc->REGEX, cons(sc, x, NULL))));
	
      case TOK_CHAR_CONST:
	if ((x = mk_char_const(sc, readstr_upto(sc, DELIMITERS))).as_long == sc->EMPTY.as_long) {
	  Error_0(sc, "Error - undefined character literal");
	} else {
	  s_return(sc, x);
	}
	
      case TOK_TAG:{
	const char * tag = readstr_upto(sc, DELIMITERS);
	nanoclj_cell_t * f = find_slot_in_env(sc, sc->envir, sc->TAG_HOOK, true);
	if (f) {
	  nanoclj_val_t hook = slot_value_in_env(f);
	  if (!is_nil(hook)) {
	    sc->code = mk_pointer(cons(sc, hook, cons(sc, mk_string(sc, tag), NULL)));
	    s_goto(sc, OP_EVAL);
	  }
	}
	sprintf(sc->errbuff, "No reader function for tag %s", tag);
	Error_0(sc, sc->errbuff);
      }
	
      case TOK_IGNORE:
	readstr_upto(sc, DELIMITERS);
	sc->tok = token(sc, inport);
	if (sc->tok == TOK_EOF) {
	  s_return(sc, mk_character(EOF));
	}
	s_goto(sc, OP_RDSEXPR);      
	
      case TOK_SHARP_CONST:
	if ((x = mk_sharp_const(sc, readstr_upto(sc, DELIMITERS))).as_long == sc->EMPTY.as_long) {
	  Error_0(sc, "Error - undefined sharp expression");
	} else {
	  s_return(sc, x);
	}
	
      default:
	Error_0(sc, "Error - illegal token");
      }
      break;
    }
    
  case OP_RDLIST:
    x = get_in_port(sc);
    if (!is_reader(x)) {
      Error_0(sc, "Error - not a reader");
    } else {
      nanoclj_port_t * inport = port_unchecked(x);
      sc->args = cons(sc, sc->value, sc->args);
      sc->tok = token(sc, inport);
      if (sc->tok == TOK_EOF) {
	s_return(sc, mk_character(EOF));
      } else if (sc->tok == TOK_RPAREN) {
	sc->nesting_stack[sc->file_i]--;
	s_return(sc, mk_pointer(reverse_in_place(sc, sc->EMPTY, sc->args)));
      } else if (sc->tok == TOK_DOT) {
	s_save(sc, OP_RDDOT, sc->args, sc->EMPTY);
	sc->tok = token(sc, inport);
	s_goto(sc, OP_RDSEXPR);
      } else {
	s_save(sc, OP_RDLIST, sc->args, sc->EMPTY);
	s_goto(sc, OP_RDSEXPR);
      }

    }
    
  case OP_RDVEC_ELEMENT:
    x = get_in_port(sc);
    if (!is_reader(x)) {
      Error_0(sc, "Error - not a reader");
    } else {
      nanoclj_port_t * inport = port_unchecked(x);
      sc->args = conjoin(sc, sc->args, sc->value);
      sc->tok = token(sc, inport);
      if (sc->tok == TOK_EOF) {
	s_return(sc, mk_character(EOF));
      } else if (sc->tok == TOK_RSQUARE) {
	sc->nesting_stack[sc->file_i]--;
	s_return(sc, mk_pointer(sc->args));
      } else {
	s_save(sc, OP_RDVEC_ELEMENT, sc->args, sc->EMPTY);
	s_goto(sc, OP_RDSEXPR);
      }
    }

  case OP_RDMAP_ELEMENT:
    x = get_in_port(sc);
    if (!is_reader(x)) {
      Error_0(sc, "Error - not a reader");
    } else {
      nanoclj_port_t * inport = port_unchecked(x);
      sc->args = cons(sc, sc->value, sc->args);
      sc->tok = token(sc, inport);
      if (sc->tok == TOK_EOF) {
	s_return(sc, mk_character(EOF));
      } else if (sc->tok == TOK_RCURLY) {
	sc->nesting_stack[sc->file_i]--;
	s_return(sc, mk_pointer(reverse_in_place(sc, sc->EMPTY, sc->args)));
      } else {
	s_save(sc, OP_RDMAP_ELEMENT, sc->args, sc->EMPTY);
	s_goto(sc, OP_RDSEXPR);
      }
    }
    
  case OP_RDDOT:
    x = get_in_port(sc);
    if (!is_reader(x)) {
      Error_0(sc, "Error - not a reader");
    } else {
      nanoclj_port_t * inport = port_unchecked(x);
      if (token(sc, inport) != TOK_RPAREN) {
	Error_0(sc, "Error - illegal dot expression");
      } else {
	sc->nesting_stack[sc->file_i]--;
	s_return(sc, mk_pointer(reverse_in_place(sc, sc->value, sc->args)));
      }
    }

  case OP_RDQUOTE:
    s_return(sc, mk_pointer(cons(sc, sc->QUOTE, cons(sc, sc->value, NULL))));

  case OP_RDDEREF:
    s_return(sc, mk_pointer(cons(sc, sc->DEREF, cons(sc, sc->value, NULL))));

  case OP_RDVAR:
    s_return(sc, mk_pointer(cons(sc, sc->VAR, cons(sc, sc->value, NULL))));
    
  case OP_RDQQUOTE:
    s_return(sc, mk_pointer(cons(sc, sc->QQUOTE, cons(sc, sc->value, NULL))));
    
  case OP_RDQQUOTEVEC:
    s_return(sc, mk_pointer(cons(sc, def_symbol(sc, "apply"),
				 cons(sc, def_symbol(sc, "vector"),
				      cons(sc, mk_pointer(cons(sc, sc->QQUOTE,
							       cons(sc, sc->value, NULL))), NULL)))));
    
  case OP_RDUNQUOTE:
    s_return(sc, mk_pointer(cons(sc, sc->UNQUOTE, cons(sc, sc->value, NULL))));

  case OP_RDUQTSP:
    s_return(sc, mk_pointer(cons(sc, sc->UNQUOTESP, cons(sc, sc->value, NULL))));
    
  case OP_RDFN:{
    bool has_rest;
    int n_args = get_literal_fn_arity(sc, sc->value, &has_rest);
    size_t vsize = n_args + (has_rest ? 2 : 0);
    nanoclj_cell_t * vec = get_vector_object(sc, T_VECTOR, vsize);
    if (n_args >= 1) set_vector_elem(vec, 0, sc->ARG1);
    if (n_args >= 2) set_vector_elem(vec, 1, sc->ARG2);
    if (n_args >= 3) set_vector_elem(vec, 2, sc->ARG3);
    if (has_rest) {
      set_vector_elem(vec, n_args, sc->AMP);
      set_vector_elem(vec, n_args + 1, sc->ARG_REST);
    }
    s_return(sc, mk_pointer(cons(sc, sc->LAMBDA, cons(sc, mk_pointer(vec), cons(sc, sc->value, NULL)))));
  }
    
  case OP_RDSET:
    if (sc->value.as_long == sc->EMPTY.as_long) {
      s_return(sc, mk_pointer(mk_sorted_set(sc, 0)));
    } else {
      s_return(sc, mk_pointer(cons(sc, sc->SORTED_SET, decode_pointer(sc->value))));
    }
    
  case OP_RDMAP:
    if (sc->value.as_long == sc->EMPTY.as_long) {
      s_return(sc, mk_pointer(mk_arraymap(sc, 0)));
    } else {
      s_return(sc, mk_pointer(cons(sc, sc->ARRAY_MAP, decode_pointer(sc->value))));
    }
    
  case OP_SEQ:
    if (!unpack_args_1(sc, &arg0)) {
      Error_0(sc, "Error - Invalid arity");
    } else if (is_cell(arg0)) {
      nanoclj_cell_t * c = decode_pointer(arg0);
      if (is_seqable_type(_type(c))) {
	s_return(sc, mk_pointer(seq(sc, c)));
      }
    }
    Error_0(sc, "Error - value is not ISeqable");

  case OP_RSEQ:
    if (!unpack_args_1(sc, &arg0)) {
      Error_0(sc, "Error - Invalid arity");
    } else if (is_cell(arg0)) {
      nanoclj_cell_t * c = decode_pointer(arg0);
      int t = _type(c);
      if (t == T_VECTOR || t == T_ARRAYMAP || t == T_SORTED_SET || t == T_STRING ||
	  t == T_CHAR_ARRAY || t == T_FILE) {
	s_return(sc, mk_pointer(rseq(sc, c)));	
      }
    }
    Error_0(sc, "Error - value is not reverse seqable");

  case OP_SEQP:
    if (!unpack_args_1(sc, &arg0)) {
      Error_0(sc, "Error - Invalid arity");
    } else if (is_cell(arg0)) {
      nanoclj_cell_t * c = decode_pointer(arg0);
      if (c) {
	s_retbool(_is_sequence(c) || _type(c) == T_LIST || _type(c) == T_LAZYSEQ);
      }
    }
    s_retbool(false);

  case OP_EMPTYP:
    if (!unpack_args_1(sc, &arg0)) {
      Error_0(sc, "Error - Invalid arity");
    } else if (is_cell(arg0)) {
      nanoclj_cell_t * c = decode_pointer(arg0);
      if (is_seqable_type(_type(c))) {
	s_retbool(is_empty(sc, c));
      }
    }
    Error_0(sc, "Error - value is not ISeqable");
    
  case OP_HASH:
    if (!unpack_args_1(sc, &arg0)) {
      Error_0(sc, "Error - Invalid arity");
    }
    s_return(sc, mk_int(hasheq(arg0)));

  case OP_COMPARE:
    if (!unpack_args_2(sc, &arg0, &arg1)) {
      Error_0(sc, "Error - Invalid arity");
    }
    s_return(sc, mk_int(compare(arg0, arg1)));
	     
  case OP_SORT:{
    if (!unpack_args_1(sc, &arg0)) {
      Error_0(sc, "Error - Invalid arity");
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
      nanoclj_cell_t * vec = get_vector_object(sc, T_VECTOR, l);
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
      Error_0(sc, "Error - Invalid arity");
    }
    nanoclj_val_t input = arg0;
    nanoclj_val_t options = arg1;
    utf8proc_uint8_t * dest = NULL;
    strview_t sv = to_strview(input);
    utf8proc_ssize_t s = utf8proc_map((const unsigned char *)sv.ptr,
				      (utf8proc_ssize_t)sv.size,
				      &dest,
				      (utf8proc_option_t)to_long(options)
				      );
    if (s >= 0) {
      nanoclj_val_t r = mk_pointer(get_string_object(sc, T_STRING, (char *)dest, s, 0));
      free(dest);
      s_return(sc, r);
    } else {
      s_return(sc, mk_nil());
    }
  }

  case OP_META:
    if (!unpack_args_1(sc, &arg0)) {
      Error_0(sc, "Error - Invalid arity");
    }
    if (is_cell(arg0)) {
      nanoclj_cell_t * c = decode_pointer(arg0);
      if (c) s_return(sc, mk_pointer(get_metadata(c)));
    }
    s_return(sc, mk_nil());
    
  case OP_IN_NS:
    if (!unpack_args_1(sc, &arg0)) {
      Error_0(sc, "Error - Invalid arity");
    } else {
      nanoclj_cell_t * var = find_slot_in_env(sc, sc->root_env, arg0, true);
      nanoclj_cell_t * ns;
      if (var) {
	ns = decode_pointer(slot_value_in_env(var));
      } else {
	ns = def_namespace_with_sym(sc, arg0);
      }
      bool r = _s_return(sc, mk_pointer(ns));
      sc->envir = sc->global_env = ns;
      return r;
    }
    
  case OP_RE_PATTERN:
    if (!unpack_args_1(sc, &arg0)) {
      Error_0(sc, "Error - Invalid arity");
    }
    x = arg0;
    /* TODO: construct a regex */
    s_return(sc, x);

  case OP_ADD_WATCH:
    if (!unpack_args_3(sc, &arg0, &arg1, &arg2)) {
      Error_0(sc, "Error - Invalid arity");
    }
    if (!is_cell(arg0)) {
      Error_0(sc, "Error - Cannot add watch to a primitive");
    } else {
      nanoclj_cell_t * c = decode_pointer(arg0);
      if (_type(c) != T_VAR) {
	Error_0(sc, "Error - Watches can only be added to Vars");
      } else {
	nanoclj_cell_t * md = _so_vector_metadata(c);
	if (!md) md = mk_arraymap(sc, 0);
	nanoclj_val_t w;
	if (get_elem(sc, md, sc->WATCHES, &w)) {
	  // TODO: append watch
	} else {
	  md = conjoin(sc, md, mk_mapentry(sc, sc->WATCHES, mk_pointer(cons(sc, arg2, NULL))));
	}
	_so_vector_metadata(c) = md;
	s_return(sc, arg0);
      }      
    }
    break;

  case OP_REALIZEDP:
    if (!unpack_args_1(sc, &arg0)) {
      Error_0(sc, "Error - Invalid arity");
    } else if (is_cell(arg0)) {
      nanoclj_cell_t * c = decode_pointer(arg0);
      if (_type(c) == T_LAZYSEQ || _type(c) == T_DELAY) {
	s_retbool(_is_realized(c));
      }
    }
    s_retbool(true);

  case OP_NAME:
    if (!unpack_args_1(sc, &arg0)) {
      Error_0(sc, "Error - Invalid arity");
    } else {
      s_return(sc, mk_string_from_sv(sc, to_strview(arg0)));
    }
    
  case OP_SET_COLOR:
    if (!unpack_args_1(sc, &arg0)) {
      Error_0(sc, "Error - Invalid arity");
    } else if (is_vector(arg0)) {
      set_color(sc, decode_pointer(arg0), get_out_port(sc));
    }
    s_return(sc, mk_nil());

  case OP_SET_FONT_SIZE:
    if (!unpack_args_1(sc, &arg0)) {
      Error_0(sc, "Error - Invalid arity");
    }
#if NANOCLJ_HAS_CANVAS
    x = get_out_port(sc);
    if (is_canvas(x)) {
      canvas_set_font_size(canvas_unchecked(x), to_double(arg0) * sc->window_scale_factor);
    }
#endif
    s_return(sc, mk_nil());
    
  case OP_SET_LINE_WIDTH:
    if (!unpack_args_1(sc, &arg0)) {
      Error_0(sc, "Error - Invalid arity");
    }
#if NANOCLJ_HAS_CANVAS
    x = get_out_port(sc);
    if (is_canvas(x)) {
      /* Do not scale line width whit DPI scale factor */
      canvas_set_line_width(canvas_unchecked(x), to_double(arg0));
    }
#endif
    s_return(sc, mk_nil());

  case OP_MOVETO:
    if (!unpack_args_2(sc, &arg0, &arg1)) {
      Error_0(sc, "Error - Invalid arity");
    }
#if NANOCLJ_HAS_CANVAS
    x = get_out_port(sc);
    if (is_canvas(x)) {
      canvas_move_to(canvas_unchecked(x), to_double(arg0) * sc->window_scale_factor, to_double(arg1) * sc->window_scale_factor);
    }
#endif
    s_return(sc, mk_nil());
    
  case OP_LINETO:
    if (!unpack_args_2(sc, &arg0, &arg1)) {
      Error_0(sc, "Error - Invalid arity");
    }
#if NANOCLJ_HAS_CANVAS
    x = get_out_port(sc);
    if (is_canvas(x)) {
      canvas_line_to(canvas_unchecked(x), to_double(arg0) * sc->window_scale_factor, to_double(arg1) * sc->window_scale_factor);
    }
#endif
    s_return(sc, mk_nil());

  case OP_ARC:
    if (!unpack_args_5(sc, &arg0, &arg1, &arg2, &arg3, &arg4)) {
      Error_0(sc, "Error - Invalid arity");
    }
#if NANOCLJ_HAS_CANVAS
    x = get_out_port(sc);
    if (is_canvas(x)) {
      canvas_arc(canvas_unchecked(x), to_double(arg0) * sc->window_scale_factor, to_double(arg1) * sc->window_scale_factor,
		 to_double(arg2) * sc->window_scale_factor, to_double(arg3), to_double(arg4));
    }
#endif
    s_return(sc, mk_nil());

  case OP_CLOSE_PATH:
    if (!unpack_args_0(sc)) {
      Error_0(sc, "Error - Invalid arity");
    }
#if NANOCLJ_HAS_CANVAS
    x = get_out_port(sc);
    if (is_canvas(x)) {
      canvas_close_path(canvas_unchecked(x));
    }
#endif
    s_return(sc, mk_nil());

  case OP_STROKE:
    if (!unpack_args_0(sc)) {
      Error_0(sc, "Error - Invalid arity");
    }
#if NANOCLJ_HAS_CANVAS
    x = get_out_port(sc);
    if (is_canvas(x)) {
      canvas_stroke(canvas_unchecked(x));
    }
#endif
    s_return(sc, mk_nil());

  case OP_FILL:
    if (!unpack_args_0(sc)) {
      Error_0(sc, "Error - Invalid arity");
    }
#if NANOCLJ_HAS_CANVAS
    x = get_out_port(sc);
    if (is_canvas(x)) {
      canvas_stroke(canvas_unchecked(x));
    }
#endif
    s_return(sc, mk_nil());

  case OP_GET_TEXT_EXTENTS:
    if (!unpack_args_1(sc, &arg0)) {
      Error_0(sc, "Error - Invalid arity");
    }
#if NANOCLJ_HAS_CANVAS
    x = get_out_port(sc);
    if (is_canvas(x)) {
      double width, height;
      canvas_get_text_extents(sc, canvas_unchecked(x), to_strview(arg0), &width, &height);
      s_return(sc, mk_vector_2d(sc, width / sc->window_scale_factor, height / sc->window_scale_factor));
    }
#endif
    
  case OP_SAVE:
    if (!unpack_args_0(sc)) {
      Error_0(sc, "Error - Invalid arity");
    }
#if NANOCLJ_HAS_CANVAS
    x = get_out_port(sc);
    if (is_canvas(x)) {
      canvas_save(canvas_unchecked(x));
    }
#endif
    s_return(sc, mk_nil());

  case OP_RESTORE:
    if (!unpack_args_0(sc)) {
      Error_0(sc, "Error - Invalid arity");
    }
    x = get_out_port(sc);
    if (is_canvas(x)) {
#if NANOCLJ_HAS_CANVAS
      canvas_restore(canvas_unchecked(x));
#endif
    } else if (is_writer(x)) {
      nanoclj_port_t * pt = port_unchecked(x); 
      if (pt->kind & port_file) {
	FILE * fh = pt->rep.stdio.file;
	reset_color(fh);
      } else if ((pt->kind & port_callback) && pt->rep.callback.restore) {
	pt->rep.callback.restore(sc->ext_data);
      }
    }
    s_return(sc, mk_nil());

  case OP_RESIZE:
    if (!unpack_args_2(sc, &arg0, &arg1)) {
      Error_0(sc, "Error - Invalid arity");      
    }
    x = get_out_port(sc);
    if (is_canvas(x)) {
      nanoclj_val_t tmp = mk_canvas(sc, to_int(arg0), to_int(arg1));
      canvas_unchecked(x) = canvas_unchecked(tmp);
      canvas_unchecked(tmp) = NULL;      
    }
    s_return(sc, mk_nil());

  case OP_FLUSH:
    x = get_out_port(sc);
    if (is_canvas(x)) {
      canvas_flush(canvas_unchecked(x));
      if (x.as_long == sc->active_element.as_long) {
	print_primitive(sc, x, false, sc->active_element_target);
      }
    } else if (is_writer(x)) {
      nanoclj_port_t * pt = port_unchecked(x);
      if (pt->kind & port_file) {
	fflush(pt->rep.stdio.file);
      }
    }
    s_return(sc, mk_nil());

  default:
    sprintf(sc->strbuff, "%d: illegal operator", sc->op);
    Error_0(sc, sc->strbuff);
  }
  return false;                 /* NOTREACHED */
}
 
/* kernel of this interpreter */
static void Eval_Cycle(nanoclj_t * sc, enum nanoclj_opcodes op) {
  sc->op = op;
  for (;;) {    
    ok_to_freely_gc(sc);
    if (!opexe(sc, (enum nanoclj_opcodes) sc->op)) {
      return;
    }
    if (sc->no_memory) {
      fprintf(stderr, "No memory!\n");
      sc->retcode = 9;
      return;
    }
  }
}

/* ========== Initialization of internal keywords ========== */

static inline void assign_syntax(nanoclj_t * sc, const char *name, unsigned int syntax) {
  strview_t sv = { name, strlen(name) };
  oblist_add_item(sc, sv, mk_symbol(sc, sv, syntax));
}

static inline void assign_proc(nanoclj_t * sc, enum nanoclj_opcodes op, const char *name) {
  nanoclj_val_t x = def_symbol(sc, name);
  nanoclj_val_t y = mk_proc(op);
  new_slot_in_env(sc, x, y);
}

static inline void update_window_info(nanoclj_t * sc, nanoclj_val_t out) {
  nanoclj_val_t size = mk_nil();
  
  if (is_writer(out)) {
    nanoclj_port_t * pt = port_unchecked(out);
    if (pt->kind & port_file) {
      FILE * fh = pt->rep.stdio.file;
      
      int cols, rows, width, height;
      if (get_window_size(stdin, fh, &cols, &rows, &width, &height)) {
	sc->window_scale_factor = width / cols > 15 ? 2.0 : 1.0;
	size = mk_vector_2d(sc, width / sc->window_scale_factor, height / sc->window_scale_factor);
      }
    }
  }
  
  intern(sc, sc->global_env, sc->WINDOW_SIZE, size);
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

static const char * checked_strvalue(nanoclj_val_t p) {
  int t = type(p);
  if (t == T_STRING || t == T_CHAR_ARRAY || t == T_FILE) {
    return strvalue(p);
  } else {
    return "";
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

void nanoclj_intern(nanoclj_t * sc, nanoclj_val_t ns, nanoclj_val_t symbol, nanoclj_val_t value) {
  intern(sc, decode_pointer(ns), symbol, value);  
}

static inline nanoclj_val_t mk_counted_string(nanoclj_t * sc, const char *str, size_t len) {
  return mk_pointer(get_string_object(sc, T_STRING, str, len, 0));
}

static struct nanoclj_interface vtbl = {
  nanoclj_intern,
  cons_checked,
  mk_integer,
  mk_real,
  def_symbol,
  mk_string,
  mk_counted_string,
  mk_character,
  mk_vector,
  mk_foreign_func,
  mk_boolean,

  is_string,
  checked_strvalue,
  is_number,
  to_long,
  to_double,
  is_integer,
  is_real,
  to_int,
  is_vector,
  size_checked,
  fill_vector_checked,
  vector_elem_checked,
  set_vector_elem_checked,
  is_reader,
  is_writer,
  is_list,
  checked_car,
  checked_cdr,

  first_checked,
  is_empty_checked,
  rest_checked,
  
  is_symbol,
  is_keyword,

  is_proc,
  is_foreign_function,
  is_closure,
  is_macro,
  is_mapentry,  
  is_environment,
  
  nanoclj_load_file,
  nanoclj_eval_string,
  def_symbol
};
#endif

nanoclj_t *nanoclj_init_new() {
  nanoclj_t *sc = (nanoclj_t *) malloc(sizeof(nanoclj_t));
  if (!nanoclj_init(sc)) {
    free(sc);
    return 0;
  } else {
    return sc;
  }
}

nanoclj_t *nanoclj_init_new_custom_alloc(func_alloc malloc, func_dealloc free, func_realloc realloc) {
  nanoclj_t *sc = (nanoclj_t *) malloc(sizeof(nanoclj_t));
  if (!nanoclj_init_custom_alloc(sc, malloc, free, realloc)) {
    free(sc);
    return 0;
  } else {
    return sc;
  }
}

int nanoclj_init(nanoclj_t * sc) {
  return nanoclj_init_custom_alloc(sc, malloc, free, realloc);
}

static inline nanoclj_cell_t * mk_properties(nanoclj_t * sc) {
#ifdef _WIN32
  const char * term = "Windows";
#else
  const char * term = getenv("TERM_PROGRAM");
  if (!term && getenv("MLTERM")) term = "mlterm";
  else if (!term && getenv("XTERM_VERSION")) term = "XTerm";
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

  int ua_len = sprintf(sc->strbuff, "nanoclj/%s", get_version());
  nanoclj_val_t ua = mk_string_from_sv(sc, (strview_t){sc->strbuff, ua_len});

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
  l = cons(sc, mk_string(sc, get_version()), l);
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

#include "functions.h"
 
int nanoclj_init_custom_alloc(nanoclj_t * sc, func_alloc malloc, func_dealloc free, func_realloc realloc) {  
  int i, n = sizeof(dispatch_table) / sizeof(dispatch_table[0]);

#if USE_INTERFACE
  sc->vptr = &vtbl;
#endif
  sc->gensym_cnt = 0;
  sc->gentypeid_cnt = T_MAX_TYPE;
  sc->malloc = malloc;
  sc->free = free;
  sc->realloc = realloc;
  sc->last_cell_seg = -1;
  sc->sink = mk_pointer(&sc->_sink);
  sc->EMPTY = mk_pointer(&sc->_EMPTY);
  sc->free_cell = &sc->_EMPTY;
  sc->fcells = 0;
  sc->no_memory = 0;
  sc->alloc_seg = sc->malloc(sizeof(*(sc->alloc_seg)) * CELL_NSEGMENT);
  sc->cell_seg = sc->malloc(sizeof(*(sc->cell_seg)) * CELL_NSEGMENT);
  sc->strbuff = sc->malloc(STRBUFF_INITIAL_SIZE);
  sc->strbuff_size = STRBUFF_INITIAL_SIZE;
  sc->errbuff = sc->malloc(STRBUFF_INITIAL_SIZE);
  sc->errbuff_size = STRBUFF_INITIAL_SIZE;  
  sc->save_inport = mk_nil();
  sc->loadport = mk_nil();
  sc->nesting = 0;

  sc->active_element = mk_nil();
  sc->active_element_target = mk_nil();
  sc->active_element_x = sc->active_element_y = 0;
  
  if (alloc_cellseg(sc, FIRST_CELLSEGS) != FIRST_CELLSEGS) {
    sc->no_memory = 1;
    return 0;
  }
  dump_stack_initialize(sc);
  sc->code = sc->EMPTY;
#ifdef USE_RECUR_REGISTER
  sc->recur = sc->EMPTY;
#endif
  sc->tracing = 0;

  /* init sc->EMPTY */
  sc->_EMPTY.type = T_NIL;
  sc->_EMPTY.flags = T_GC_ATOM | MARK;
  _set_car(&(sc->_EMPTY), sc->EMPTY);
  _set_cdr(&(sc->_EMPTY), sc->EMPTY);
  _cons_metadata(&(sc->_EMPTY)) = NULL;
  
  /* init sink */
  sc->_sink.type = T_LIST;
  sc->_sink.flags = MARK;
  _set_car(&(sc->_sink), sc->EMPTY);
  _set_cdr(&(sc->_sink), sc->EMPTY);
  _cons_metadata(&(sc->_sink)) = NULL;

  sc->oblist = oblist_initial_value(sc);
  /* init global_env */
  new_frame_in_env(sc, NULL);
  sc->root_env = sc->global_env = sc->envir;
  
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

  for (i = 0; i < n; i++) {
    if (dispatch_table[i] != 0) {
      assign_proc(sc, (enum nanoclj_opcodes)i, dispatch_table[i]);
    }
  }

  /* initialization of global nanoclj_val_ts to special symbols */
  sc->LAMBDA = def_symbol(sc, "fn");
  sc->DO = def_symbol(sc, "do");
  sc->ARG1 = def_symbol(sc, "%1");
  sc->ARG2 = def_symbol(sc, "%2");
  sc->ARG3 = def_symbol(sc, "%3");
  sc->ARG_REST = def_symbol(sc, "%&");
  sc->QUOTE = def_symbol(sc, "quote");
  sc->DEREF = def_symbol(sc, "deref");
  sc->VAR = def_symbol(sc, "var");
  sc->QQUOTE = def_symbol(sc, "quasiquote");
  sc->UNQUOTE = def_symbol(sc, "unquote");
  sc->UNQUOTESP = def_symbol(sc, "unquote-splicing");

  sc->SLASH_HOOK = def_symbol(sc, "*slash-hook*");
  sc->TAG_HOOK = def_symbol(sc, "*default-data-reader-fn*");
  sc->COMPILE_HOOK = def_symbol(sc, "*compile-hook*");
  sc->IN = def_symbol(sc, "*in*");
  sc->OUT = def_symbol(sc, "*out*");
  sc->ERR = def_symbol(sc, "*err*");
  sc->NS = def_symbol(sc, "*ns*");
  sc->ENV = def_symbol(sc, "*env*");
  sc->WINDOW_SIZE = def_symbol(sc, "*window-size*");
  sc->WINDOW_SCALE_F = def_symbol(sc, "*window-scale-factor*");
  sc->MOUSE_POS = def_symbol(sc, "*mouse-pos*");

  sc->RECUR = def_symbol(sc, "recur");
  sc->AMP = def_symbol(sc, "&");
  sc->UNDERSCORE = def_symbol(sc, "_");
  sc->DOC = def_keyword(sc, "doc");
  sc->WIDTH = def_keyword(sc, "width");
  sc->HEIGHT = def_keyword(sc, "height");
  sc->CHANNELS = def_keyword(sc, "channels");
  sc->WATCHES = def_keyword(sc, "watches");
  sc->NAME = def_keyword(sc, "name");
  sc->TYPE = def_keyword(sc, "type");
    
  sc->SORTED_SET = def_symbol(sc, "sorted-set");
  sc->ARRAY_MAP = def_symbol(sc, "array-map");
  sc->REGEX = def_symbol(sc, "regex");
  sc->EMPTYVEC = get_vector_object(sc, T_VECTOR, 0);

  sc->types = mk_vector_store(sc, 0, 1024);
  			      
  /* Java types */

  nanoclj_cell_t * Object = mk_named_type(sc, "java.lang.Object", gentypeid(sc), sc->global_env);
  nanoclj_cell_t * Number = mk_named_type(sc, "java.lang.Number", gentypeid(sc), Object);
  nanoclj_cell_t * Throwable = mk_named_type(sc, "java.lang.Throwable", gentypeid(sc), Object);
  nanoclj_cell_t * Exception = mk_named_type(sc, "java.lang.Exception", T_EXCEPTION, Throwable);
  nanoclj_cell_t * RuntimeException = mk_named_type(sc, "java.lang.RuntimeException", gentypeid(sc), Exception);
  nanoclj_cell_t * NullPointerException = mk_named_type(sc, "java.lang.NullPointerException", gentypeid(sc), RuntimeException);

  nanoclj_cell_t * AFn = mk_named_type(sc, "clojure.lang.AFn", gentypeid(sc), Object);
    
  mk_named_type(sc, "java.lang.Class", T_CLASS, AFn); /* non-standard parent */
  mk_named_type(sc, "java.lang.String", T_STRING, Object);
  mk_named_type(sc, "java.lang.Boolean", T_BOOLEAN, Object);
  mk_named_type(sc, "java.lang.Character", T_CHARACTER, Object);
  mk_named_type(sc, "java.lang.Integer", T_INTEGER, Number);
  mk_named_type(sc, "java.lang.Double", T_REAL, Number);
  mk_named_type(sc, "java.lang.Long", T_LONG, Number);
  mk_named_type(sc, "java.io.Reader", T_READER, Object);
  mk_named_type(sc, "java.io.Writer", T_WRITER, Object);
  mk_named_type(sc, "java.io.InputStream", T_INPUT_STREAM, Object);
  mk_named_type(sc, "java.io.OutputStream", T_OUTPUT_STREAM, Object);
  mk_named_type(sc, "java.io.File", T_FILE, Object);
    
  /* Clojure types */
  nanoclj_cell_t * AReference = mk_named_type(sc, "clojure.lang.AReference", gentypeid(sc), Object);
  nanoclj_cell_t * Obj = mk_named_type(sc, "clojure.lang.Obj", gentypeid(sc), Object);
  nanoclj_cell_t * ASeq = mk_named_type(sc, "clojure.lang.ASeq", gentypeid(sc), Obj);
  nanoclj_cell_t * PersistentVector = mk_named_type(sc, "clojure.lang.PersistentVector", T_VECTOR, AFn);
    
  mk_named_type(sc, "clojure.lang.PersistentTreeSet", T_SORTED_SET, AFn);
  mk_named_type(sc, "clojure.lang.PersistentArrayMap", T_ARRAYMAP, AFn);
  mk_named_type(sc, "clojure.lang.Symbol", T_SYMBOL, AFn);
  
  mk_named_type(sc, "clojure.lang.Keyword", T_KEYWORD, AFn); /* non-standard parent */
  mk_named_type(sc, "clojure.lang.BigInt", T_BIGINT, Number);
  mk_named_type(sc, "clojure.lang.Ratio", T_RATIO, Number);
  mk_named_type(sc, "clojure.lang.Delay", T_DELAY, Object);
  mk_named_type(sc, "clojure.lang.LazySeq", T_LAZYSEQ, Obj);
  mk_named_type(sc, "clojure.lang.Cons", T_LIST, ASeq);
  mk_named_type(sc, "clojure.lang.Namespace", T_ENVIRONMENT, AReference);
  mk_named_type(sc, "clojure.lang.Var", T_VAR, AReference);
  mk_named_type(sc, "clojure.lang.MapEntry", T_MAPENTRY, PersistentVector);
  
  /* nanoclj types */
  nanoclj_cell_t * Closure = mk_named_type(sc, "nanoclj.core.Closure", T_CLOSURE, AFn);
  mk_named_type(sc, "nanoclj.core.Procedure", T_PROC, AFn);
  mk_named_type(sc, "nanoclj.core.Macro", T_MACRO, Closure);
  mk_named_type(sc, "nanoclj.core.ForeignFunction", T_FOREIGN_FUNCTION, AFn);
  mk_named_type(sc, "nanoclj.core.ForeignObject", T_FOREIGN_OBJECT, AFn);

  mk_named_type(sc, "nanoclj.core.CharArray", T_CHAR_ARRAY, Object);
  mk_named_type(sc, "nanoclj.core.Image", T_IMAGE, Object);
  mk_named_type(sc, "nanoclj.core.Canvas", T_CANVAS, Object);
  mk_named_type(sc, "nanoclj.core.Audio", T_AUDIO, Object);
  mk_named_type(sc, "nanoclj.core.EmptyList", T_NIL, Object);

  intern(sc, sc->global_env, def_symbol(sc, "root"), mk_pointer(sc->root_env));
  intern(sc, sc->global_env, def_symbol(sc, "nil"), mk_nil());
  intern(sc, sc->global_env, sc->MOUSE_POS, mk_nil());

  register_functions(sc);

  sc->sixel_term = has_sixels();
  sc->window_scale_factor = 1.0;

  sc->term_colors = get_term_colortype();

  sc->properties = mk_properties(sc);
    
  return !sc->no_memory;
}

void nanoclj_set_input_port_file(nanoclj_t * sc, FILE * fin) {
  nanoclj_val_t inport = port_from_file(sc, fin, port_input);
  intern(sc, sc->global_env, sc->IN, inport);
}

void nanoclj_set_input_port_string(nanoclj_t * sc, const char *str, size_t length) {
  nanoclj_val_t inport = port_from_string(sc, (strview_t){ str, length }, port_input);
  intern(sc, sc->global_env, sc->IN, inport);
}

void nanoclj_set_output_port_file(nanoclj_t * sc, FILE * fout) {
  nanoclj_val_t p = port_from_file(sc, fout, port_output);
  intern(sc, sc->root_env, sc->OUT, p);

  update_window_info(sc, p);
}

void nanoclj_set_output_port_callback(nanoclj_t * sc,
				      void (*text) (const char *, size_t, void *),
				      void (*color) (double, double, double, void *),
				      void (*restore) (void *),
				      void (*image) (nanoclj_image_t *, void *)
				      ) {
  nanoclj_val_t p = port_from_callback(sc, text, color, restore, image, port_output);
  intern(sc, sc->root_env, sc->OUT, p);
  intern(sc, sc->global_env, sc->WINDOW_SIZE, mk_nil());
}

void nanoclj_set_output_port_string(nanoclj_t * sc, const char * str, size_t length) {
  nanoclj_val_t p = port_from_string(sc, (strview_t){ str, length }, port_output);
  intern(sc, sc->root_env, sc->OUT, p);
  intern(sc, sc->global_env, sc->WINDOW_SIZE, mk_nil());
}

void nanoclj_set_error_port_callback(nanoclj_t * sc, void (*text) (const char *, size_t, void *)) {
  nanoclj_val_t p = port_from_callback(sc, text, NULL, NULL, NULL, port_output);
  intern(sc, sc->global_env, sc->ERR, p);
}

void nanoclj_set_error_port_file(nanoclj_t * sc, FILE * fout) {
  nanoclj_val_t p = port_from_file(sc, fout, port_output);
  intern(sc, sc->global_env, sc->ERR, p);
}

void nanoclj_set_external_data(nanoclj_t * sc, void *p) {
  sc->ext_data = p;
}

void nanoclj_set_object_invoke_callback(nanoclj_t * sc, nanoclj_val_t (*func) (nanoclj_t *, void *, nanoclj_val_t)) {
  sc->object_invoke_callback = func;
}

void nanoclj_deinit(nanoclj_t * sc) {
  int i;

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
  sc->loadport = mk_nil();
  sc->free(sc->strbuff);
  sc->free(sc->errbuff);

  sc->active_element = mk_nil();
  sc->active_element_target = mk_nil();

  gc(sc, NULL, NULL, NULL);
  
  for (i = 0; i <= sc->last_cell_seg; i++) {
    sc->free(sc->alloc_seg[i]);
  }
  sc->free(sc->cell_seg);
  sc->free(sc->alloc_seg);

  for (i = 0; i <= sc->file_i; i++) {
    if (sc->load_stack[i].kind & port_file) {
      char *fname = sc->load_stack[i].rep.stdio.filename;
      if (fname) sc->free(fname);
    }
  }
}

void nanoclj_load_file(nanoclj_t * sc, FILE * fin) {
  nanoclj_load_named_file(sc, fin, 0);
}

void nanoclj_load_named_file(nanoclj_t * sc, FILE * fin, const char *filename) {
  if (fin == NULL) {
    fprintf(stderr, "File can not be NULL when loading a file\n");
    return;
  }
  dump_stack_reset(sc);
  sc->envir = sc->global_env;
  sc->file_i = 0;
  sc->load_stack[0].kind = port_input | port_file;
  sc->load_stack[0].backchar[0] = sc->load_stack[0].backchar[1] = -1;
  sc->load_stack[0].rep.stdio.file = fin;
  sc->load_stack[0].rep.stdio.filename = NULL;    
  sc->loadport = mk_reader(sc, sc->load_stack);
  sc->retcode = 0;

  sc->load_stack[0].rep.stdio.curr_line = 0;
  if (fin != stdin && filename) {
    sc->load_stack[0].rep.stdio.filename = store_string(sc, strlen(filename), filename);
  } else {
    sc->load_stack[0].rep.stdio.filename = NULL;
  }

  sc->args = mk_long(sc, sc->file_i);
  Eval_Cycle(sc, OP_T0LVL);

  /* Clear the loadport manually so it wont get collected */
  cell_type(sc->loadport) = 0;
  cell_flags(sc->loadport) = T_GC_ATOM;

  if (sc->retcode == 0) {
    sc->retcode = sc->nesting != 0;
  }
}

nanoclj_val_t nanoclj_eval_string(nanoclj_t * sc, const char *cmd, size_t len) {
  dump_stack_reset(sc);

  char * buffer = (char *)sc->malloc(len);
  memcpy(buffer, cmd, len);

  sc->envir = sc->global_env;
  sc->file_i = 0;
  sc->load_stack[0].kind = port_input | port_string;
  sc->load_stack[0].backchar[0] = sc->load_stack[0].backchar[1] = -1;
  sc->load_stack[0].rep.string.curr = buffer;
  sc->load_stack[0].rep.string.data.data = buffer;
  sc->load_stack[0].rep.string.data.size = len;
  sc->loadport = mk_reader(sc, sc->load_stack);
  sc->retcode = 0;
  sc->args = mk_long(sc, sc->file_i);
  
  Eval_Cycle(sc, OP_T0LVL);

  /* Clear the loadport manually so it wont get collected */
  cell_type(sc->loadport) = 0;
  cell_flags(sc->loadport) = T_GC_ATOM;
  if (sc->retcode == 0) {
    sc->retcode = sc->nesting != 0;
  }
  return sc->value;
}

/* "func" and "args" are assumed to be already eval'ed. */
nanoclj_val_t nanoclj_call(nanoclj_t * sc, nanoclj_val_t func, nanoclj_val_t args) {
  save_from_C_call(sc);
  sc->envir = sc->global_env;
  sc->args = decode_pointer(args);
  sc->code = func;
  sc->retcode = 0;
  Eval_Cycle(sc, OP_APPLY);  
  return sc->value;
}

#if !NANOCLJ_STANDALONE
void nanoclj_register_foreign_func(nanoclj_t * sc, nanoclj_registerable * sr) {
  intern(sc, sc->global_env, def_symbol(sc, sr->name), mk_foreign_func(sc, sr->f));
}

void nanoclj_register_foreign_func_list(nanoclj_t * sc,
    nanoclj_registerable * list, int count) {
  int i;
  for (i = 0; i < count; i++) {
    nanoclj_register_foreign_func(sc, list + i);
  }
}

nanoclj_val_t nanoclj_apply0(nanoclj_t * sc, const char *procname) {
  return nanoclj_eval(sc, mk_pointer(cons(sc, def_symbol(sc, procname), NULL)));
}

#endif

nanoclj_val_t nanoclj_eval(nanoclj_t * sc, nanoclj_val_t obj) {
  return eval(sc, decode_pointer(obj));
}

/* ========== Main ========== */

#if NANOCLJ_STANDALONE

static inline FILE *open_file(const char *fname) {
  if (str_eq(fname, "-") || str_eq(fname, "--")) {
    return stdin;
  }
  return fopen(fname, "r");
}

int main(int argc, const char **argv) {
  if (argc == 1) {
    printf("nanoclj %s\n", get_version());
  }
  if (argc == 2 && str_eq(argv[1], "--help")) {
    printf("Usage: %s [switches] [--] [programfile] [arguments]\n", argv[0]);
    return 1;
  }
  if (argc == 2 && str_eq(argv[1], "--legal")) {
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
		mk_foreign_func(&sc, scm_load_ext));
#endif

  nanoclj_cell_t * args = NULL;
  for (int i = argc - 1; i >= 2; i--) {
    nanoclj_val_t value = mk_string(&sc, argv[i]);
    args = cons(&sc, value, args);
  }
  intern(&sc, sc.global_env, def_symbol(&sc, "*command-line-args*"), mk_pointer(args));
  
  FILE * fh = open_file(InitFile);
  nanoclj_load_named_file(&sc, fh, InitFile);
  if (sc.retcode != 0) {
    fprintf(stderr, "Errors encountered reading %s\n", InitFile);
    exit(1);
  }
  fclose(fh);
  
  if (argc >= 2) {
    fh = open_file(argv[1]);
    nanoclj_load_named_file(&sc, fh, argv[1]);
    if (sc.retcode != 0) {
      fprintf(stderr, "Errors encountered reading %s\n", argv[1]);
      exit(1);
    }
    fclose(fh);

    /* Call -main if it exists */
    nanoclj_val_t main = def_symbol(&sc, "-main");
    nanoclj_cell_t * x = find_slot_in_env(&sc, sc.envir, main, true);
    if (x) {
      nanoclj_val_t v = slot_value_in_env(x);
      v = nanoclj_call(&sc, v, sc.EMPTY);
      return to_int(v);
    }
  } else {
    const char * expr = "(clojure.repl/repl)";
    nanoclj_eval_string(&sc, expr, strlen(expr));
  }
  int retcode = sc.retcode;
  nanoclj_deinit(&sc);

  return retcode;
}

#endif
