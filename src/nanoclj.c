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
#define SIGNATURE_TYPE    (kNaN | MASK_TYPE_TYPE)
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

#define VERSION "nanoclj (v0.2)"

#include <string.h>
#include <stdlib.h>

#define str_eq(X,Y) (!strcmp((X),(Y)))

#ifndef InitFile
#define InitFile "init.clj"
#endif

#ifndef FIRST_CELLSEGS
#define FIRST_CELLSEGS 3
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
  T_PAIR = 8,
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
  T_TYPE = 19,
  T_KEYWORD = 20,
  T_MAPENTRY = 21,
  T_ARRAYMAP = 22,  
  T_SEQ = 23,
  T_SORTED_SET = 24,
  T_FOREIGN_OBJECT = 25,
  T_BIGINT = 26,
  T_CHAR_ARRAY = 27,
  T_INPUT_STREAM = 28,
  T_OUTPUT_STREAM = 29,
  T_RATIO = 30,
  T_LIST = 31,
  T_IMAGE = 32,
  T_CANVAS = 33,
  T_VAR = 34,
  T_DELAY = 35
};

typedef struct strview_s {
  const char * ptr;
  size_t size;
} strview_t;

struct symbol {
  int_fast16_t syntax;
  const char * name;
};

/* ADJ is enough slack to align cells in a TYPE_BITS-bit boundary */
#define ADJ		64
#define TYPE_BITS	 6
#define T_MASKTYPE      63      /* 0000000000111111 */
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
  case SIGNATURE_TYPE:    return T_TYPE;
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

static inline bool is_cell(nanoclj_val_t value) {
  return prim_type(value) == T_CELL;
}

static inline bool is_primitive(nanoclj_val_t value) {
  return !is_cell(value);
}

static inline int_fast16_t _type(nanoclj_cell_t * a) {
  if (!a) return T_NIL;
  return a->_flag & T_MASKTYPE;
}

static inline bool is_symbol(nanoclj_val_t p) {
  return prim_type(p) == T_SYMBOL;
}

static inline bool is_keyword(nanoclj_val_t p) {
  return prim_type(p) == T_KEYWORD;
}

static inline nanoclj_cell_t * decode_pointer(nanoclj_val_t value) {
  return (nanoclj_cell_t *)(value.as_long & MASK_PAYLOAD_PTR);
}

static inline struct symbol * decode_symbol(nanoclj_val_t value) {
  return (struct symbol *)(value.as_long & MASK_PAYLOAD_PTR);
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

static inline bool is_pair(nanoclj_val_t value) {
  if (!is_cell(value)) return false;
  nanoclj_cell_t * c = decode_pointer(value);
  uint64_t t = _type(c);
  return t == T_PAIR || t == T_LIST;
}

static inline nanoclj_val_t mk_pointer(nanoclj_cell_t * ptr) {
  nanoclj_val_t v;
  v.as_long = SIGNATURE_POINTER | (uint64_t)ptr;
  return v;
}

static inline nanoclj_val_t mk_keyword(const char * ptr) {
  nanoclj_val_t v;
  v.as_long = SIGNATURE_KEYWORD | (uint64_t)ptr;
  return v;
}

static inline nanoclj_val_t mk_nil() {
  nanoclj_val_t v;
  v.as_long = kNIL;
  return v;
}

static inline nanoclj_val_t mk_symbol(nanoclj_t * sc, const char * name, int_fast16_t syntax) {
  struct symbol * s = sc->malloc(sizeof(struct symbol));
  s->name = name;
  s->syntax = syntax;
  
  nanoclj_val_t v;
  v.as_long = SIGNATURE_SYMBOL | (uint64_t)s;
  return v;
}

static inline bool is_nil(nanoclj_val_t v) {
  return v.as_long == kNIL;
}

#define _typeflag(p)	 ((p)->_flag)

#define _car(p)		 ((p)->_object._cons.car)
#define _cdr(p)		 ((p)->_object._cons.cdr)
#define _watches_unchecked(p)	((p)->_object._cons.watches)

#define _is_realized(p)           (_typeflag(p) & T_REALIZED)
#define _set_realized(p)          (_typeflag(p) |= T_REALIZED)
#define _is_mark(p)               (_typeflag(p) & MARK)
#define _setmark(p)               (_typeflag(p) |= MARK)
#define _clrmark(p)               (_typeflag(p) &= UNMARK)
#define _is_small(p)	          (_typeflag(p) & T_SMALL)
#define _is_gc_atom(p)            (_typeflag(p) & T_GC_ATOM)
#define _set_gc_atom(p)           (_typeflag(p) |= T_GC_ATOM)
#define _clr_gc_atom(p)           (_typeflag(p) &= CLR_GC_ATOM)

#define _offset_unchecked(p)	  ((p)->_object._collection.offset)
#define _size_unchecked(p)	  ((p)->_object._collection.size)
#define _store_unchecked(p)	  ((p)->_object._collection.store)

#define _port_unchecked(p)	  ((p)->_object._port)
#define _image_unchecked(p)	  ((p)->_object._image)
#define _canvas_unchecked(p)	  ((p)->_object._canvas)
#define _metadata_unchecked(p)     ((p)->_metadata)
#define _origin_unchecked(p)	  ((p)->_object._seq.origin)
#define _lvalue_unchecked(p)       ((p)->_object._lvalue)
#define _position_unchecked(p)	  ((p)->_object._seq.pos)
#define _ff_unchecked(p)	  ((p)->_object._ff.ptr)
#define _fo_unchecked(p)	  ((p)->_object._fo.ptr)
#define _min_arity_unchecked(p)	  ((p)->_object._ff.min_arity)
#define _max_arity_unchecked(p)	  ((p)->_object._ff.max_arity)

#define _strvalue_unchecked(p)      ((p)->_object._string.data)
#define _strlength_unchecked(p)        ((p)->_object._string.size)

#define _smallstrvalue_unchecked(p)      (&((p)->_object._small_string.data[0]))
#define _smallstrlength_unchecked(p)      ((p)->_object._small_string.size)

/* macros for cell operations */
#define typeflag(p)      (_typeflag(decode_pointer(p)))
#define is_small(p)	 (typeflag(p)&T_SMALL)

#define car(p)           _car(decode_pointer(p))
#define cdr(p)           _cdr(decode_pointer(p))

#define strvalue_unchecked(p)      (decode_pointer(p)->_object._string.data)
#define strlength_unchecked(p)        (decode_pointer(p)->_object._string.size)

#define smallstrvalue_unchecked(p)      (&(decode_pointer(p)->_object._small_string.data[0]))
#define smallstrlength_unchecked(p)      (decode_pointer(p)->_object._small_string.size)

#define offset_unchecked(p)	  (decode_pointer(p)->_object._collection.offset)
#define size_unchecked(p)	  (decode_pointer(p)->_object._collection.size)
#define store_unchecked(p)	  (decode_pointer(p)->_object._collection.store)

#define lvalue_unchecked(p)       (decode_pointer(p)->_object._lvalue)
#define rvalue_unchecked(p)       ((p).as_double)
#define metadata_unchecked(p)     (decode_pointer(p)->_metadata)
#define origin_unchecked(p)	  (decode_pointer(p)->_object._seq.origin)
#define position_unchecked(p)	  (decode_pointer(p)->_object._seq.pos)
#define port_unchecked(p)	  (decode_pointer(p)->_object._port)
#define image_unchecked(p)	  (decode_pointer(p)->_object._image)
#define canvas_unchecked(p)	  (decode_pointer(p)->_object._canvas)

static inline int32_t decode_integer(nanoclj_val_t value) {
  return (int32_t)value.as_long & MASK_PAYLOAD_INT;
}

static inline bool is_boolean(nanoclj_val_t p) {
  return (prim_type(p) == T_BOOLEAN);
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
static inline bool is_arraymap(nanoclj_val_t p) {
  if (!is_cell(p)) return false;
  nanoclj_cell_t * c = decode_pointer(p);
  return _type(c) == T_ARRAYMAP;
}
static inline bool is_set(nanoclj_val_t p) {
  if (!is_cell(p)) return false;
  nanoclj_cell_t * c = decode_pointer(p);
  return _type(c) == T_SORTED_SET;
}
static inline bool is_seq(nanoclj_val_t p) {
  if (!is_cell(p)) return false;
  nanoclj_cell_t * c = decode_pointer(p);
  return _type(c) == T_SEQ;
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

static inline bool is_coll_type(int_fast16_t type_id) {
  switch (type_id) {
  case T_PAIR:
  case T_LIST:
  case T_VECTOR:
  case T_MAPENTRY:
  case T_ARRAYMAP:
  case T_SEQ:
  case T_SORTED_SET:
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
  return _store_unchecked(vec)->data[_offset_unchecked(vec) + ielem];
}

static inline void set_vector_elem(nanoclj_cell_t * vec, size_t ielem, nanoclj_val_t a) {
  _store_unchecked(vec)->data[_offset_unchecked(vec) + ielem] = a;
}

static void fill_vector(nanoclj_cell_t * vec, nanoclj_val_t elem) {
  size_t num = _size_unchecked(vec);
  for (size_t i = 0; i < num; i++) {
    set_vector_elem(vec, i, elem);
  }
}

static inline bool is_integral_type(int_fast16_t type) {
  switch (type) {
  case T_BOOLEAN:
  case T_INTEGER:
  case T_CHARACTER:
  case T_PROC:
  case T_TYPE:
    return true;
  }
  return false;
}

static inline bool is_type(nanoclj_val_t p) {
  return prim_type(p) == T_TYPE;
}

static inline bool is_number(nanoclj_val_t p) {
  switch (type(p)) {
  case T_INTEGER:
  case T_LONG:
  case T_REAL:
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

static inline bool is_character(nanoclj_val_t p) {
  return prim_type(p) == T_CHARACTER;
}

static inline char * _strvalue(nanoclj_cell_t * s) {
  if (_is_small(s)) {
    return _smallstrvalue_unchecked(s);
  } else {
    return _strvalue_unchecked(s);
  }
}
static inline const char * strvalue(nanoclj_val_t p) {
  if (is_cell(p)) {
    nanoclj_cell_t * c = decode_pointer(p);
    if (c && (_type(c) == T_STRING || _type(c) == T_CHAR_ARRAY)) {
      return _strvalue(c);
    }
  }
  return "";
}

static inline size_t strlength(nanoclj_val_t p) {
  if (is_small(p)) {
    return smallstrlength_unchecked(p);
  } else {
    return strlength_unchecked(p);
  }
}

static inline size_t _strlength(nanoclj_cell_t * s) {
  if (_is_small(s)) {
    return _smallstrlength_unchecked(s);
  } else {
    return _strlength_unchecked(s);
  }
}

static inline long long to_long(nanoclj_val_t p) {
  switch (prim_type(p)) {
  case T_INTEGER:
  case T_PROC:
  case T_TYPE:
  case T_CHARACTER:
    return decode_integer(p);
  case T_REAL:
    return (long long)rvalue_unchecked(p);
  case T_CELL:
    {
      nanoclj_cell_t * c = decode_pointer(p);
      switch (_type(c)) {
      case T_LONG:
	return _lvalue_unchecked(c);
      case T_RATIO:
	return to_long(_car(c)) / to_long(_cdr(c));
      case T_VECTOR:
      case T_SORTED_SET:
      case T_ARRAYMAP:
	return _size_unchecked(c);
      }
    }
  }
  
  return 0;
}

static inline int32_t to_int(nanoclj_val_t p) {
  switch (prim_type(p)) {
  case T_CHARACTER:
  case T_PROC:
  case T_INTEGER:
  case T_TYPE:
    return decode_integer(p);
  case T_REAL:
    return (int_fast32_t)rvalue_unchecked(p);
  case T_CELL:
    {
      nanoclj_cell_t * c = decode_pointer(p);
      switch (_type(c)) {
      case T_LONG:
	return _lvalue_unchecked(c);
      case T_RATIO:
	return to_long(_car(c)) / to_long(_cdr(c));
      case T_VECTOR:
      case T_SORTED_SET:
      case T_ARRAYMAP:
	return _size_unchecked(c);
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
    return rvalue_unchecked(p);
  case T_CELL: {
    nanoclj_cell_t * c = decode_pointer(p);
    switch (_type(c)) {
    case T_LONG: return (double)lvalue_unchecked(p);
    case T_RATIO: return to_double(_car(c)) / to_double(_cdr(c));
    }
  }
  }
  return NAN;
}

static inline bool is_eof(nanoclj_val_t p) {
  return is_character(p) && decode_integer(p) == EOF;
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

static inline const char *symname(nanoclj_val_t p) {
  return decode_symbol(p)->name;
}

static inline const char *keywordname(nanoclj_val_t p) {
  return (const char *)decode_pointer(p);
}

static inline bool is_proc(nanoclj_val_t p) {
  return prim_type(p) == T_PROC;
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
static inline nanoclj_val_t closure_env(nanoclj_cell_t * p) {
  return _cdr(p);
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
  return _type(c) == T_ENVIRONMENT;
}

/* true or false value functions */
/* () is #t in R5RS, but nil is false in clojure */

static inline bool is_false(nanoclj_val_t p) {
  return p.as_long == kFALSE || p.as_long == kNIL;
}

static inline bool is_true(nanoclj_val_t p) {
  return !is_false(p);
}

#define is_mark(p)       (typeflag(p)&MARK)
#define clrmark(p)       typeflag(p) &= UNMARK

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

static void gc(nanoclj_t * sc, nanoclj_val_t a, nanoclj_val_t b);
static void putstr(nanoclj_t * sc, const char *s, nanoclj_val_t port);
static void dump_stack_mark(nanoclj_t *);
static nanoclj_val_t get_err_port(nanoclj_t * sc);
static nanoclj_val_t get_in_port(nanoclj_t * sc);

static inline const char* get_version() {
  return VERSION;
}

static inline const char * typename_from_id(int_fast16_t id) {
  switch (id) {
  case 0: return "nanoclj.core.EmptyList";
  case 1: return "java.lang.Boolean";
  case 2: return "java.lang.String";
  case 3: return "java.lang.Integer";
  case 4: return "java.lang.Long";
  case 5: return "java.lang.Double";
  case 6: return "clojure.lang.Symbol";
  case 7: return "nanoclj.core.Procedure";
  case 8: return "clojure.lang.Cons";
  case 9: return "nanoclj.core.Closure";
  case 10: return "java.lang.Exception";	
  case 11: return "nanoclj.core.ForeignFunction";
  case 12: return "java.lang.Character";
  case 13: return "java.io.Reader";
  case 14: return "java.io.Writer";
  case 15: return "clojure.lang.PersistentVector";
  case 16: return "nanoclj.core.Macro";
  case 17: return "clojure.lang.LazySeq";
  case 18: return "clojure.lang.Namespace"; /* Environment */
  case 19: return "nanoclj.core.Type";
  case 20: return "clojure.lang.Keyword";
  case 21: return "clojure.lang.MapEntry";
  case 22: return "clojure.lang.PersistentArrayMap";
  case 23: return "nanoclj.core.Seq";
  case 24: return "clojure.lang.PersistentTreeSet";
  case 25: return "nanoclj.core.ForeignObject";
  case 26: return "clojure.lang.BigInt";
  case 27: return "nanoclj.core.CharArray";
  case 28: return "java.io.InputStream";
  case 29: return "java.io.OutputStream";
  case 30: return "clojure.lang.Ratio";
  case 31: return "clojure.lang.PersistentList";
  case 32: return "nanoclj.core.Image";
  case 33: return "nanoclj.core.Canvas";
  case 34: return "clojure.lang.Var";
  case 35: return "clojure.lang.Delay";
  }
  return "";
}

static inline strview_t _to_strview(nanoclj_cell_t * c) {
  switch (_type(c)) {
  case T_STRING:
  case T_CHAR_ARRAY:
    if (_is_small(c)) {
      return (strview_t){ _smallstrvalue_unchecked(c), _smallstrlength_unchecked(c) };
    } else {
      return (strview_t){ _strvalue_unchecked(c), _strlength_unchecked(c) };
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
  case T_SYMBOL: {
    const char * s = symname(x);
    return (strview_t){ s, strlen(s) };
  }
  case T_KEYWORD: {
    const char * s = keywordname(x);
    return (strview_t){ s, strlen(s) };
  }
  case T_TYPE: {
    const char * s = typename_from_id(decode_integer(x));
    return (strview_t){ s, strlen(s) };
  }
  case T_CELL:
    return _to_strview(decode_pointer(x));
  }  
  return (strview_t){ "", 0 };
}

static inline int_fast16_t syntaxnum(nanoclj_val_t p) {
  if (is_symbol(p)) {
    struct symbol * s = decode_symbol(p);
    return s->syntax;
  }
  return 0;
}

/* allocate new cell segment */
static inline int alloc_cellseg(nanoclj_t * sc, int n) {
  nanoclj_val_t newp;
  nanoclj_val_t last;
  nanoclj_val_t p;
  char *cp;
  long i;
  int k;
  int adj = ADJ;

  if (adj < sizeof(nanoclj_cell_t)) {
    adj = sizeof(nanoclj_cell_t);
  }
  for (k = 0; k < n; k++) {
    if (sc->last_cell_seg >= CELL_NSEGMENT - 1) {
      return k;
    }
    cp = (char *) sc->malloc(CELL_SEGSIZE * sizeof(nanoclj_cell_t) + adj);
    if (cp == 0) {
      return k;
    }
    i = ++sc->last_cell_seg;
    sc->alloc_seg[i] = cp;
    /* adjust in TYPE_BITS-bit boundary */
    if (((unsigned long) cp) % adj != 0) {
      cp = (char *) (adj * ((unsigned long) cp / adj + 1));
    }
    /* insert new segment in address order */
    newp = mk_pointer((nanoclj_cell_t *)cp);
    sc->cell_seg[i] = newp;
    while (i > 0 && sc->cell_seg[i - 1].as_long > sc->cell_seg[i].as_long) {
      p = sc->cell_seg[i];
      sc->cell_seg[i] = sc->cell_seg[i - 1];
      sc->cell_seg[--i] = p;
    }
    sc->fcells += CELL_SEGSIZE;
    last = mk_pointer(decode_pointer(newp) + CELL_SEGSIZE - 1);
    for (p = newp; p.as_long <= last.as_long;
	 p = mk_pointer(decode_pointer(p) + 1)
	 ) {
      typeflag(p) = 0;
      cdr(p) = mk_pointer(decode_pointer(p) + 1);
      car(p) = sc->EMPTY;
    }
    /* insert new cells in address order on free list */
    if (sc->free_cell == &(sc->_EMPTY) || decode_pointer(p) < sc->free_cell) {
      cdr(last) = mk_pointer(sc->free_cell);
      sc->free_cell = decode_pointer(newp);
    } else {
      nanoclj_cell_t * p = sc->free_cell;
      while (_cdr(p).as_long != sc->EMPTY.as_long && newp.as_long > _cdr(p).as_long)
        p = decode_pointer(_cdr(p));

      cdr(last) = _cdr(p);
      _cdr(p) = newp;
    }
  }

  return n;
}

/* get new cell.  parameter a, b is marked by gc. */

static inline nanoclj_cell_t * get_cell_x(nanoclj_t * sc, nanoclj_val_t a, nanoclj_val_t b) {
  if (sc->no_memory) {
    return &(sc->_sink);
  }

  if (sc->free_cell == &(sc->_EMPTY)) {
    const int min_to_be_recovered = sc->last_cell_seg * 8;
    gc(sc, a, b);
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
  return x;
}

/* To retain recent allocs before interpreter knows about them -
   Tehom */

static inline void retain(nanoclj_t * sc, nanoclj_val_t recent, nanoclj_val_t extra) {
  nanoclj_cell_t * holder = get_cell_x(sc, recent, extra);
  _typeflag(holder) = T_PAIR;
  _car(holder) = recent;
  _cdr(holder) = car(sc->sink);
  _metadata_unchecked(holder) = mk_nil();
  car(sc->sink) = mk_pointer(holder);
}

static inline nanoclj_cell_t * get_cell(nanoclj_t * sc, int flags, nanoclj_val_t a, nanoclj_val_t b, nanoclj_val_t meta) {
  nanoclj_cell_t * cell = get_cell_x(sc, a, b);

  /* For right now, include "a" and "b" in "cell" so that gc doesn't
     think they are garbage. */
  /* Tentatively record it as a pair so gc understands it. */
  _typeflag(cell) = flags;
  _car(cell) = a;
  _cdr(cell) = b;
  _metadata_unchecked(cell) = meta;

#if 0
  retain(sc, mk_pointer(cell), sc->EMPTY);
#endif
  
  return cell;
}

static inline nanoclj_val_t mk_reader(nanoclj_t * sc, nanoclj_port_t * p) {
  nanoclj_cell_t * x = get_cell_x(sc, sc->EMPTY, sc->EMPTY);

  _typeflag(x) = T_READER | T_GC_ATOM;
  _port_unchecked(x) = p;
  _metadata_unchecked(x) = mk_nil();

  return mk_pointer(x);
}

static inline nanoclj_val_t mk_writer(nanoclj_t * sc, nanoclj_port_t * p) {
  nanoclj_cell_t * x = get_cell_x(sc, sc->EMPTY, sc->EMPTY);

  _typeflag(x) = T_WRITER | T_GC_ATOM;
  _port_unchecked(x) = p;
  _metadata_unchecked(x) = mk_nil();

  return mk_pointer(x);
}

static inline nanoclj_val_t mk_foreign_func(nanoclj_t * sc, foreign_func f) {
  nanoclj_cell_t * x = get_cell_x(sc, sc->EMPTY, sc->EMPTY);

  _typeflag(x) = T_FOREIGN_FUNCTION | T_GC_ATOM;
  _ff_unchecked(x) = f;
  _min_arity_unchecked(x) = 0;
  _max_arity_unchecked(x) = 0x7fffffff;
  _metadata_unchecked(x) = mk_nil();
  
  return mk_pointer(x);
}

static inline nanoclj_val_t mk_foreign_object(nanoclj_t * sc, void * o) {
  nanoclj_cell_t * x = get_cell_x(sc, sc->EMPTY, sc->EMPTY);

  _typeflag(x) = T_FOREIGN_OBJECT | T_GC_ATOM;
  _fo_unchecked(x) = o;
  _min_arity_unchecked(x) = 0;
  _max_arity_unchecked(x) = 0x7fffffff;
  _metadata_unchecked(x) = mk_nil();
  
  return mk_pointer(x);
}

static inline nanoclj_val_t mk_foreign_func_with_arity(nanoclj_t * sc, foreign_func f, int min_arity, int max_arity) {
  nanoclj_cell_t * x = get_cell_x(sc, sc->EMPTY, sc->EMPTY);

  _typeflag(x) = T_FOREIGN_FUNCTION | T_GC_ATOM;
  _ff_unchecked(x) = f;
  _min_arity_unchecked(x) = min_arity;
  _max_arity_unchecked(x) = max_arity;
  _metadata_unchecked(x) = mk_nil();

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

static inline nanoclj_val_t mk_type(int t) {
  nanoclj_val_t v;
  v.as_long = SIGNATURE_TYPE | (uint32_t)t;
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

static inline nanoclj_val_t mk_long(nanoclj_t * sc, long long num) {
  nanoclj_cell_t * x = get_cell_x(sc, sc->EMPTY, sc->EMPTY);
  _typeflag(x) = T_LONG | T_GC_ATOM;
  _lvalue_unchecked(x) = num;
  _metadata_unchecked(x) = mk_nil();
  return mk_pointer(x);
}

/* get number atom (integer) */
static inline nanoclj_val_t mk_integer(nanoclj_t * sc, long long num) {
  if (num >= INT_MIN && num <= INT_MAX) {
    return mk_int(num);
  } else {
    return mk_long(sc, num);
  }
}

static inline nanoclj_val_t mk_image(nanoclj_t * sc, int32_t width, int32_t height,
				     int32_t channels, unsigned char * data) {
  nanoclj_cell_t * x = get_cell_x(sc, sc->EMPTY, sc->EMPTY);
  nanoclj_image_t * image = (nanoclj_image_t*)malloc(sizeof(nanoclj_image_t));
  _typeflag(x) = T_IMAGE | T_GC_ATOM;
  _image_unchecked(x) = image;
  _metadata_unchecked(x) = mk_nil();

  size_t size = width * height * channels;
  image->data = (unsigned char *)malloc(size);
  image->width = width;
  image->height = height;
  image->channels = channels;
  
  if (data) memcpy(image->data, data, size);
  
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
      *den = to_long(_cdr(c));
      return to_long(_car(c));
    }
  }
  }
  *den = 1;
  return 0;
}

static inline nanoclj_val_t mk_ratio(nanoclj_t * sc, nanoclj_val_t num, nanoclj_val_t den) {  
  nanoclj_cell_t * x = get_cell_x(sc, sc->EMPTY, sc->EMPTY);
  _typeflag(x) = T_RATIO | T_GC_ATOM;
  _car(x) = num;
  _cdr(x) = den;
  _metadata_unchecked(x) = mk_nil();
    
  return mk_pointer(x);
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

static inline nanoclj_val_t mk_counted_string(nanoclj_t * sc, const char *str, size_t len) {
  nanoclj_cell_t * x = get_cell_x(sc, sc->EMPTY, sc->EMPTY);
  _metadata_unchecked(x) = mk_nil();

  if (len <= 23) {
    _typeflag(x) = T_STRING | T_GC_ATOM | T_SMALL;
    if (str) {
      memcpy(_smallstrvalue_unchecked(x), str, len);
      _smallstrvalue_unchecked(x)[len] = 0;
    }
    _smallstrlength_unchecked(x) = len;
  } else {
    char * s = (char *)malloc(len);
    memcpy(s, str, len);
    
    _typeflag(x) = T_STRING | T_GC_ATOM;
    _strvalue_unchecked(x) = s;
    _strlength_unchecked(x) = len;
  } 

  return mk_pointer(x);
}

/* get new string */
static inline nanoclj_val_t mk_string(nanoclj_t * sc, const char *str) {
  if (str) {
    return mk_counted_string(sc, str, strlen(str));
  } else {
    return mk_nil();
  }
}

static inline nanoclj_val_t mk_exception(nanoclj_t * sc, const char * text) {
  nanoclj_cell_t * x = get_cell(sc, T_EXCEPTION, mk_string(sc, text), sc->EMPTY, mk_nil());
  return mk_pointer(x);
}

static inline nanoclj_cell_t * mk_seq(nanoclj_t * sc, nanoclj_cell_t * origin, size_t position) {
  nanoclj_cell_t * x = get_cell_x(sc, sc->EMPTY, sc->EMPTY);
  _typeflag(x) = T_SEQ | T_GC_ATOM;
  _origin_unchecked(x) = origin;
  _position_unchecked(x) = position;
  _metadata_unchecked(x) = mk_nil();
  return x;
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
static inline int inchar(nanoclj_t * sc, nanoclj_port_t * pt) {
  int c;
  if (pt->backchar >= 0) {
    c = pt->backchar;
    pt->backchar = -1;
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
  }
  return c;
}

/* back character to input buffer */
static inline void backchar(int c, nanoclj_port_t * pt) {
  pt->backchar = c;
}

/* Medium level cell allocation */

/* get new cons cell */
static inline nanoclj_val_t cons(nanoclj_t * sc, nanoclj_val_t a, nanoclj_val_t b) {
  int t;
  if (b.as_long == sc->EMPTY.as_long) {
    t = T_LIST;
  } else {
    t = type(b);
    if (t == T_LIST || t == T_LAZYSEQ || t == T_SEQ) {
      t = T_LIST;
    } else {
      t = T_PAIR;
    }
  }
  return mk_pointer(get_cell(sc, t, a, b, mk_nil()));
}

/* Creates a vector store with double the size requested */
static inline nanoclj_vector_store_t * mk_vector_store(nanoclj_t * sc, size_t len) {
  nanoclj_vector_store_t * s = (nanoclj_vector_store_t *)malloc(sizeof(nanoclj_vector_store_t));
  s->data = (nanoclj_val_t*)sc->malloc(2 * len * sizeof(nanoclj_val_t));
  s->size = len;
  s->reserved = (len > 1 ? len : 1) * 2;
  s->refcnt = 0;
  return s;
}

static inline void finalize_vector_store(nanoclj_t * sc, nanoclj_vector_store_t * s) {
  sc->free(s->data);
  sc->free(s);
}

static inline nanoclj_cell_t * get_vector_object(nanoclj_t * sc, int_fast16_t t, size_t offset, nanoclj_vector_store_t * store) {
  nanoclj_cell_t * main_cell = get_cell_x(sc, sc->EMPTY, sc->EMPTY);
  if (sc->no_memory) {
    /* TODO: free the store */
    return &(sc->_sink);
  }

  if (store) store->refcnt++;
  
  /* Record it as a vector so that gc understands it. */
  _typeflag(main_cell) = t | T_GC_ATOM;
  _size_unchecked(main_cell) = store ? store->size - offset : 0;
  _metadata_unchecked(main_cell) = mk_nil();
  _store_unchecked(main_cell) = store;
  _offset_unchecked(main_cell) = offset;

  return main_cell;
}

/* Sequence handling */

/* Returns nil or not-empty sequence */
static inline nanoclj_cell_t * seq(nanoclj_t * sc, nanoclj_cell_t * coll) {
  if (!coll) {
    return NULL;
  }
  if (_type(coll) == T_LAZYSEQ) {
    nanoclj_val_t code = cons(sc, sc->DEREF, cons(sc, mk_pointer(coll), sc->EMPTY));
    nanoclj_val_t r = nanoclj_eval(sc, code);
    coll = decode_pointer(r);
  }

  switch (_type(coll)) {
  case T_NIL:
  case T_LONG:
  case T_MAPENTRY:
  case T_ENVIRONMENT:
  case T_CLOSURE:
  case T_MACRO:
  case T_PAIR:
    return NULL;
  case T_LIST:
  case T_LAZYSEQ:
  case T_SEQ:
    break;
  case T_STRING:
  case T_CHAR_ARRAY:
    if (_strlength(coll) == 0) {
      return NULL;
    } else {
      return mk_seq(sc, coll, 0);
    }
  case T_VECTOR:
  case T_ARRAYMAP:
  case T_SORTED_SET:
    if (_size_unchecked(coll) == 0) {
      return NULL;
    } else {
      return mk_seq(sc, coll, 0);
    }
  case T_READER: {
    nanoclj_port_t * pt = _port_unchecked(coll);
    int c = inchar(sc, pt);
    if (c == EOF) {
      return NULL;
    } else {
      backchar(c, pt);
      return mk_seq(sc, coll, 0);
    }
  }
  }  
  
  return coll;
}

static inline bool is_empty(nanoclj_t * sc, nanoclj_cell_t * coll) {
  return seq(sc, coll) == NULL;
}

static inline nanoclj_cell_t * rest(nanoclj_t * sc, nanoclj_cell_t * coll) {
  int typ = _type(coll);

  if (typ == T_LAZYSEQ) {
    nanoclj_val_t code = cons(sc, sc->DEREF, cons(sc, mk_pointer(coll), sc->EMPTY));
    nanoclj_val_t r = nanoclj_eval(sc, code);
    coll = decode_pointer(r);
  }
  
  switch (typ) {
  case T_NIL:
    break;    
  case T_LIST:
  case T_LAZYSEQ:
    return decode_pointer(_cdr(coll));    
  case T_VECTOR:
  case T_ARRAYMAP:
  case T_SORTED_SET:
    if (_size_unchecked(coll) >= 2) {
      return mk_seq(sc, coll, 1);
    }
    break;
  case T_STRING:
  case T_CHAR_ARRAY:
    if (_strlength(coll)) {
      const char * ptr = _strvalue(coll);
      const char * new_ptr = utf8_next(ptr);
      if (new_ptr < ptr + _strlength(coll)) {
	return mk_seq(sc, coll, new_ptr - ptr);
      }
    }
    break;
  case T_READER:{
    nanoclj_port_t * pt = _port_unchecked(coll);
    inchar(sc, pt);
    int c = inchar(sc, pt);
    if (c != EOF) {
      backchar(c, pt);
      return mk_seq(sc, coll, 0);
    }
  }
  case T_SEQ:{    
    nanoclj_cell_t * o = _origin_unchecked(coll);
    size_t pos = _position_unchecked(coll);
    
    switch (_type(o)) {
    case T_VECTOR:
    case T_ARRAYMAP:
    case T_SORTED_SET:
      if (pos + 1 < _size_unchecked(o)) {
	return mk_seq(sc, o, pos + 1);
      }
      break;
    case T_STRING:
    case T_CHAR_ARRAY:{
      const char * ptr = _strvalue(o);
      const char * new_ptr = utf8_next(ptr + pos);
      const char * end = ptr + _strlength(o);
      if (new_ptr < end) {
	return mk_seq(sc, o, new_ptr - ptr);
      }
    }
      break;
    case T_READER:{
      nanoclj_port_t * pt = _port_unchecked(o);
      inchar(sc, pt); /* drop the first */
      int c = inchar(sc, pt);
      if (c != EOF) {
	backchar(c, pt);
	return coll;
      }
    }
    }
  }
  }

  return &(sc->_EMPTY);
}

static inline nanoclj_cell_t * next(nanoclj_t * sc, nanoclj_cell_t * coll) {
  nanoclj_cell_t * r = rest(sc, coll);
  if (_type(r) == T_LAZYSEQ) {
    nanoclj_val_t code = cons(sc, sc->DEREF, cons(sc, mk_pointer(r), sc->EMPTY));
    r = decode_pointer(nanoclj_eval(sc, code));
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
    nanoclj_val_t code = cons(sc, sc->DEREF, cons(sc, mk_pointer(coll), sc->EMPTY));
    nanoclj_val_t r = nanoclj_eval(sc, code);
    coll = decode_pointer(r);
  }
  
  switch (_type(coll)) {
  case T_NIL:
    break;
  case T_MAPENTRY:
  case T_CLOSURE:
  case T_MACRO:
  case T_ENVIRONMENT:
  case T_RATIO:
  case T_PAIR:
  case T_LIST:
  case T_VAR:
  case T_LAZYSEQ:
    return _car(coll);
  case T_VECTOR:
  case T_ARRAYMAP:
  case T_SORTED_SET:
    if (_size_unchecked(coll) > 0) {
      return vector_elem(coll, 0);
    }
    break;
  case T_STRING:
  case T_CHAR_ARRAY:
    if (_strlength(coll)) {
      return mk_character(utf8_decode(_strvalue(coll)));
    }
    break;
  case T_READER:{
    nanoclj_port_t * pt = _port_unchecked(coll);
    int c = inchar(sc, pt);
    if (c != EOF) {
      backchar(c, pt);
      if (is_binary(pt)) {
	return mk_int(c);
      } else {
	return mk_character(c);
      }
    }
    break;
  }
  case T_SEQ:{
    nanoclj_cell_t * o = _origin_unchecked(coll);
    size_t pos = _position_unchecked(coll);
    
    switch (_type(o)) {
    case T_VECTOR:
    case T_ARRAYMAP:
    case T_SORTED_SET:{
      if (pos < _size_unchecked(o)) {
	return vector_elem(o, pos);
      }
      break;
    }
    case T_STRING:
    case T_CHAR_ARRAY:
      if (pos < _strlength(o)) {
	return mk_character(utf8_decode(_strvalue(o) + pos));
      }
      break;
    case T_READER:{
      nanoclj_port_t *pt = _port_unchecked(o);
      int c = inchar(sc, pt);
      if (c != EOF) {
	backchar(c, pt);
	if (is_binary(pt)) {
	  return mk_int(c);
	} else {
	  return mk_character(c);
	}
      }
      break;
    }
    }
  }
  }

  return mk_nil();
}

static nanoclj_val_t second(nanoclj_t * sc, nanoclj_cell_t * a) {
  if (!a) return sc->EMPTY;
  int t = _type(a);
  if (t == T_NIL) {
    return sc->EMPTY;
  } else if (t == T_PAIR || t == T_MAPENTRY || t == T_VAR || t == T_RATIO) { 
    return _cdr(a);
  } else {
    return first(sc, rest(sc, a));
  }
}

/* =================== Canvas ====================== */

#ifdef WIN32
#include "nanoclj_gdi.h"
#else
#include "nanoclj_cairo.h"
#endif

/* ========== Environment implementation  ========== */

static inline int hash_fn(const char *key, int table_size) {
  unsigned int hashed = 0;
  const char *c;
  int bits_per_int = sizeof(unsigned int) * 8;

  for (c = key; *c; c++) {
    /* letters have about 5 bits in them */
    hashed = (hashed << 5) | (hashed >> (bits_per_int - 5));
    hashed ^= *c;
  }
  return hashed % table_size;
}

/*
 * In this implementation, each frame of the environment may be
 * a hash table: a vector of alists hashed by variable name.
 * In practice, we use a vector only for the initial frame;
 * subsequent frames are too small and transient for the lookup
 * speed to out-weigh the cost of making a new vector.
 */

static inline void new_frame_in_env(nanoclj_t * sc, nanoclj_val_t old_env) {
  nanoclj_val_t new_frame;

  /* The interaction-environment has about 300 variables in it. */
  if (old_env.as_long == sc->EMPTY.as_long) {
    nanoclj_cell_t * vec = get_vector_object(sc, T_VECTOR, 0, mk_vector_store(sc, OBJ_LIST_SIZE));
    fill_vector(vec, sc->EMPTY);
    new_frame = mk_pointer(vec);
  } else {
    new_frame = sc->EMPTY;
  }

  sc->envir = mk_pointer(get_cell(sc, T_ENVIRONMENT, new_frame, old_env, mk_nil()));
}

static inline void new_slot_spec_in_env(nanoclj_t * sc, nanoclj_val_t env,
					nanoclj_val_t variable, nanoclj_val_t value) {
  nanoclj_cell_t * slot0 = get_cell(sc, T_VAR, variable, value, mk_nil());
  _watches_unchecked(slot0) = sc->EMPTY;
  
  nanoclj_val_t slot = mk_pointer(slot0);
  nanoclj_val_t x = car(env);
  
  if (is_vector(x)) {
    nanoclj_cell_t * vec = decode_pointer(x);
    int location = hash_fn(symname(variable), _size_unchecked(vec));
    set_vector_elem(vec, location, cons(sc, slot, vector_elem(vec, location)));
  } else {     
    car(env) = cons(sc, slot, car(env));
  }
}

static inline nanoclj_val_t find_slot_in_env(nanoclj_t * sc, nanoclj_val_t env, nanoclj_val_t hdl, bool all) {
  nanoclj_val_t x, y;

  for (x = env; x.as_long != sc->EMPTY.as_long; x = cdr(x)) {
    y = car(x);
    if (is_vector(y)) {
      nanoclj_cell_t * c = decode_pointer(y);
      int location = hash_fn(symname(hdl), _size_unchecked(c));
      y = vector_elem(c, location);
    }

    for (; y.as_long != sc->EMPTY.as_long; y = cdr(y)) {
      nanoclj_val_t sym = caar(y);
      if (sym.as_long == hdl.as_long) {
	break;
      }
    }
    if (y.as_long != sc->EMPTY.as_long) {
      break;
    }
    if (!all) {
      return sc->EMPTY;
    }
  }
  if (x.as_long != sc->EMPTY.as_long) {
    return car(y);
  }
  return sc->EMPTY;
}

static inline void new_slot_in_env(nanoclj_t * sc, nanoclj_val_t variable, nanoclj_val_t value) {
  new_slot_spec_in_env(sc, sc->envir, variable, value);
}

static inline nanoclj_val_t set_slot_in_env(nanoclj_t * sc, nanoclj_val_t slot0, nanoclj_val_t state) {
  nanoclj_cell_t * slot = decode_pointer(slot0);
  nanoclj_val_t old_state = _cdr(slot);
  _cdr(slot) = state;
  nanoclj_cell_t * watches = decode_pointer(_watches_unchecked(slot));
  for (; !is_empty(sc, watches); watches = rest(sc, watches)) {
    nanoclj_val_t fn = first(sc, watches);
    nanoclj_call(sc, fn, cons(sc, mk_nil(), cons(sc, slot0, cons(sc, old_state, cons(sc, state, sc->EMPTY)))));
  }
  return state;
}

static inline nanoclj_val_t slot_value_in_env(nanoclj_val_t slot) {
  return cdr(slot);
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
      case T_CHAR_ARRAY:{
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
      case T_SORTED_SET:{
	size_t la = _size_unchecked(a2), lb = _size_unchecked(b2);
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
	
      case T_PAIR:
      case T_LIST:
      case T_MAPENTRY:
      case T_CLOSURE:
      case T_LAZYSEQ:
      case T_MACRO:
      case T_ENVIRONMENT:{
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
  if (_size_unchecked(vec) > 0) {
    qsort(_store_unchecked(vec)->data + _offset_unchecked(vec)
	  , _size_unchecked(vec), sizeof(nanoclj_val_t), _compare);
  }
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
      case T_CHAR_ARRAY:{
	strview_t sv1 = _to_strview(a), sv2 = _to_strview(b);
	return sv1.size == sv2.size && strncmp(sv1.ptr, sv2.ptr, sv1.size) == 0;
      }
      case T_LONG:
	return _lvalue_unchecked(a) == _lvalue_unchecked(b);
      case T_VECTOR:
      case T_SORTED_SET:{
	size_t l = _size_unchecked(a);
	if (l == _size_unchecked(b)) {
	  for (size_t i = 0; i < l; i++) {
	    if (!equals(sc, vector_elem(a, i), vector_elem(b, i))) {
	      return 0;
	    }
	  }
	  return 1;
	}
      }
	break;
      case T_PAIR:
      case T_LIST:
      case T_MAPENTRY:
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
      for (; !is_empty(sc, a) && !is_empty(sc, b); a = rest(sc, a), b = rest(sc, b)) {
	if (!equals(sc, first(sc, a), first(sc, b))) {
	  return 0;
	}
      }
      if (is_empty(sc, a) && is_empty(sc, b)) {
	return 1;
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

static int hasheq(nanoclj_val_t v) { 
  switch (prim_type(v)) {
  case T_BOOLEAN:
  case T_CHARACTER:
  case T_PROC:
  case T_TYPE:
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
    case T_CHAR_ARRAY:{
      strview_t sv = _to_strview(c);
      int hashcode = 0, m = 1;
      for (int i = (int)sv.size - 1; i >= 0; i--) {
	hashcode += sv.ptr[i] * m;
	m *= 31;
      }
      return murmur3_hash_int(hashcode);
    }

    case T_MAPENTRY:{
      uint32_t hash = 1;
      hash = 31 * hash + (uint32_t)hasheq(_car(c));
      hash = 31 * hash + (uint32_t)hasheq(_cdr(c));
      return murmur3_hash_coll(hash, 2);
    }      

    case T_VECTOR:
    case T_SORTED_SET:{
      uint32_t hash = 1;
      size_t n = _size_unchecked(c);
      for (size_t i = 0; i < n; i++) {
	hash = 31 * hash + (uint32_t)hasheq(vector_elem(c, i));	
      }
      return murmur3_hash_coll(hash, n);
    }

    case T_ARRAYMAP:{
      uint32_t hash = 0;
      size_t n = _size_unchecked(c);
      for (size_t i = 0; i < n; i++) {
	hash += (uint32_t)hasheq(vector_elem(c, i));
      }
      return murmur3_hash_coll(hash, n);
    }

    case T_LIST:
    case T_PAIR:{
      uint32_t hash = 31 + hasheq(_car(c));
      size_t n = 1;
      for ( nanoclj_val_t x = _cdr(c) ; is_pair(x); x = cdr(x), n++) {
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
  car(sc->sink) = sc->EMPTY;
}


#if defined TSGRIND
static void check_cell_alloced(nanoclj_val_t p, int expect_alloced) {
  /* Can't use putstr(sc,str) because callers have no access to
     sc.  */
  if (typeflag(p) & !expect_alloced) {
    fprintf(stderr, "Cell is already allocated!\n");
  }
  if (!(typeflag(p)) & expect_alloced) {
    fprintf(stderr, "Cell is not allocated!\n");
  }

}
static void check_range_alloced(nanoclj_val_t p, int n, int expect_alloced) {
  int i;
  for (i = 0; i < n; i++) {
    (void) check_cell_alloced(p + i, expect_alloced);
  }
}

#endif

/* ========== oblist implementation  ========== */

static inline nanoclj_cell_t * oblist_initial_value(nanoclj_t * sc) {
  nanoclj_cell_t * vec = get_vector_object(sc, T_VECTOR, 0, mk_vector_store(sc, OBJ_LIST_SIZE));
  fill_vector(vec, sc->EMPTY);
  return vec;
}

static inline nanoclj_val_t oblist_add_item(nanoclj_t * sc, const char * name, nanoclj_val_t v) {
  int location = hash_fn(name, _size_unchecked(sc->oblist));
  set_vector_elem(sc->oblist, location, cons(sc, v, vector_elem(sc->oblist, location)));
  return v;
}

/* returns the new symbol */
static inline nanoclj_val_t oblist_add_symbol_by_name(nanoclj_t * sc, const char *name, size_t len) {
  /* This is leaked intentionally for now */
  char * str = (char*)sc->malloc(len + 1);
  memcpy(str, name, len);
  str[len] = 0;
  return oblist_add_item(sc, name, mk_symbol(sc, str, 0));
}

/* returns the new keyword */
static inline nanoclj_val_t oblist_add_keyword_by_name(nanoclj_t * sc, const char *name, size_t len) {
  /* This is leaked intentionally for now */
  char * str = (char*)malloc(len + 1);
  memcpy(str, name, len);
  str[len] = 0;
  return oblist_add_item(sc, name, mk_keyword(str));
}

static inline nanoclj_val_t oblist_find_symbol_by_name(nanoclj_t * sc, const char *name, size_t len) {
  int location = hash_fn(name, _size_unchecked(sc->oblist));
  nanoclj_val_t x = vector_elem(sc->oblist, location);
  for (; x.as_long != sc->EMPTY.as_long; x = cdr(x)) {
    nanoclj_val_t y = car(x);
    if (is_symbol(y)) {
      const char * s = symname(y);
      if (strncmp(name, s, len) == 0) {
	return y;
      }
    }
  }
  return sc->EMPTY;
}

static inline nanoclj_val_t oblist_find_keyword_by_name(nanoclj_t * sc, const char *name, size_t len) {
  int location = hash_fn(name, _size_unchecked(sc->oblist));
  nanoclj_val_t x = vector_elem(sc->oblist, location);
  for (; x.as_long != sc->EMPTY.as_long; x = cdr(x)) {
    nanoclj_val_t y = car(x);
    if (is_keyword(y)) {
      const char * s = keywordname(y);
      if (strncmp(name, s, len) == 0) {
	return y;
      }
    }
  }
  return sc->EMPTY;
}

static inline nanoclj_val_t mk_vector(nanoclj_t * sc, size_t len) {
  nanoclj_vector_store_t * s = NULL;
  if (len > 0) s = mk_vector_store(sc, len);
  return mk_pointer(get_vector_object(sc, T_VECTOR, 0, s));
}

static inline nanoclj_val_t mk_vector_2d(nanoclj_t * sc, double a, double b) {
  nanoclj_cell_t * vec = get_vector_object(sc, T_VECTOR, 0, mk_vector_store(sc, 2));
  set_vector_elem(vec, 0, mk_real(a));
  set_vector_elem(vec, 1, mk_real(b));
  return mk_pointer(vec);
}

/* Copies a vector so that the copy can be mutated */
static inline nanoclj_cell_t * copy_vector(nanoclj_t * sc, nanoclj_cell_t * vec) {
  size_t len = _size_unchecked(vec);
  nanoclj_vector_store_t * s = NULL;
  if (len > 0) {
    s = mk_vector_store(sc, len);
    memcpy(s->data, _store_unchecked(vec)->data + _offset_unchecked(vec), len * sizeof(nanoclj_val_t));
  }
  return get_vector_object(sc, _type(vec), 0, s);
}

static inline nanoclj_cell_t * conj(nanoclj_t * sc, nanoclj_cell_t * coll, nanoclj_val_t new_value) {
  if (!coll) coll = &(sc->_EMPTY);
	       
  int t = _type(coll);
  if (t == T_PAIR || t == T_NIL || t == T_LIST) {
    if (t == T_NIL) t = T_LIST;
    return get_cell(sc, t, new_value, mk_pointer(coll), mk_nil());
  } else {
    nanoclj_vector_store_t * s = _store_unchecked(coll);
    size_t old_size = _size_unchecked(coll);
    size_t old_offset = _offset_unchecked(coll);
    
    if (!s) {
      s = mk_vector_store(sc, 0);
    } else if (!(old_offset + old_size == s->size && s->size < s->reserved)) {
      nanoclj_vector_store_t * old_s = s;
      s = mk_vector_store(sc, old_size);
      memcpy(s->data, old_s->data + old_offset, old_size * sizeof(nanoclj_val_t));    
    }
    
    s->data[s->size++] = new_value;  
    return get_vector_object(sc, t, old_offset, s);
  }
}

static inline nanoclj_cell_t * subvec(nanoclj_t * sc, nanoclj_cell_t * vec, size_t offset, size_t len) {
  nanoclj_vector_store_t * s = _store_unchecked(vec);
  size_t old_size = _size_unchecked(vec);
  size_t old_offset = _offset_unchecked(vec);

  if (offset >= old_size) {
    len = 0;
  } else if (offset + len > old_size) {
    len = old_size - offset;
  }
  
  nanoclj_cell_t * new_vec = get_vector_object(sc, T_VECTOR, old_offset + offset, s);
  _size_unchecked(new_vec) = len;
  return new_vec;
}

static inline nanoclj_cell_t * mk_sorted_set(nanoclj_t * sc, int len) {
  return get_vector_object(sc, T_SORTED_SET, 0, len > 0 ? mk_vector_store(sc, len) : NULL);
}

static inline bool get_elem(nanoclj_t * sc, nanoclj_cell_t * coll, nanoclj_val_t key, nanoclj_val_t * result) {
  switch (_type(coll)) {
  case T_VECTOR:
    if (is_number(key)) {
      long long index = to_long(key);
      if (index >= 0 && index < _size_unchecked(coll)) {
	if (result) *result = vector_elem(coll, index);
	return true;
      }
    }
    break;

  case T_STRING:
  case T_CHAR_ARRAY:
    if (is_number(key)) {
      long long index = to_long(key);
      const char * str = _strvalue(coll);
      const char * end = str + _strlength(coll);
      
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
    size_t size = _size_unchecked(coll);
    for (int i = 0; i < size; i++) {
      nanoclj_val_t pair = vector_elem(coll, i);
      if (compare(key, car(pair)) == 0) {
	if (result) *result = cdr(pair);
	return true;
      }    
    }
  }
    break;

  case T_SORTED_SET:{
    size_t size = _size_unchecked(coll);
    for (int i = 0; i < size; i++) {
      nanoclj_val_t value = vector_elem(coll, i);
      if (compare(key, value) == 0) {
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
  }
  
  return false;
}

/* get new keyword */
static inline nanoclj_val_t def_keyword_from_sv(nanoclj_t * sc, strview_t sv) {
  /* first check oblist */
  nanoclj_val_t x = oblist_find_keyword_by_name(sc, sv.ptr, sv.size);
  if (x.as_long != sc->EMPTY.as_long) {
    return x;
  } else {
    return oblist_add_keyword_by_name(sc, sv.ptr, sv.size);
  }
}

/* get new symbol */
static inline nanoclj_val_t def_symbol_from_sv(nanoclj_t * sc, strview_t sv) {
  /* first check oblist */
  nanoclj_val_t x = oblist_find_symbol_by_name(sc, sv.ptr, sv.size);
  if (x.as_long != sc->EMPTY.as_long) {
    return x;
  } else {
    return oblist_add_symbol_by_name(sc, sv.ptr, sv.size);
  }
}

static inline nanoclj_val_t def_keyword(nanoclj_t * sc, const char *name) {
  return def_keyword_from_sv(sc, (strview_t){ name, strlen(name) });
}

static inline nanoclj_val_t def_symbol(nanoclj_t * sc, const char *name) {
  return def_symbol_from_sv(sc, (strview_t){ name, strlen(name) });
}

/* get new symbol or keyword */
static inline nanoclj_val_t def_symbol_or_keyword(nanoclj_t * sc, const char *name) {
  if (name[0] == ':') return def_keyword(sc, name + 1);
  else return def_symbol(sc, name);
}

static inline nanoclj_val_t def_namespace_with_sym(nanoclj_t *sc, nanoclj_val_t sym) {
  nanoclj_val_t ns = mk_pointer(get_cell(sc, T_ENVIRONMENT, sc->EMPTY, sc->root_env, mk_string(sc, symname(sym))));
  nanoclj_intern(sc, sc->global_env, sym, ns);
  return ns;
}

static inline nanoclj_val_t def_namespace(nanoclj_t * sc, const char *name) {
  return def_namespace_with_sym(sc, def_symbol(sc, name));
}

static inline nanoclj_val_t gensym(nanoclj_t * sc, const char * prefix, size_t prefix_len) {
  nanoclj_val_t x;
  char name[256];

  for (; sc->gensym_cnt < LONG_MAX; sc->gensym_cnt++) {
    size_t len = snprintf(name, 256, "%.*s%ld", (int)prefix_len, prefix, sc->gensym_cnt);
    
    /* first check oblist */
    x = oblist_find_symbol_by_name(sc, name, len);

    if (x.as_long != sc->EMPTY.as_long) {
      continue;
    } else {
      return oblist_add_symbol_by_name(sc, name, len);
    }
  }

  return sc->EMPTY;
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
    return cons(sc, sc->SLASH_HOOK,
		cons(sc,
		     cons(sc,
			  sc->QUOTE,
			  cons(sc, mk_primitive(sc, p + 1), sc->EMPTY)),
		     cons(sc, def_symbol(sc, q), sc->EMPTY)));
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
        return sc->EMPTY;
      }   
    } else if (name[2] == 0) {
      c = name[1];
    } else if (*(utf8_next(name + 1)) == 0) {
      c = utf8_decode(name + 1);
    } else {
      return sc->EMPTY;
    }
    return mk_character(c);
  } else {
    return sc->EMPTY;
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
    return sc->EMPTY;
  }
}

/* ========== garbage collector ========== */

/*--
 *  We use algorithm E (Knuth, The Art of Computer Programming Vol.1,
 *  sec. 2.3.5), the Schorr-Deutsch-Waite link-inversion algorithm,
 *  for marking.
 */
static inline void mark(nanoclj_val_t a) {
  if (is_primitive(a)) {
    return;
  }
  
  nanoclj_cell_t * t = NULL;
  nanoclj_cell_t * p = decode_pointer(a);
  nanoclj_cell_t * q;
  nanoclj_val_t q0;

E2:_setmark(p);
  switch (_type(p)) {
  case T_VECTOR:
  case T_ARRAYMAP:
  case T_SORTED_SET:{
    size_t offset = _offset_unchecked(p);
    size_t num = _size_unchecked(p);
    nanoclj_val_t * data = _store_unchecked(p)->data;
    for (size_t i = 0; i < num; i++) {
      /* Vector cells will be treated like ordinary cells */
      mark(data[offset + i]);
    }
  }
    break;
  case T_SEQ:
    mark(mk_pointer(_origin_unchecked(p)));
    break;
  }

  nanoclj_val_t metadata = _metadata_unchecked(p);
  if (!is_nil(metadata)) {
    mark(metadata);
  }
  
  if (_is_gc_atom(p))
    goto E6;
  /* E4: down car */
  q0 = _car(p);
  if (!is_primitive(q0) && !is_nil(q0) && !is_mark(q0)) {
    _set_gc_atom(p);                 /* a note that we have moved car */
    _car(p) = mk_pointer(t);
    t = p;
    p = decode_pointer(q0);
    goto E2;
  }
E5:q0 = _cdr(p);                 /* down cdr */
  if (!is_primitive(q0) && !is_nil(q0) && !is_mark(q0)) {
    _cdr(p) = mk_pointer(t);
    t = p;
    p = decode_pointer(q0);
    goto E2;
  }
E6:                            /* up.  Undo the link switching from steps E4 and E5. */
  if (!t) {
    return;
  }

  q = t;
  if (_is_gc_atom(q)) {
    _clr_gc_atom(q);
    t = decode_pointer(_car(q));
    _car(q) = mk_pointer(p);
    p = q;
    goto E5;
  } else {
    t = decode_pointer(_cdr(q));
    _cdr(q) = mk_pointer(p);
    p = q;
    goto E6;
  }
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
  switch (_type(a)) {
  case T_STRING:
  case T_CHAR_ARRAY:
    if (!_is_small(a)) {
      sc->free(_strvalue_unchecked(a));
    }
    break;
  case T_VECTOR:
  case T_ARRAYMAP:
  case T_SORTED_SET:{
    nanoclj_vector_store_t * s = _store_unchecked(a);
    if (s) {
      s->refcnt--;
      if (s->refcnt == 0) finalize_vector_store(sc, s);
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
  case T_CANVAS:
#if NANOCLJ_HAS_CANVAS
    finalize_canvas(sc, _canvas_unchecked(a));
    _canvas_unchecked(a) = NULL;
#endif
    break;
  }
}

/* garbage collection. parameter a, b is marked. */
static void gc(nanoclj_t * sc, nanoclj_val_t a, nanoclj_val_t b) {
#if GC_VERBOSE
  putstr(sc, "gc...", get_err_port(sc));
#endif

  /* mark system globals */
  if (sc->oblist) mark(mk_pointer(sc->oblist));
  mark(sc->global_env);

  /* mark current registers */
  mark(sc->args);
  mark(sc->envir);
  mark(sc->code);
#ifdef USE_RECUR_REGISTER
  mark(sc->recur);
#endif
  dump_stack_mark(sc);
  mark(sc->value);
  mark(sc->save_inport);
  mark(sc->loadport);

  mark(sc->EMPTYSTR);

#if 0
  mark(sc->active_element);
#endif
  
#if 0
  fprintf(stderr, "marking sink %p\n", (void *)sc->sink);
#endif
  
  /* Mark recent objects the interpreter doesn't know about yet. */
  mark(car(sc->sink));

#if 0
  fprintf(stderr, "marking sink done\n");
#endif
  
  /* Mark any older stuff above nested C calls */
  mark(sc->c_nest);

  /* mark variables a, b */
  mark(a);
  mark(b);

  /* garbage collect */
  clrmark(sc->EMPTY);
  sc->fcells = 0;
  nanoclj_cell_t * free_cell = decode_pointer(sc->EMPTY);

  /* free-list is kept sorted by address so as to maintain consecutive
     ranges, if possible, for use with vectors. Here we scan the cells
     (which are also kept sorted by address) downwards to build the
     free-list in sorted order.
   */

  long total_cells = 0;
  
  for (int_fast32_t i = sc->last_cell_seg; i >= 0; i--) {
    nanoclj_cell_t * min_p = decode_pointer(sc->cell_seg[i]);
    nanoclj_cell_t * p = min_p + CELL_SEGSIZE;

    while (--p >= min_p) {
      total_cells++;
      
      if (_is_mark(p)) {
	_clrmark(p);
      } else {
        /* reclaim cell */
        if (_typeflag(p) != 0) {
	  finalize_cell(sc, p);
          _typeflag(p) = 0;
          _car(p) = sc->EMPTY;
        }
        ++sc->fcells;
	_cdr(p) = mk_pointer(free_cell);
        free_cell = p;
      }
    }
  }

  sc->free_cell = free_cell;

#if GC_VERBOSE
  char msg[80];
  sprintf(msg, "done: %ld / %ld cells were recovered.\n", sc->fcells, total_cells);
  putstr(sc, msg, get_err_port(sc));
#endif
}

/* ========== Routines for Reading ========== */

static inline int file_push(nanoclj_t * sc, strview_t sv) {
  if (sc->file_i == MAXFIL - 1) {
    return 0;
  }
  char * fname = (char *)sc->malloc(sv.size + 1);
  memcpy(fname, sv.ptr, sv.size);
  fname[sv.size] = 0;
  
  FILE * fin = fopen(fname, "r");
  sc->free(fname);
  
  if (fin != 0) {
    sc->file_i++;
    sc->load_stack[sc->file_i].kind = port_file | port_input;
    sc->load_stack[sc->file_i].backchar = -1;
    sc->load_stack[sc->file_i].rep.stdio.file = fin;
    sc->nesting_stack[sc->file_i] = 0;
    port_unchecked(sc->loadport) = sc->load_stack + sc->file_i;

    sc->load_stack[sc->file_i].rep.stdio.curr_line = 0;
    if (fname) {
      sc->load_stack[sc->file_i].rep.stdio.filename = store_string(sc, strlen(fname), fname);
    }
  }
  return fin != 0;
}

static inline void file_pop(nanoclj_t * sc) {
  if (sc->file_i != 0) {
    sc->nesting = sc->nesting_stack[sc->file_i];
    port_close(sc, port_unchecked(sc->loadport));
    sc->file_i--;
    port_unchecked(sc->loadport) = sc->load_stack + sc->file_i;
  }
}

static inline nanoclj_port_t *port_rep_from_file(nanoclj_t * sc, FILE * f, int prop) {
  nanoclj_port_t *pt;

  pt = (nanoclj_port_t *) sc->malloc(sizeof *pt);
  if (pt == NULL) {
    return NULL;
  }
  pt->kind = port_file | prop;
  pt->backchar = -1;
  pt->rep.stdio.file = f;
  return pt;
}

static inline nanoclj_val_t port_from_filename(nanoclj_t * sc, strview_t sv, int prop) {
  const char * mode;
  if (prop == (port_input | port_output)) {
    mode = "a+";
  } else if (prop == port_output) {
    mode = "w";
  } else {
    mode = "r";
  }
  char * fn = alloc_c_str(sc, sv);
  FILE * f = fopen(fn, mode);
  
  nanoclj_val_t r = mk_nil();
  if (f) {
    nanoclj_port_t * pt = port_rep_from_file(sc, f, prop);
    if (fn) {
      pt->rep.stdio.filename = store_string(sc, sv.size, sv.ptr);
    }
    pt->rep.stdio.curr_line = 0;
    
    if (prop & port_input) {
      r = mk_reader(sc, pt);
    } else {
      r = mk_writer(sc, pt);
    }
  }

  sc->free(fn);
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
  nanoclj_port_t *pt = (nanoclj_port_t *)sc->malloc(sizeof *pt);
  if (!pt) {
    return sc->EMPTY;
  }
  pt->kind = port_callback | prop;
  pt->backchar = -1;
  pt->rep.callback.text = text;
  pt->rep.callback.color = color;
  pt->rep.callback.restore = restore;
  pt->rep.callback.image = image;

  return mk_writer(sc, pt);
}

static inline nanoclj_val_t port_from_string(nanoclj_t * sc, strview_t sv, int prop) {
  nanoclj_port_t * pt = (nanoclj_port_t *)sc->malloc(sizeof(nanoclj_port_t));
  if (!pt) {
    return sc->EMPTY;
  }
  char * buffer = (char *)sc->malloc(sv.size);
  memcpy(buffer, sv.ptr, sv.size);
  
  pt->kind = port_string | prop;
  pt->backchar = -1;
  pt->rep.string.curr = buffer;
  pt->rep.string.data.data = buffer;
  pt->rep.string.data.size = sv.size;

  if (prop & port_input) {
    return mk_reader(sc, pt);
  } else {
    return mk_writer(sc, pt);
  }
}

#define BLOCK_SIZE 256

static inline nanoclj_val_t port_from_scratch(nanoclj_t * sc) {
  nanoclj_port_t * pt = (nanoclj_port_t *)sc->malloc(sizeof(nanoclj_port_t));
  if (!pt) {
    return mk_nil();
  }
  char * start = sc->malloc(BLOCK_SIZE);
  if (!start) {
    sc->free(pt);
    return mk_nil();
  }
  pt->kind = port_string | port_output;
  pt->backchar = -1;
  pt->rep.string.curr = start;
  pt->rep.string.data.data = start;
  pt->rep.string.data.size = BLOCK_SIZE;

  return mk_writer(sc, pt);
}

static inline int realloc_port_string(nanoclj_t * sc, nanoclj_port_t * p) {
  char *start = p->rep.string.data.data;
  size_t new_size = p->rep.string.data.size + BLOCK_SIZE;
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
      *s = 0;
      return 1;
    } else {
      int bytes = (c < 0x800) ? 2 : ((c < 0x10000) ? 3 : 4);
      int len = bytes;
      s[bytes] = 0;
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

static inline void putchars(nanoclj_t * sc, const char *s, size_t len, nanoclj_val_t out) {
  assert(s);

  if (is_writer(out)) {
    nanoclj_port_t * pt = port_unchecked(out);
    if (pt->kind & port_file) {
      fwrite(s, 1, len, pt->rep.stdio.file);
    } else if (pt->kind & port_callback) {
      pt->rep.callback.text(s, len, sc->ext_data);
    } else {
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
    canvas_show_text(canvas, (strview_t){ s, len });
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
      if (isatty(fileno(fh))) {
	set_term_color(fh,
		       to_double(vector_elem(vec, 0)),
		       to_double(vector_elem(vec, 1)),
		       to_double(vector_elem(vec, 2)),
		       sc->term_colors
		       );
      }
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
    int c = inchar(sc, inport);
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
    c = inchar(sc, inport);
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
        *p = 0;
        return mk_counted_string(sc, sc->strbuff, p - sc->strbuff);
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
static inline int skipspace(nanoclj_t * sc) {
  int c = 0, curr_line = 0;
  nanoclj_port_t * inport = port_unchecked(get_in_port(sc));

  do {
    c = inchar(sc, inport);
    if (c == '\n') {
      curr_line++;
    }
  } while (isspace(c));

/* record it */
  if (sc->load_stack[sc->file_i].kind & port_file) {
    sc->load_stack[sc->file_i].rep.stdio.curr_line += curr_line;
  }

  return c;
}

/* get token */
static inline int token(nanoclj_t * sc, nanoclj_port_t * inport) {
  int c;
  switch (c = skipspace(sc)) {
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
    c = inchar(sc, inport);
    if (is_one_of(" \n\t", c)) {
      return (TOK_DOT);
    } else {
      /* FIXME: cannot backchar two values */
      backchar(c, inport);
      backchar('.', inport);
      return TOK_PRIMITIVE;
    }
  case '\'':
    return (TOK_QUOTE);
  case '@':
    return (TOK_DEREF);
  case ';':
    while ((c = inchar(sc, inport)) != '\n' && c != EOF) {
    }

    if (c == '\n' && sc->load_stack[sc->file_i].kind & port_file) {
      sc->load_stack[sc->file_i].rep.stdio.curr_line++;
    }

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
    if ((c = inchar(sc, inport)) == '@') {
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
    c = inchar(sc, inport);
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
      while ((c = inchar(sc, inport)) != '\n' && c != EOF) {
      }

      if (c == '\n' && sc->load_stack[sc->file_i].kind & port_file) {
        sc->load_stack[sc->file_i].rep.stdio.curr_line++;
      }

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
#endif

static inline void print_image(nanoclj_t * sc, nanoclj_image_t * img, nanoclj_val_t out) {
  assert(is_writer(out));
  if (!is_writer(out)) {
    return;
  }

  nanoclj_port_t * pt = port_unchecked(out);

  if ((pt->kind & port_callback) && pt->rep.callback.image) {
    pt->rep.callback.image(img, sc->ext_data);
  } else {
    if (sc->sixel_term) {
#if NANOCLJ_SIXEL
      int width = img->width, height = img->height;
      unsigned char * tmp = malloc(3 * width * height);
      for (unsigned int i = 0; i < width * height; i++) {
	tmp[3 * i + 0] = img->data[4 * i + 0];
	tmp[3 * i + 1] = img->data[4 * i + 1];
	tmp[3 * i + 2] = img->data[4 * i + 2];
      }
      sixel_dither_t * dither;
      sixel_dither_new(&dither, -1, NULL);
      sixel_dither_initialize(dither, tmp, width, height, SIXEL_PIXELFORMAT_RGB888, SIXEL_LARGE_NORM, SIXEL_REP_CENTER_BOX, SIXEL_QUALITY_HIGHCOLOR);

      sixel_output_t * output = NULL;
      sixel_output_new(&output, sixel_write, stdout, NULL);
      
      /* convert pixels into sixel format and write it to output context */
      sixel_encode(tmp,
		   img->width,
		   img->height,
		   8,
		   dither,
		   output);
      sc->free(tmp);
      sixel_output_destroy(output);
      sixel_dither_destroy(dither);
      return;
#endif      
    }

    char * p = sc->strbuff;
    size_t len = snprintf(sc->strbuff, sc->strbuff_size, "#object[%s %p %d %d %d]", typename_from_id(T_IMAGE), (void *)img, img->width, img->height, img->channels);
    putchars(sc, p, 3, out);
  }
}

/* Uses internal buffer unless string pointer is already available */
static inline void print_primitive(nanoclj_t * sc, nanoclj_val_t l, int print_flag, nanoclj_val_t out) {
  const char *p = 0;
  int plen = -1;

  switch (prim_type(l)) {
  case T_BOOLEAN:
    p = decode_integer(l) == 0 ? "false" : "true";
    break;
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
      plen = sprintf(sc->strbuff, "%.15g", rvalue_unchecked(l));	
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
	  char_to_utf8(c, sc->strbuff + 1);
	}
        break;
      }
    }
    break;
  case T_SYMBOL:
    p = symname(l);
    break;
  case T_KEYWORD:
    p = sc->strbuff;
    plen = snprintf(sc->strbuff, sc->strbuff_size, ":%s", keywordname(l));
    break;
  case T_TYPE:
    p = typename_from_id(decode_integer(l));
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
      case T_LONG:
	p = sc->strbuff;
	plen = sprintf(sc->strbuff, "%lld", _lvalue_unchecked(c));
	break;
      case T_STRING:	
	if (!print_flag) {
	  p = _strvalue(c);
	  plen = _strlength(c);
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
      case T_ENVIRONMENT:
	p = sc->strbuff;
	plen = snprintf(sc->strbuff, sc->strbuff_size, "#object[%s %p %s]", typename_from_id(T_ENVIRONMENT), (void *)c, "?");
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
	    if (l.as_long == sc->active_element.as_long && 0) {
	  
	    } else {
	      int x, y;
	      if (fh == stdout) {
		sc->active_element = l;
		if (0 && get_cursor_position(stdin, fh, &x, &y)) {
		  sc->active_element_x = x;
		  sc->active_element_y = y;
		}
	      } else {
		sc->active_element = mk_nil();
	      }
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
    p = sc->strbuff;
    plen = snprintf(sc->strbuff, sc->strbuff_size, "#object[%s]", typename_from_id(type(l)));
  }
  
  if (plen < 0) {
    plen = strlen(p);
  }
  
  putchars(sc, p, plen, out);
}

static inline size_t seq_length(nanoclj_t * sc, nanoclj_cell_t * a) {
  size_t i = 0;  
  switch (_type(a)) {
  case T_NIL:
    return 0;

  case T_LONG:
    return 1;
    
  case T_PAIR:
  case T_VAR:
  case T_RATIO:
    return 2;    
    
  case T_SEQ:
    /* Seq is never empty */
    for (; a; a = next(sc, a), i++) { }
    return i;

  case T_VECTOR:
    return _size_unchecked(a);

  case T_LIST:
  case T_LAZYSEQ:
#if 1
    for (; !is_empty(sc, a); a = rest(sc, a), i++) { }
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

static inline nanoclj_cell_t * mk_arraymap(nanoclj_t * sc, size_t len) {
  return get_vector_object(sc, T_ARRAYMAP, 0, len > 0 ? mk_vector_store(sc, len) : NULL);
}

static inline nanoclj_val_t mk_mapentry(nanoclj_t * sc, nanoclj_val_t key, nanoclj_val_t val) {
  return mk_pointer(get_cell(sc, T_MAPENTRY, key, val, mk_nil()));
}

static inline nanoclj_val_t mk_collection(nanoclj_t * sc, int type, nanoclj_cell_t * args) {
  size_t len = seq_length(sc, args);

  nanoclj_cell_t * coll;
  switch (type) {
  case T_VECTOR:
    coll = get_vector_object(sc, T_VECTOR, 0, len > 0 ? mk_vector_store(sc, len) : NULL);  
    break;
  case T_ARRAYMAP:
    len /= 2;
    coll = get_vector_object(sc, T_ARRAYMAP, 0, len > 0 ? mk_vector_store(sc, len) : NULL);
    break;
  case T_SORTED_SET:
    coll = get_vector_object(sc, T_SORTED_SET, 0, len > 0 ? mk_vector_store(sc, len) : NULL);
    break;
  default:
    return sc->EMPTY;
  }
  if (sc->no_memory) {
    return sc->sink;
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
  return mk_pointer(coll);
}

static inline nanoclj_val_t mk_format_va(nanoclj_t * sc, const char *fmt, ...) {
  va_list ap;

  /* Determine required size */
  va_start(ap, fmt);
  int n = vsnprintf(NULL, 0, fmt, ap);
  va_end(ap);
  
  if (n < 0) {
    return sc->EMPTY;
  }
  
  /* One extra byte for '\0' */
  size_t size = (size_t) n + 1;
  char * p = sc->malloc(size);
  if (p == NULL) {
    return sc->sink;
  }
    
  va_start(ap, fmt);
  n = vsnprintf(p, size, fmt, ap);
  va_end(ap);
  
  nanoclj_val_t r = sc->EMPTY;
  if (n >= 0) {
    r = mk_counted_string(sc, p, n);
  }
  free(p);
  return r;
}

static inline nanoclj_val_t mk_format(nanoclj_t * sc, const char * fmt, nanoclj_cell_t * args) {
  int n_args = 0;
  long long arg0l = 0, arg1l = 0;
  double arg0d = 0.0, arg1d = 0.0;
  const char * arg0s = NULL, * arg1s = NULL;
  int plan = 0, m = 1;
  
  for (args = seq(sc, args); args; args = next(sc, args), n_args++, m *= 4) {
    nanoclj_val_t arg = first(sc, args);
    long long l;
    double d;
    const char * s;
    
    switch (type(arg)) {
    case T_INTEGER:
    case T_LONG:
    case T_TYPE:
    case T_PROC:
    case T_BOOLEAN:
      l = to_long(arg);
      switch (n_args) {
      case 0: arg0l = l; break;
      case 1: arg1l = l; break;
      }
      plan += 1 * m;
      break;
    case T_CHARACTER:
      l = decode_integer(arg);
      switch (n_args) {
      case 0: arg0l = l; break;
      case 1: arg1l = l; break;
      }
      plan += 1 * m;
      break;
    case T_REAL:
      d = rvalue_unchecked(arg);
      switch (n_args) {
      case 0: arg0d = d; break;
      case 1: arg1d = d; break;
      }
      plan += 2 * m;    
      break;      
    case T_STRING:
      s = strvalue(arg);
      switch (n_args) {
      case 0: arg0s = s; break;
      case 1: arg1s = s; break;
      }
      plan += 3 * m;
      break;
    case T_KEYWORD:
      s = keywordname(arg);
      switch (n_args) {
      case 0: arg0s = s; break;
      case 1: arg1s = s; break;
      }
      plan += 3 * m;
      break;
    case T_SYMBOL:
      s = symname(arg);
      switch (n_args) {
      case 0: arg0s = s; break;
      case 1: arg1s = s; break;
      }
      plan += 3 * m;
      break;
    }
  }
  
  switch (plan) {
  case 0: return mk_format_va(sc, fmt);
    
  case 1: return mk_format_va(sc, fmt, arg0l);
  case 2: return mk_format_va(sc, fmt, arg0d);
  case 3: return mk_format_va(sc, fmt, arg0s);
    
  case 5: return mk_format_va(sc, fmt, arg0l, arg1l);
  case 6: return mk_format_va(sc, fmt, arg0d, arg1l);
  case 7: return mk_format_va(sc, fmt, arg0s, arg1l);
    
  case 9: return mk_format_va(sc, fmt, arg0l, arg1d);
  case 10: return mk_format_va(sc, fmt, arg0d, arg1d);
  case 11: return mk_format_va(sc, fmt, arg0s, arg1d);

  case 13: return mk_format_va(sc, fmt, arg0l, arg1s);
  case 14: return mk_format_va(sc, fmt, arg0d, arg1s);
  case 15: return mk_format_va(sc, fmt, arg0s, arg1s);    
  }
  return sc->EMPTY;
}

/* ========== Routines for Evaluation Cycle ========== */

/* make closure. c is code. e is environment */
static inline nanoclj_val_t mk_closure(nanoclj_t * sc, nanoclj_val_t c, nanoclj_val_t e) {
  return mk_pointer(get_cell(sc, T_CLOSURE, c, e, mk_nil()));
}

/* reverse list --- in-place */
static inline nanoclj_val_t reverse_in_place(nanoclj_t * sc, nanoclj_val_t term, nanoclj_val_t list) {
  nanoclj_val_t p = list, result = term, q;

  while (p.as_long != sc->EMPTY.as_long) {
    q = cdr(p);
    cdr(p) = result;
    result = p;
    p = q;
  }
  return (result);
}

/* ========== Evaluation Cycle ========== */

static nanoclj_val_t get_err_port(nanoclj_t * sc) {
  nanoclj_val_t x = find_slot_in_env(sc, sc->envir, sc->ERR, 1);
  if (x.as_long != sc->EMPTY.as_long) {
    return slot_value_in_env(x);
  }
  return mk_nil();
}

static nanoclj_val_t get_out_port(nanoclj_t * sc) {
  nanoclj_val_t x = find_slot_in_env(sc, sc->envir, sc->OUT, 1);
  if (x.as_long != sc->EMPTY.as_long) {
    return slot_value_in_env(x);
  }
  return mk_nil();
}

static nanoclj_val_t get_in_port(nanoclj_t * sc) {
  nanoclj_val_t x = find_slot_in_env(sc, sc->envir, sc->IN, 1);
  if (x.as_long != sc->EMPTY.as_long) {
    return slot_value_in_env(x);
  }
  return mk_nil();
}

static inline nanoclj_val_t _Error_1(nanoclj_t * sc, const char *s, size_t len) {
  const char *str = s;

  char sbuf[AUXBUFF_SIZE];

  /* make sure error is not in REPL */
  if (sc->load_stack[sc->file_i].kind & port_file &&
      sc->load_stack[sc->file_i].rep.stdio.file != stdin) {
    char * tmp = (char *)sc->malloc(len + 1);
    memcpy(tmp, str, len);
    tmp[len] = 0;
      
    int ln = sc->load_stack[sc->file_i].rep.stdio.curr_line;
    const char *fname = sc->load_stack[sc->file_i].rep.stdio.filename;

    /* should never happen */
    if (!fname)
      fname = "<unknown>";

    /* we started from 0 */
    ln++;
    len = snprintf(sbuf, AUXBUFF_SIZE, "(%s : %i) %s", fname, ln, tmp);
    sc->free(tmp);

    str = (const char *) sbuf;
  }

  nanoclj_val_t error_str = mk_counted_string(sc, str, len);
#ifdef VECTOR_ARGS
  sc->args = conj(sc, sc->EMPTYVEC, error_str);
#else
  sc->args = cons(sc, error_str, sc->EMPTY);
#endif
  sc->op = OP_ERR0;
  return (nanoclj_val_t)kTRUE;
}

#define Error_0(sc,s)    return _Error_1(sc, s, strlen(s))

/* Too small to turn into function */
#define  BEGIN     do {
#define  END  } while (0)
#define s_goto(sc,a) BEGIN                                  \
    sc->op = (a);                                           \
    return (nanoclj_val_t)kTRUE; END

#define s_return(sc,a) return _s_return(sc,a)

static inline dump_stack_frame_t * s_add_frame(nanoclj_t * sc) {
  int nframes = decode_integer(sc->dump);
  /* enough room for the next frame? */
  if (nframes >= sc->dump_size) {
    sc->dump_size *= 2;
    fprintf(stderr, "reallocing stack (%d)\n", sc->dump_size);
    sc->dump_base = (dump_stack_frame_t *)sc->realloc(sc->dump_base, sizeof(dump_stack_frame_t) * sc->dump_size);
  }
  sc->dump = mk_int(nframes + 1);
  return sc->dump_base + nframes;
}
  
static inline void s_save(nanoclj_t * sc, enum nanoclj_opcodes op, nanoclj_val_t args,
			  nanoclj_val_t code) {
  dump_stack_frame_t * next_frame = s_add_frame(sc);
  next_frame->op = op;
  next_frame->args = args;
  next_frame->envir = sc->envir;
  next_frame->code = code;
  next_frame->is_c_call_return = false;
#ifdef USE_RECUR_REGISTER
  next_frame->recur = sc->recur;
#endif
}

static inline nanoclj_val_t _s_return(nanoclj_t * sc, nanoclj_val_t a) { 
  int nframes = decode_integer(sc->dump);

  sc->value = a;
  if (nframes <= 0) {
    return sc->EMPTY;
  }
  nframes--;
  dump_stack_frame_t * frame = sc->dump_base + nframes;
  sc->dump = mk_int(nframes);
  if (frame->is_c_call_return) {
    return sc->EMPTY;
  }
  sc->op = frame->op;
  sc->args = frame->args;
  sc->envir = frame->envir;
  sc->code = frame->code;
#ifdef USE_RECUR_REGISTER
  sc->recur = frame->recur;
#endif

  return (nanoclj_val_t)kTRUE;
}

static inline void s_set_return_envir(nanoclj_t * sc, nanoclj_val_t env) {
  int nframes = decode_integer(sc->dump);
  if (nframes >= 1) {
    dump_stack_frame_t * frame = sc->dump_base + nframes - 1;
    frame->envir = env;
  }  
}

static inline void dump_stack_reset(nanoclj_t * sc) {
  /* in this implementation, sc->dump is the number of frames on the stack */
  sc->dump = mk_int(0);
}

static inline void dump_stack_initialize(nanoclj_t * sc) {
  sc->dump_size = 256;
  sc->dump_base = (dump_stack_frame_t *)sc->malloc(sizeof(dump_stack_frame_t) * sc->dump_size);
  dump_stack_reset(sc);
}

static inline void dump_stack_free(nanoclj_t * sc) {
  free(sc->dump_base);
  sc->dump_base = NULL;
  sc->dump = mk_int(0);
  sc->dump_size = 0;
}

static inline void dump_stack_mark(nanoclj_t * sc) {
  int nframes = decode_integer(sc->dump);
  for (int i = 0; i < nframes; i++) {
    dump_stack_frame_t *frame = sc->dump_base + i;
    mark(frame->args);
    mark(frame->envir);
    mark(frame->code);
  }
}

#define s_retbool(tf)    s_return(sc, (tf) ? (nanoclj_val_t)kTRUE : (nanoclj_val_t)kFALSE)

static inline bool is_list(nanoclj_val_t a) {
  return is_cell(a) && _type(decode_pointer(a)) == T_LIST;
}

static inline bool destructure(nanoclj_t * sc, nanoclj_cell_t * binding, nanoclj_cell_t * y, size_t num_args, bool first_level) {
  size_t n = _size_unchecked(binding);
  bool is_multiarity = n >= 2 && vector_elem(binding, n - 2).as_long == sc->AMP.as_long;

  if (first_level) {
    if (!is_multiarity && num_args != n) {
      return false;
    } else if (is_multiarity && num_args < n - 2) {
      return false;
    }
  }

  for (size_t i = 0; i < n; i++, y = next(sc, y)) {
    nanoclj_val_t e = vector_elem(binding, i);
    if (e.as_long == sc->AMP.as_long) {
      if (++i < n) {
	e = vector_elem(binding, i);
	if (is_primitive(e)) {
	  new_slot_in_env(sc, vector_elem(binding, i), mk_pointer(y != &sc->_EMPTY ? y : NULL));
	} else {
	  nanoclj_cell_t * y2 = next(sc, y);
	  if (!destructure(sc, decode_pointer(e), y2, 0, false)) {
	    return false;
	  }
	}
      }
      y = &(sc->_EMPTY);
      break;
    } else if (!first_level || y != &(sc->_EMPTY)) {
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
  if (!is_empty(sc, y)) {
    return false;
  }
  return true;
}

static inline nanoclj_val_t construct_by_type(nanoclj_t * sc, int type_id, nanoclj_val_t args0) {
  nanoclj_cell_t * args = decode_pointer(args0);
  nanoclj_val_t x, y;
  
  switch (type_id) {
  case T_NIL:
    return sc->EMPTY;
    
  case T_BOOLEAN:
    return is_true(first(sc, args)) ? (nanoclj_val_t)kTRUE : (nanoclj_val_t)kFALSE;

  case T_STRING:{
    strview_t sv = to_strview(first(sc, args));
    return mk_counted_string(sc, sv.ptr, sv.size);
  }

  case T_CHAR_ARRAY:{
    strview_t sv = to_strview(first(sc, args));
    nanoclj_val_t a = mk_counted_string(sc, sv.ptr, sv.size);
    typeflag(a) = T_CHAR_ARRAY | T_GC_ATOM | (typeflag(a) & T_SMALL);
    return a;
  }
  case T_VECTOR:
  case T_ARRAYMAP:
  case T_SORTED_SET:
    return mk_collection(sc, type_id, args);

  case T_CLOSURE:
    x = first(sc, args);
    if (car(x).as_long == sc->LAMBDA.as_long) {
      x = cdr(x);
    }
    args = rest(sc, args);
    if (is_empty(sc, args)) {
      y = sc->envir;
    } else {
      y = first(sc, args);
    }
    return mk_closure(sc, x, y);
    
  case T_INTEGER:
    return mk_int(to_int(first(sc, args)));

  case T_LONG:
    /* Casts the argument to long (or int if it is sufficient) */
    return mk_integer(sc, to_long(first(sc, args)));
  
  case T_REAL:
    return mk_real(to_double(first(sc, args)));

  case T_CHARACTER:
    return mk_character(to_int(first(sc, args)));

  case T_TYPE:
    return mk_type(to_int(first(sc, args)));

  case T_ENVIRONMENT:{
    nanoclj_val_t parent = sc->EMPTY;
    nanoclj_val_t name = sc->EMPTY;
    if (!is_empty(sc, args)) {
      parent = first(sc, args);
      args = rest(sc, args);
      if (!is_empty(sc, args)) {
	name = first(sc, args);
      }
    } 
    nanoclj_cell_t * vec = get_vector_object(sc, T_VECTOR, 0, mk_vector_store(sc, OBJ_LIST_SIZE));
    fill_vector(vec, sc->EMPTY);
    return mk_pointer(get_cell(sc, T_ENVIRONMENT, mk_pointer(vec), parent, name));
  }

  case T_PAIR:
  case T_LIST:
    x = first(sc, args);
    y = second(sc, args);
    if (is_cell(y)) {
      nanoclj_cell_t * tail = decode_pointer(y);
      if (!tail) {
	return mk_pointer(get_cell(sc, T_LIST, x, sc->EMPTY, mk_nil()));
      } else {
	int t = _type(tail);
	if (t == T_LAZYSEQ || t == T_SEQ || t == T_LIST) {
	  return mk_pointer(get_cell(sc, T_LIST, x, y, mk_nil()));
	} else {
	  tail = seq(sc, tail);
	  if (tail) {
	    return mk_pointer(get_cell(sc, T_LIST, x, mk_pointer(tail), mk_nil()));
	  }
	}
      }
    }
    return mk_pointer(get_cell(sc, T_PAIR, x, y, mk_nil()));

  case T_MAPENTRY:
    return mk_mapentry(sc, first(sc, args), second(sc, args));
        
  case T_SYMBOL:
    return def_symbol_from_sv(sc, to_strview(first(sc, args)));
    
  case T_KEYWORD:
    return def_keyword_from_sv(sc, to_strview(first(sc, args)));

  case T_SEQ:
    x = first(sc, args);
    if (is_cell(x)) {
      return mk_pointer(seq(sc, decode_pointer(x)));
    } else {
      return mk_nil();
    }

  case T_LAZYSEQ:
    x = mk_closure(sc, cons(sc, sc->EMPTY, first(sc, args)), sc->envir);
    typeflag(x) = T_LAZYSEQ;
    return x;

  case T_DELAY:
    x = mk_closure(sc, cons(sc, sc->EMPTY, first(sc, args)), sc->envir);
    typeflag(x) = T_DELAY;
    return x;

  case T_READER:
    x = first(sc, args);
    if (is_string(x)) {
      return port_from_filename(sc, to_strview(x), port_input);
    } else if (is_char_array(x)) {
      return port_from_string(sc, to_strview(x), port_input);
    }
    break;
    
  case T_WRITER:
    if (is_empty(sc, args)) {
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
    return mk_canvas(sc, to_int(first(sc, args)) * sc->window_scale_factor, to_int(second(sc, args)) * sc->window_scale_factor);
#endif

  case T_IMAGE:
    if (is_empty(sc, args)) {
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

#define INF_ARG 2147483647

static char * dispatch_table[] = {
#define _OP_DEF(A,OP) A,
#include "nanoclj_opdf.h"
  0
};

static inline bool unpack_args_0(nanoclj_t * sc) {
  return sc->args.as_long == sc->EMPTY.as_long;  
}

static inline bool unpack_args_1(nanoclj_t * sc) {
  if (sc->args.as_long != sc->EMPTY.as_long) {
    nanoclj_cell_t * c = decode_pointer(sc->args);
    sc->arg0 = _car(c);
    nanoclj_val_t r = _cdr(c);
    return r.as_long == sc->EMPTY.as_long;      
  }
  return false;
}

static inline bool unpack_args_1_plus(nanoclj_t * sc) {
 if (sc->args.as_long != sc->EMPTY.as_long) {
    nanoclj_cell_t * c = decode_pointer(sc->args);
    sc->arg0 = _car(c);
    sc->arg1 = mk_nil();
    sc->arg_rest = _cdr(c);
    return true;
 }
 return false;
}

static inline bool unpack_args_2(nanoclj_t * sc) {
  if (sc->args.as_long != sc->EMPTY.as_long) {
    nanoclj_cell_t * c = decode_pointer(sc->args);
    sc->arg0 = _car(c);
    nanoclj_val_t r = _cdr(c);
    if (r.as_long != sc->EMPTY.as_long) {
      c = decode_pointer(r);
      sc->arg1 = _car(c);
      nanoclj_val_t r = _cdr(c);
      return r.as_long == sc->EMPTY.as_long;      
    }
  }
  return false;
}

static inline bool unpack_args_3(nanoclj_t * sc) {
  if (sc->args.as_long != sc->EMPTY.as_long) {
    nanoclj_cell_t * c = decode_pointer(sc->args);
    sc->arg0 = _car(c);
    nanoclj_val_t r = _cdr(c);
    if (r.as_long != sc->EMPTY.as_long) {
      c = decode_pointer(r);
      sc->arg1 = _car(c);
      r = _cdr(c);
      if (r.as_long != sc->EMPTY.as_long) {
	c = decode_pointer(r);
	sc->arg2 = _car(c);
	r = _cdr(c);
	return r.as_long == sc->EMPTY.as_long;	
      }
    }
  }
  return false;
}

static inline bool unpack_args_5(nanoclj_t * sc) {
  if (sc->args.as_long != sc->EMPTY.as_long) {
    nanoclj_cell_t * c = decode_pointer(sc->args);
    sc->arg0 = _car(c);
    nanoclj_val_t r = _cdr(c);
    if (r.as_long != sc->EMPTY.as_long) {
      c = decode_pointer(r);
      sc->arg1 = _car(c);
      r = _cdr(c);
      if (r.as_long != sc->EMPTY.as_long) {
	c = decode_pointer(r);
	sc->arg2 = _car(c);
	r = _cdr(c);
	if (r.as_long != sc->EMPTY.as_long) {
	  c = decode_pointer(r);
	  sc->arg3 = _car(c);
	  r = _cdr(c);
	  if (r.as_long != sc->EMPTY.as_long) {
	    c = decode_pointer(r);
	    sc->arg4 = _car(c);
	    r = _cdr(c);
	    return r.as_long == sc->EMPTY.as_long;
	  }
	}
      }
    }
  }
  return false;
}

static inline bool unpack_args_2_plus(nanoclj_t * sc) {
  if (sc->args.as_long != sc->EMPTY.as_long) {
    nanoclj_cell_t * c = decode_pointer(sc->args);
    sc->arg0 = _car(c);
    nanoclj_val_t r = _cdr(c);
    if (r.as_long != sc->EMPTY.as_long) {
      c = decode_pointer(r);
      sc->arg1 = _car(c);
      sc->arg_rest = _cdr(c);
      return true;
    }
  }
  return false;
}

static inline int get_literal_fn_arity(nanoclj_t * sc, nanoclj_val_t body, bool * has_rest) {  
  if (is_pair(body)) {
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

static inline nanoclj_val_t opexe(nanoclj_t * sc, enum nanoclj_opcodes op) {
  nanoclj_val_t x, y, ns, meta;
  int syn;
  nanoclj_val_t params0;
  nanoclj_cell_t * params;

  if (sc->nesting != 0) {
    sc->nesting = 0;
    sc->retcode = -1;
    Error_0(sc, "Error - unmatched parentheses");
  }

  switch (op) {
  case OP_LOAD:                /* load */
    if (!unpack_args_1(sc)) {
      Error_0(sc, "Error - Invalid arity");
    } else {
      strview_t sv = to_strview(sc->arg0);
      if (!file_push(sc, sv)) {
	sprintf(sc->strbuff, "Error - unable to open %.*s", (int)sv.size, sv.ptr);
	Error_0(sc, sc->strbuff);
      } else {
	sc->args = mk_integer(sc, sc->file_i);
	s_goto(sc, OP_T0LVL);
      }
    }

  case OP_T0LVL:               /* top level */
      /* If we reached the end of file, this loop is done. */
    if (port_unchecked(sc->loadport)->kind & port_saw_EOF) {
      if (sc->global_env.as_long != sc->root_env.as_long) {
	fprintf(stderr, "restoring root env\n");
	sc->envir = sc->global_env = sc->root_env;
      }
      
      if (sc->file_i == 0) {
#if 0
        sc->args = sc->EMPTY;
        s_goto(sc, OP_QUIT);
#else
	return sc->EMPTY;
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
    nanoclj_intern(sc, sc->root_env, sc->IN, sc->loadport);
      
    s_save(sc, OP_T0LVL, sc->EMPTY, sc->EMPTY);
    s_save(sc, OP_T1LVL, sc->EMPTY, sc->EMPTY);

    sc->args = sc->EMPTY;
    s_goto(sc, OP_READ);

  case OP_T1LVL:               /* top level */
    sc->code = sc->value;
    nanoclj_intern(sc, sc->root_env, sc->IN, sc->save_inport);
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
    
  case OP_GENSYM:
    if (sc->args.as_long != sc->EMPTY.as_long) {
      strview_t sv = to_strview(car(sc->args));
      s_return(sc, gensym(sc, sv.ptr, sv.size));
    } else {
      s_return(sc, gensym(sc, "G__", 3));
    }
    
  case OP_EVAL:                /* main part of evaluation */
#if USE_TRACING
    if (sc->tracing) {
      /*s_save(sc,OP_VALUEPRINT,sc->EMPTY,sc->EMPTY); */
      s_save(sc, OP_REAL_EVAL, sc->args, sc->code);
      sc->args = sc->code;
      putstr(sc, "\nEval: ", get_err_port(sc));
      s_goto(sc, OP_P0LIST);
    }
    /* fall through */
  case OP_REAL_EVAL:
#endif
    switch (prim_type(sc->code)) {
    case T_SYMBOL:
      if (sc->code.as_long == sc->NS.as_long) { /* special symbols */
	s_return(sc, sc->envir);
#ifdef USE_RECUR_REGISTER
      } else if (sc->code.as_long == sc->RECUR.as_long) {
	s_return(sc, sc->recur);
#endif
      } else {
	x = find_slot_in_env(sc, sc->envir, sc->code, 1);
	if (x.as_long != sc->EMPTY.as_long) {
	  s_return(sc, slot_value_in_env(x));
	} else {
	  snprintf(sc->strbuff, sc->strbuff_size, "Use of undeclared Var %s", symname(sc->code));
	  Error_0(sc, sc->strbuff);
	}
      }

    case T_CELL:{
      nanoclj_cell_t * code_cell = decode_pointer(sc->code);
      switch (_type(code_cell)) {
      case T_PAIR:
      case T_LIST:
	if ((syn = syntaxnum(_car(code_cell))) != 0) {       /* SYNTAX */
	  sc->code = _cdr(code_cell);
	  s_goto(sc, syn);
	} else {                  /* first, eval top element and eval arguments */
	  /* fprintf(stderr, "calling %s\n", is_symbol(car(sc->code)) ? symname(car(sc->code)) : "n/a"); */
	  s_save(sc, OP_E0ARGS, sc->EMPTY, sc->code);
	  /* If no macros => s_save(sc,OP_E1ARGS, sc->EMPTY, cdr(sc->code)); */
	  sc->code = _car(code_cell);
	  s_goto(sc, OP_EVAL);
	}
	
      case T_VECTOR:{
	if (_size_unchecked(code_cell) > 0) {
	  s_save(sc, OP_E0VEC, mk_int(0), sc->code);
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
    int i = decode_integer(sc->args);
    set_vector_elem(vec, i, sc->value);
    if (i + 1 < _size_unchecked(vec)) {
      s_save(sc, OP_E0VEC, mk_int(i + 1), sc->code);
      sc->code = vector_elem(vec, i + 1);
      s_goto(sc, OP_EVAL);
    } else {
      s_return(sc, sc->code);
    }
  }
    
  case OP_E0ARGS:              /* eval arguments */
    if (is_macro(sc->value)) {  /* macro expansion */
      s_save(sc, OP_DOMACRO, sc->EMPTY, sc->EMPTY);
      sc->args = cons(sc, sc->code, sc->EMPTY);
      sc->code = sc->value;
      s_goto(sc, OP_APPLY);
    } else {
      sc->code = cdr(sc->code);
      s_goto(sc, OP_E1ARGS);
    }

  case OP_E1ARGS:              /* eval arguments */
    sc->args = cons(sc, sc->value, sc->args);
    if (is_pair(sc->code)) {    /* continue */
      s_save(sc, OP_E1ARGS, sc->args, cdr(sc->code));
      sc->code = car(sc->code);
      sc->args = sc->EMPTY;
      s_goto(sc, OP_EVAL);
    } else {                    /* end */
      sc->args = reverse_in_place(sc, sc->EMPTY, sc->args);
      sc->code = car(sc->args);
      sc->args = cdr(sc->args);
      s_goto(sc, OP_APPLY);
    }

#if USE_TRACING
  case OP_TRACING:{
    if (!unpack_args_1(sc)) {
      Error_0(sc, "Error - Invalid arity");
    }

    int tr = sc->tracing;
    sc->tracing = to_int(sc->arg0);
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

    case T_TYPE:
      if (1 || sc->args.as_long != sc->EMPTY.as_long) {
	s_return(sc, construct_by_type(sc, decode_integer(sc->code), sc->args));
      } else {
	Error_0(sc, "Error - Invalid arity");
      }

    case T_KEYWORD:{
      nanoclj_val_t coll = car(sc->args), elem;
      if (is_cell(coll) && get_elem(sc, decode_pointer(coll), sc->code, &elem)) {
	s_return(sc, elem);
      } else if (cdr(sc->args).as_long != sc->EMPTY.as_long) {
	s_return(sc, cadr(sc->args));
      } else {
	s_return(sc, mk_nil());
      }
    }	

    case T_CELL:{      
      nanoclj_cell_t * code_cell = decode_pointer(sc->code);
      switch (_type(code_cell)) {
      case T_FOREIGN_FUNCTION:{
	/* Keep nested calls from GC'ing the arglist */
	retain(sc, sc->args, sc->EMPTY);
	if (_min_arity_unchecked(code_cell) > 0 || _max_arity_unchecked(code_cell) < 0x7fffffff) {
	  int n = seq_length(sc, decode_pointer(sc->args));
	  if (n < _min_arity_unchecked(code_cell) || n > _max_arity_unchecked(code_cell)) {
	    Error_0(sc, "Error - Wrong number of args passed (1)");
	  }
	}
	x = _ff_unchecked(code_cell)(sc, sc->args);
	if (is_exception(x)) {
	  strview_t sv = to_strview(car(x));
	  return _Error_1(sc, sv.ptr, sv.size);
	} else {
	  s_return(sc, x);
	}
      }
      case T_FOREIGN_OBJECT:{
	/* Keep nested calls from GC'ing the arglist */
	retain(sc, sc->args, sc->EMPTY);
	x = sc->object_invoke_callback(sc, _fo_unchecked(code_cell), sc->args);
	s_return(sc, x);
      }
      case T_ENVIRONMENT:{
	fprintf(stderr, "trying to change env\n");
	sc->global_env = sc->code;
	s_return(sc, (nanoclj_val_t)kTRUE);
      }
      case T_VECTOR:
      case T_ARRAYMAP:
      case T_SORTED_SET:
      case T_IMAGE:{
	if (sc->args.as_long == sc->EMPTY.as_long) {
	  Error_0(sc, "Error - Invalid arity");
	}
	nanoclj_val_t elem;
	if (get_elem(sc, decode_pointer(sc->code), car(sc->args), &elem)) {
	  s_return(sc, elem);
	} else if (cdr(sc->args).as_long != sc->EMPTY.as_long) {
	  s_return(sc, cadr(sc->args));
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
	y = sc->args;
	meta = _metadata_unchecked(code_cell);
	
	if (!is_nil(meta) && is_string(meta)) {
	  /* if the function has a name, bind it */
	  new_slot_in_env(sc, meta, sc->code);
	}
	
#ifndef USE_RECUR_REGISTER
	new_slot_in_env(sc, sc->RECUR, sc->code);
#else
	sc->recur = sc->code;
#endif

	params0 = car(x);
	params = decode_pointer(params0);

	if (_type(params) == T_VECTOR) { /* Clojure style arguments */	  
	  if (!is_cell(y) || !destructure(sc, params, decode_pointer(y), seq_length(sc, decode_pointer(y)), true)) {
	    Error_0(sc, "Error - Wrong number of args passed (2)");	   
	  }
	  sc->code = cdr(x);
	} else if (_type(params) == T_LIST && is_vector(_car(params))) { /* Clojure style multi-arity arguments */
	  nanoclj_cell_t * yy = decode_pointer(y);
	  size_t needed_args = seq_length(sc, yy);
	  bool found_match = false;
	  for ( ; x.as_long != sc->EMPTY.as_long; x = cdr(x)) {
	    nanoclj_val_t vec = caar(x);

	    if (destructure(sc, decode_pointer(vec), yy, needed_args, true)) {
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
	  for ( ; is_pair(x); x = cdr(x), y = cdr(y)) {
	    if (is_nil(y)) {
	      Error_0(sc, "Error - malformed argument");
	    } else if (y.as_long == sc->EMPTY.as_long) {
	      Error_0(sc, "Error - not enough arguments (3)");
	    } else {
	      new_slot_in_env(sc, car(x), car(y));
	    }
	  }
	  
	  if (x.as_long == sc->EMPTY.as_long) {
	    /*--
	     * if (y.as_long != sc->EMPTY.as_long) {
	     *   Error_0(sc,"too many arguments");
	     * }
	     */
	  } else if (is_symbol(x)) {
	    new_slot_in_env(sc, x, y);
	  } else {
	    Error_0(sc, "Error - syntax error in closure: not a symbol");
	  }
	  sc->code = cdr(closure_code(code_cell));
	}
	sc->args = sc->EMPTY;
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

#if 1
  case OP_LAMBDA:              /* lambda */
    /* If the hook is defined, apply it to sc->code, otherwise
       set sc->value fall thru */
    {
      nanoclj_val_t f = find_slot_in_env(sc, sc->envir, sc->COMPILE_HOOK, 1);
      if (f.as_long == sc->EMPTY.as_long) {
        sc->value = sc->code;
        /* Fallthru */
      } else {
        s_save(sc, OP_LAMBDA1, sc->args, sc->code);
        sc->args = cons(sc, sc->code, sc->EMPTY);
        sc->code = slot_value_in_env(f);
        s_goto(sc, OP_APPLY);
      }
    }

  case OP_LAMBDA1:{
    x = sc->value;
    bool has_name = false;
    
    if (is_symbol(car(x)) && is_vector(cadr(x))) {
      has_name = true;
    } else if (is_symbol(car(x)) && (is_pair(cadr(x)) && is_vector(caadr(x)))) {
      has_name = true;
    }
    
    nanoclj_val_t name;
    if (has_name) {
      name = car(x);
      x = cdr(x);
    } else {
      name = mk_nil();
    }
    
    x = mk_closure(sc, x, sc->envir);

    if (has_name) {
      metadata_unchecked(x) = name;
    }
    s_return(sc, x);
  }
   
#else
  case OP_LAMBDA:              /* lambda */
    s_return(sc, mk_closure(sc, sc->code, sc->envir));

#endif

  case OP_QUOTE:               /* quote */
    s_return(sc, car(sc->code));

  case OP_VAR:                 /* var */
    s_return(sc, find_slot_in_env(sc, sc->envir, car(sc->code), 1));

  case OP_INTERN:
    if (!unpack_args_2_plus(sc)) {
      Error_0(sc, "Error - Invalid arity");
    }

    ns = sc->arg0; /* namespace */
    x = sc->arg1; /* name */
    y = find_slot_in_env(sc, ns, x, 0);

    /* If symbol does not exist or new value is given */
    if (y.as_long == sc->EMPTY.as_long || sc->arg_rest.as_long != sc->EMPTY.as_long) {
      nanoclj_val_t new_value = sc->arg_rest.as_long != sc->EMPTY.as_long ? car(sc->arg_rest) : sc->EMPTY;
      if (y.as_long != sc->EMPTY.as_long) {
	set_slot_in_env(sc, y, new_value);
      } else {
	new_slot_spec_in_env(sc, ns, x, new_value);
      }
    }
    s_return(sc, x);  
    
  case OP_DEF0:                /* define */    
    x = car(sc->code);
    if (caddr(sc->code).as_long != sc->EMPTY.as_long) {
      nanoclj_cell_t * meta = mk_arraymap(sc, 1);
      set_vector_elem(meta, 0, mk_mapentry(sc, sc->DOC, cadr(sc->code)));
      y = mk_pointer(meta);
      sc->code = caddr(sc->code);
    } else {
      y = mk_nil();
      sc->code = cadr(sc->code);
    }

    if (!is_symbol(x)) {
      Error_0(sc, "Error - variable is not a symbol");
    }
    s_save(sc, OP_DEF1, y, x);
    s_goto(sc, OP_EVAL);
    
  case OP_DEF1:                /* define */
    if (!is_nil(sc->args) && is_cell(sc->value)) {
      /* TODO: should value be copied before settings the meta */
      metadata_unchecked(sc->value) = sc->args;
    }
    x = find_slot_in_env(sc, sc->global_env, sc->code, 0);
    if (x.as_long != sc->EMPTY.as_long) {
#if 0
      fprintf(stderr, "%s already refers to a value\n", symname(sc->code));
#endif
      set_slot_in_env(sc, x, sc->value);
    } else {
      new_slot_spec_in_env(sc, sc->global_env, sc->code, sc->value);
    }
    s_return(sc, sc->code);

  case OP_RESOLVE:                /* resolve */
    if (!unpack_args_1_plus(sc)) {
      Error_0(sc, "Error - Invalid arity");
    }

    if (sc->arg_rest.as_long != sc->EMPTY.as_long) {
      ns = sc->arg0;
      x = car(sc->arg_rest);
    } else {
      ns = sc->envir;
      x = sc->arg0;
    }
    x = find_slot_in_env(sc, ns, x, 1);
    s_return(sc, x.as_long != sc->EMPTY.as_long ? x : mk_nil());

  case OP_SET:
    if (!unpack_args_2(sc)) {
      Error_0(sc, "Error - Invalid arity");
    }

    y = find_slot_in_env(sc, sc->envir, sc->arg0, 1);
    if (y.as_long != sc->EMPTY.as_long) {
      s_return(sc, set_slot_in_env(sc, y, sc->arg1));      
    } else {
      Error_0(sc, "Use of undeclared var");
    }
    
  case OP_DO:               /* do */
    if (!is_pair(sc->code)) {
      s_return(sc, sc->code);
    }
    if (cdr(sc->code).as_long != sc->EMPTY.as_long) {
      s_save(sc, OP_DO, sc->EMPTY, cdr(sc->code));
    }
    sc->code = car(sc->code);
    s_goto(sc, OP_EVAL);

  case OP_IF0:                 /* if */
    s_save(sc, OP_IF1, sc->EMPTY, cdr(sc->code));
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
    nanoclj_val_t body = cdr(sc->code);

    size_t n = _size_unchecked(input_vec) / 2;

    nanoclj_val_t values = sc->EMPTY;
    nanoclj_cell_t * args = get_vector_object(sc, T_VECTOR, 0, n > 0 ? mk_vector_store(sc, n) : NULL);

    for (int i = (int)n - 1; i >= 0; i--) {
      set_vector_elem(args, i, vector_elem(input_vec, 2 * i + 0));
      values = cons(sc, vector_elem(input_vec, 2 * i + 1), values);
    }

    body = cons(sc, mk_pointer(args), body);
    body = cons(sc, sc->LAMBDA, body);
    
    sc->code = cons(sc, body, values);
    sc->args = sc->EMPTY;
    s_goto(sc, OP_EVAL);
  }

  case OP_LET0:                /* let */
    x = car(sc->code);
    
    if (is_vector(x)) {       /* Clojure style */
      nanoclj_cell_t * vec = decode_pointer(x);
      new_frame_in_env(sc, sc->envir);
      s_save(sc, OP_LET1_VEC, mk_int(0), sc->code);
      
      sc->code = vector_elem(vec, 1);
      sc->args = sc->EMPTY;
      s_goto(sc, OP_EVAL);
    } else {
      sc->args = sc->EMPTY;
      sc->value = sc->code;
      sc->code = is_symbol(x) ? cadr(sc->code) : x;

      s_goto(sc, OP_LET1);
    }

  case OP_LET1_VEC:{   
    nanoclj_cell_t * vec = decode_pointer(car(sc->code));
    int i = decode_integer(sc->args);
    new_slot_in_env(sc, vector_elem(vec, i), sc->value);
    
    if (i + 2 < _size_unchecked(vec)) {
      s_save(sc, OP_LET1_VEC, mk_int(i + 2), sc->code);
      
      sc->code = vector_elem(vec, i + 3);
      sc->args = sc->EMPTY;
      s_goto(sc, OP_EVAL);      
    } else {
      sc->code = cdr(sc->code);
      sc->args = sc->EMPTY;
      s_goto(sc, OP_DO);
    }
  }
    
  case OP_LET1:                /* let (calculate parameters) */
    sc->args = cons(sc, sc->value, sc->args);
    
    if (is_pair(sc->code)) {    /* continue */
      if (!is_pair(car(sc->code)) || !is_pair(cdar(sc->code))) {
	Error_0(sc, "Error - Bad syntax of binding spec in let");
      }
      s_save(sc, OP_LET1, sc->args, cdr(sc->code));
      sc->code = cadar(sc->code);
      sc->args = sc->EMPTY;
      s_goto(sc, OP_EVAL);
    } else {                    /* end */
      sc->args = reverse_in_place(sc, sc->EMPTY, sc->args);
      sc->code = car(sc->args);
      sc->args = cdr(sc->args);
      s_goto(sc, OP_LET2);      
    }

  case OP_LET2:                /* let */
    new_frame_in_env(sc, sc->envir);
    for (x =
        is_symbol(car(sc->code)) ? cadr(sc->code) : car(sc->code), y =
	   sc->args; y.as_long != sc->EMPTY.as_long; x = cdr(x), y = cdr(y)) {
      new_slot_in_env(sc, caar(x), car(y));
    }
    if (is_symbol(car(sc->code))) {     /* named let */
      for (x = cadr(sc->code), sc->args = sc->EMPTY; x.as_long != sc->EMPTY.as_long; x = cdr(x)) {
        if (!is_pair(x))
          Error_0(sc, "Error - Bad syntax of binding in let");
        if (!is_list(car(x)))
          Error_0(sc, "Error - Bad syntax of binding in let");
        sc->args = cons(sc, caar(x), sc->args);
      }
      x = mk_closure(sc, cons(sc, reverse_in_place(sc, sc->EMPTY, sc->args),
			      cddr(sc->code)), sc->envir);
      new_slot_in_env(sc, car(sc->code), x);
      sc->code = cddr(sc->code);
      sc->args = sc->EMPTY;
    } else {
      sc->code = cdr(sc->code);
      sc->args = sc->EMPTY;
    }
    s_goto(sc, OP_DO);

  case OP_COND0:               /* cond */
    if (sc->code.as_long == sc->EMPTY.as_long) {
      s_return(sc, sc->EMPTY);
    }
    if (!is_pair(sc->code)) {
      Error_0(sc, "Error - syntax error in cond");
    }
    s_save(sc, OP_COND1, sc->EMPTY, sc->code);
    sc->code = car(sc->code);
    s_goto(sc, OP_EVAL);

  case OP_COND1:               /* cond */
    if (is_true(sc->value)) {
      if ((sc->code = cdr(sc->code)).as_long == sc->EMPTY.as_long) {
        s_return(sc, sc->value);
      }
      if (is_nil(sc->code)) {
        if (!is_pair(cdr(sc->code))) {
          Error_0(sc, "Error - syntax error in cond");
        }
        x = cons(sc, sc->QUOTE, cons(sc, sc->value, sc->EMPTY));
        sc->code = cons(sc, cadr(sc->code), cons(sc, x, sc->EMPTY));
        s_goto(sc, OP_EVAL);
      }
      sc->code = cons(sc, car(sc->code), sc->EMPTY);
      s_goto(sc, OP_DO);
    } else {
      if ((sc->code = cddr(sc->code)).as_long == sc->EMPTY.as_long) {
        s_return(sc, sc->EMPTY);
      } else {
        s_save(sc, OP_COND1, sc->EMPTY, sc->code);
        sc->code = car(sc->code);
        s_goto(sc, OP_EVAL);
      }
    }
    
  case OP_LAZYSEQ:               /* lazy-seq */
    x = mk_closure(sc, cons(sc, sc->EMPTY, sc->code), sc->envir);
    typeflag(x) = T_LAZYSEQ;
    s_return(sc, x);    

  case OP_AND0:                /* and */
    if (sc->code.as_long == sc->EMPTY.as_long) {
      s_return(sc, (nanoclj_val_t)kTRUE);
    }
    s_save(sc, OP_AND1, sc->EMPTY, cdr(sc->code));
    sc->code = car(sc->code);
    s_goto(sc, OP_EVAL);

  case OP_AND1:                /* and */
    if (is_false(sc->value)) {
      s_return(sc, sc->value);
    } else if (sc->code.as_long == sc->EMPTY.as_long) {
      s_return(sc, sc->value);
    } else {
      s_save(sc, OP_AND1, sc->EMPTY, cdr(sc->code));
      sc->code = car(sc->code);
      s_goto(sc, OP_EVAL);
    }

  case OP_OR0:                 /* or */
    if (sc->code.as_long == sc->EMPTY.as_long) {
      s_return(sc, (nanoclj_val_t)kFALSE);
    }
    s_save(sc, OP_OR1, sc->EMPTY, cdr(sc->code));
    sc->code = car(sc->code);
    s_goto(sc, OP_EVAL);

  case OP_OR1:                 /* or */
    if (is_true(sc->value)) {
      s_return(sc, sc->value);
    } else if (sc->code.as_long == sc->EMPTY.as_long) {
      s_return(sc, sc->value);
    } else {
      s_save(sc, OP_OR1, sc->EMPTY, cdr(sc->code));
      sc->code = car(sc->code);
      s_goto(sc, OP_EVAL);
    }

  case OP_MACRO0:              /* macro */
    if (is_pair(car(sc->code))) {
      x = caar(sc->code);
      sc->code = cons(sc, sc->LAMBDA, cons(sc, cdar(sc->code), cdr(sc->code)));
    } else {
      x = car(sc->code);
      sc->code = cadr(sc->code);
    }
    if (!is_symbol(x)) {
      Error_0(sc, "Error - variable is not a symbol");
    }
    s_save(sc, OP_MACRO1, sc->EMPTY, x);
    s_goto(sc, OP_EVAL);

  case OP_MACRO1:              /* macro */
    typeflag(sc->value) = T_MACRO;
    x = find_slot_in_env(sc, sc->envir, sc->code, 0);
    if (x.as_long != sc->EMPTY.as_long) {
      set_slot_in_env(sc, x, sc->value);
    } else {
      new_slot_in_env(sc, sc->code, sc->value);
    }
    s_return(sc, sc->code);

  case OP_TRY:
    if (!is_pair(sc->code)) {
      s_return(sc, sc->code);
    }
    x = car(sc->code);
    x = nanoclj_eval(sc, x);
    s_return(sc, x);
      
  case OP_PAPPLY:              /* apply* */
    if (!unpack_args_2(sc)) {
      Error_0(sc, "Error - Invalid arity");
    }

    sc->code = sc->arg0;
    sc->args = sc->arg1;
    s_goto(sc, OP_APPLY);

  case OP_PEVAL:               /* eval */
    if (!unpack_args_1_plus(sc)) {
      Error_0(sc, "Error - Invalid arity");
    }
    if (sc->arg_rest.as_long != sc->EMPTY.as_long) {
      sc->envir = car(sc->arg_rest);
    }
    sc->code = sc->arg0;
    s_goto(sc, OP_EVAL);

  case OP_RATIONALIZE:
    if (!unpack_args_1(sc)) {
      Error_0(sc, "Error - Invalid arity");
    }
    x = sc->arg0;
    if (is_real(x)) {
      if (x.as_double == 0) {
	Error_0(sc, "Divide by zero");
      } else {
	s_return(sc, mk_ratio_from_double(sc, sc->arg0));
      }
    } else {
      s_return(sc, sc->arg0);
    }
    
  case OP_INC:
    if (!unpack_args_1(sc)) {
      Error_0(sc, "Error - Invalid arity");
    }

    x = sc->arg0;
    
    switch (prim_type(x)) {
    case T_INTEGER: s_return(sc, mk_int(decode_integer(x) + 1));
    case T_REAL: s_return(sc, mk_real(rvalue_unchecked(x) + 1.0));
    case T_CELL: {
      nanoclj_cell_t * c = decode_pointer(x);
      switch (_type(c)) {
      case T_LONG: {
	long long v = _lvalue_unchecked(c);
	if (v < LLONG_MAX) {
	  s_return(sc, mk_long(sc, v + 1));
	} else {
	  Error_0(sc, "Error - Integer overflow");
	}
      }
      case T_RATIO:{
	long long num = to_long(_car(c)), den = to_long(_cdr(c)), res;
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
    if (!unpack_args_1(sc)) {
      Error_0(sc, "Error - Invalid arity");
    }

    x = sc->arg0;
    
    switch (prim_type(x)) {
    case T_INTEGER: s_return(sc, mk_int(decode_integer(x) - 1));
    case T_REAL: s_return(sc, mk_real(rvalue_unchecked(x) - 1.0));
    case T_CELL: {
      nanoclj_cell_t * c = decode_pointer(x);
      switch (_type(c)) {
      case T_LONG:{
	long long v = _lvalue_unchecked(c);
	if (v > LLONG_MIN) {
	  s_return(sc, mk_long(sc, v - 1));
	} else {
	  Error_0(sc, "Error - Integer overflow");
	}
      case T_RATIO:{
	long long num = to_long(_car(c)), den = to_long(_cdr(c)), res;
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

  case OP_ADD:{                 /* add */
    if (!unpack_args_2(sc)) {
      Error_0(sc, "Error - Invalid arity");
    }

    x = sc->arg0;
    y = sc->arg1;
    
    int tx = prim_type(x), ty = prim_type(y);
    if (tx == T_REAL || ty == T_REAL) {
      s_return(sc, mk_real(to_double(x) + to_double(y)));
    } else if (tx == T_INTEGER && ty == T_INTEGER) {
      s_return(sc, mk_integer(sc, (long long)decode_integer(x) + (long long)decode_integer(y)));
    } else {
      tx = expand_type(x, tx);
      ty = expand_type(y, ty);
            
      if (tx == T_RATIO || ty == T_RATIO) {
	long long den_x, den_y, den;
	long long num_x = get_ratio(x, &den_x);
	long long num_y = get_ratio(y, &den_y);
	long long num = normalize(num_x * den_y + num_y * den_x,
				  den_x * den_y, &den);
	if (den == 1) {
	  s_return(sc, mk_integer(sc, num));
	} else {
	  s_return(sc, mk_ratio_long(sc, num, den));
	}
      } else {
	long long res;
	if (__builtin_saddll_overflow(to_long(x), to_long(y), &res) == false) {
	  s_return(sc, mk_integer(sc, res));
	} else {
	  Error_0(sc, "Error - Integer overflow");
	}
      }
    }
  }
    
  case OP_SUB:{                 /* minus */
    if (!unpack_args_2(sc)) {
      Error_0(sc, "Error - Invalid arity");
    }

    x = sc->arg0;
    y = sc->arg1;
    
    int tx = prim_type(x), ty = prim_type(y);
    if (tx == T_REAL || ty == T_REAL) {
      s_return(sc, mk_real(to_double(x) - to_double(y)));
    } else if (tx == T_INTEGER && ty == T_INTEGER) {
      s_return(sc, mk_integer(sc, (long long)decode_integer(x) - (long long)decode_integer(y)));
    } else {
      tx = expand_type(x, tx);
      ty = expand_type(y, ty);
            
      if (tx == T_RATIO || ty == T_RATIO) {
	long long den_x, den_y, den;
	long long num_x = get_ratio(x, &den_x);
	long long num_y = get_ratio(y, &den_y);
	long long num = normalize(num_x * den_y - num_y * den_x,
				  den_x * den_y, &den);
	if (den == 1) {
	  s_return(sc, mk_integer(sc, num));
	} else {
	  s_return(sc, mk_ratio_long(sc, num, den));
	}
      } else {
	long long res;
	if (__builtin_ssubll_overflow(to_long(x), to_long(y), &res) == false) {
	  s_return(sc, mk_integer(sc, res));
	} else {
	  Error_0(sc, "Error - Integer overflow");
	}
      }
    }
  }

  case OP_MUL:{                 /* multiply */
    if (!unpack_args_2(sc)) {
      Error_0(sc, "Error - Invalid arity");
    }

    x = sc->arg0;
    y = sc->arg1;
    
    int tx = prim_type(x), ty = prim_type(y);
    if (tx == T_REAL || ty == T_REAL) {
      s_return(sc, mk_real(to_double(x) * to_double(y)));
    } else if (tx == T_INTEGER && ty == T_INTEGER) {
      s_return(sc, mk_integer(sc, (long long)decode_integer(x) * (long long)decode_integer(y)));
    } else {
      tx = expand_type(x, tx);
      ty = expand_type(y, ty);
            
      if (tx == T_RATIO || ty == T_RATIO) {
	long long den_x, den_y;
	long long num_x = get_ratio(x, &den_x);
	long long num_y = get_ratio(y, &den_y);
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
	if (__builtin_smulll_overflow(to_long(x), to_long(y), &res) == false) {
	  s_return(sc, mk_integer(sc, res));
	} else {
	  Error_0(sc, "Error - Integer overflow");
	} 
	s_return(sc, mk_integer(sc, res));
      }
    }
  }
	
  case OP_DIV:{                 /* divide */
    if (!unpack_args_2(sc)) {
      Error_0(sc, "Error - Invalid arity");
    }

    x = sc->arg0;
    y = sc->arg1;       

    int tx = prim_type(x), ty = prim_type(y);
    if (tx == T_REAL || ty == T_REAL) {
      s_return(sc, mk_real(to_double(x) / to_double(y)));
    } else if (tx == T_INTEGER && ty == T_INTEGER) {
      int divisor = decode_integer(y);
      if (divisor == 0) {
	Error_0(sc, "Divide by zero");
      } else {
	int dividend = decode_integer(x);
	if (dividend % divisor == 0) {
	  s_return(sc, mk_integer(sc, dividend / divisor));
	} else {
	  long long den;
	  long long num = normalize(dividend, divisor, &den);
	  s_return(sc, mk_ratio_long(sc, num, den));
	}
      }
    } else {
      tx = expand_type(x, tx);
      ty = expand_type(y, ty);

      if (tx == T_RATIO || ty == T_RATIO) {
	long long den_x, den_y;
	long long num_x = get_ratio(x, &den_x);
	long long num_y = get_ratio(y, &den_y);
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
	long long divisor = to_long(y);
	if (divisor == 0) {
	  Error_0(sc, "Divide by zero");
	} else {
	  long long dividend = to_long(x);
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
    
  case OP_REM: {                 /* rem */
    if (!unpack_args_2(sc)) {
      Error_0(sc, "Error - Invalid arity");
    }
    x = sc->arg0;
    y = sc->arg1;       

    int tx = prim_type(x), ty = prim_type(y);
    if (tx == T_REAL || ty == T_REAL) {
      double a = to_double(x), b = to_double(y);
      if (b == 0) {
	Error_0(sc, "Error - division by zero");
      }
      double res = fmod(a, b);
      /* remainder should have same sign as first operand */
      if (res > 0 && a < 0) res -= fabs(b);
      else if (res < 0 && a > 0) res += fabs(b);

      s_return(sc, mk_real(res));
    } else {
      long long a = to_long(sc->arg0), b = to_long(sc->arg1);
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
    if (!unpack_args_1(sc)) {
      Error_0(sc, "Error - Invalid arity");
    }
    if (!is_cell(sc->arg0)) {
      Error_0(sc, "Error - value is not ISeqable");    
    } else {
      s_return(sc, first(sc, decode_pointer(sc->arg0)));
    }

  case OP_SECOND:
    if (!unpack_args_1(sc)) {
      Error_0(sc, "Error - Invalid arity");
    } else if (!is_cell(sc->arg0)) {
      Error_0(sc, "Error - value is not ISeqable");
    } else {
      s_return(sc, second(sc, decode_pointer(sc->arg0)));
    }

  case OP_REST:                 /* rest */
  case OP_NEXT:
    if (!unpack_args_1(sc)) {
      Error_0(sc, "Error - Invalid arity");
    }
    if (!is_cell(sc->arg0)) {
      Error_0(sc, "Error - value is not ISeqable");
    } else {
      nanoclj_cell_t * x2 = decode_pointer(sc->arg0);
      if (x2 && (_type(x2) == T_MAPENTRY || _type(x2) == T_RATIO || _type(x2) == T_PAIR || _type(x2) == T_VAR)) {
	/* Handle degenerate lists */
	s_return(sc, _cdr(x2));
      } else if (op == OP_NEXT) {
	s_return(sc, mk_pointer(next(sc, x2)));
      } else {
	s_return(sc, mk_pointer(rest(sc, x2)));
      }
    }

  case OP_GET:{             /* get */
    if (!unpack_args_2_plus(sc)) {
      Error_0(sc, "Error - Invalid arity");
    }

    nanoclj_val_t coll = sc->arg0;
    nanoclj_val_t arg = sc->arg1;
    nanoclj_val_t elem;

    if (is_cell(coll) && get_elem(sc, decode_pointer(coll), arg, &elem)) {
      s_return(sc, elem);
    } else if (sc->arg_rest.as_long != sc->EMPTY.as_long) {
      /* Not found => return the not-found value if provided */
      s_return(sc, car(sc->arg_rest));
    } else {
      s_return(sc, mk_nil());
    }
  }

  case OP_CONTAINSP:
    if (!unpack_args_2(sc)) {
      Error_0(sc, "Error - Invalid arity");
    }
    
    if (is_cell(sc->arg0) && get_elem(sc, decode_pointer(sc->arg0), sc->arg1, NULL)) {
      s_return(sc, (nanoclj_val_t)kTRUE);
    } else {
      s_return(sc, (nanoclj_val_t)kFALSE);
    }
    
  case OP_CONJ:             /* conj- */
    if (!unpack_args_2(sc)) {
      Error_0(sc, "Error - Invalid arity");
    }
#if 0
    x = sc->arg0;
    y = sc->arg1;
#else
    x = car(sc->args);
    y = cadr(sc->args);
#endif
    switch (type(x)) {      
    case T_VECTOR:{
      nanoclj_cell_t * vec0 = decode_pointer(x);
      size_t vector_len = _size_unchecked(vec0);

      /* If MapEntry is added to vector, it is an index value pair */
      if (type(y) == T_MAPENTRY) {
	long long index = to_long(car(y));
	nanoclj_val_t value = cdr(y);
	if (index < 0 || index >= vector_len) {
	  Error_0(sc, "Error - Index out of bounds");     
	} else {
	  nanoclj_cell_t * vec = copy_vector(sc, vec0);
	  if (sc->no_memory) {
	    s_return(sc, sc->sink);
	  }
	  set_vector_elem(vec, index, value);
	  s_return(sc, mk_pointer(vec));
	}
      } else {
	nanoclj_cell_t * vec = conj(sc, vec0, y);
	s_return(sc, mk_pointer(vec));
      }
    }
      
    case T_ARRAYMAP:{
      nanoclj_cell_t * vec = decode_pointer(x);
      nanoclj_val_t key = car(y);
      size_t vector_len = _size_unchecked(vec);
      for (size_t i = 0; i < vector_len; i++) {
	if (car(vector_elem(vec, i)).as_long == key.as_long) {
	  vec = copy_vector(sc, vec);
	  set_vector_elem(vec, i, y);
	  s_return(sc, mk_pointer(vec));
	}
      }
      /* Not found */
      vec = conj(sc, vec, y);
      s_return(sc, mk_pointer(vec));
    }
      
    case T_SORTED_SET:{
      nanoclj_cell_t * vec = decode_pointer(x);
      if (!get_elem(sc, vec, y, NULL)) {
	vec = conj(sc, vec, y);
	vec = copy_vector(sc, vec);
	sort_vector_in_place(vec);
      }
      s_return(sc, mk_pointer(vec));
    }
      
    case T_NIL:
      if (is_nil(x)) {
	x = sc->EMPTY;
      }
      /* Empty list */
    case T_PAIR:
    case T_LIST:
      s_return(sc, cons(sc, y, x));
      
    default:
      Error_0(sc, "Error - No protocol method ICollection.-conj defined");
    }

  case OP_SUBVEC:
    if (!unpack_args_2_plus(sc)) {
     Error_0(sc, "Error - Invalid arity");
    }
    if (!is_cell(sc->arg0)) {
      Error_0(sc, "Error - Not a vector");
    } else {
      nanoclj_cell_t * c = decode_pointer(sc->arg0);
      long long start = to_long(sc->arg1);
      long long end;
      if (sc->arg_rest.as_long != sc->EMPTY.as_long) {
	end = to_long(car(sc->arg_rest));
      } else {
	end = _size_unchecked(c);
      }
      if (end < start) end = start;
      s_return(sc, mk_pointer(subvec(sc, c, start, end - start)));
    }
  case OP_NOT:                 /* not */
    if (!unpack_args_1(sc)) {
      Error_0(sc, "Error - Invalid arity");
    }
    s_retbool(is_false(sc->arg0));
    
  case OP_EQUIV:                  /* equiv */
    if (!unpack_args_2(sc)) {
      Error_0(sc, "Error - Invalid arity");
    }
    s_retbool(equiv(sc->arg0, sc->arg1));
    
  case OP_LT:                  /* lt */
    if (!unpack_args_2(sc)) {
      Error_0(sc, "Error - Invalid arity");
    }
    s_retbool(compare(sc->arg0, sc->arg1) < 0);

  case OP_GT:                  /* gt */
    if (!unpack_args_2(sc)) {
      Error_0(sc, "Error - Invalid arity");
    }
    s_retbool(compare(sc->arg0, sc->arg1) > 0);
  
  case OP_LE:                  /* le */
    if (!unpack_args_2(sc)) {
      Error_0(sc, "Error - Invalid arity");
    }
    s_retbool(compare(sc->arg0, sc->arg1) <= 0);

  case OP_GE:                  /* ge */
    if (!unpack_args_2(sc)) {
      Error_0(sc, "Error - Invalid arity");
    }
    s_retbool(compare(sc->arg0, sc->arg1) >= 0);
    
  case OP_EQ:                  /* equals? */
    if (!unpack_args_2(sc)) {
      Error_0(sc, "Error - Invalid arity");
    }
    s_retbool(equals(sc, sc->arg0, sc->arg1));

  case OP_BIT_AND:
    if (!unpack_args_2(sc)) {
      Error_0(sc, "Error - Invalid arity");
    }
    s_return(sc, mk_integer(sc, to_long(sc->arg0) & to_long(sc->arg1)));

  case OP_BIT_OR:
    if (!unpack_args_2(sc)) {
      Error_0(sc, "Error - Invalid arity");
    }
    s_return(sc, mk_integer(sc, to_long(sc->arg0) | to_long(sc->arg1)));

  case OP_BIT_XOR:
    if (!unpack_args_2(sc)) {
      Error_0(sc, "Error - Invalid arity");
    }
    s_return(sc, mk_integer(sc, to_long(sc->arg0) ^ to_long(sc->arg1)));

  case OP_BIT_SHIFT_LEFT:
    if (!unpack_args_2(sc)) {
      Error_0(sc, "Error - Invalid arity");
    }
    s_return(sc, mk_integer(sc, to_long(sc->arg0) << to_long(sc->arg1)));

  case OP_BIT_SHIFT_RIGHT:
    if (!unpack_args_2(sc)) {
      Error_0(sc, "Error - Invalid arity");
    }
    s_return(sc, mk_integer(sc, to_long(sc->arg0) >> to_long(sc->arg1)));

  case OP_TYPE:                /* type */
    if (!unpack_args_1(sc)) {
      Error_0(sc, "Error - Invalid arity");
    }
    x = sc->arg0;
    if (is_nil(x)) {
      s_return(sc, mk_nil());
    } else {
      s_return(sc, mk_type(type(x)));
    }

  case OP_IDENTICALP:                  /* identical? */
    if (!unpack_args_2(sc)) {
      Error_0(sc, "Error - Invalid arity");
    }
    s_retbool(sc->arg0.as_long == sc->arg1.as_long);

  case OP_DEREF:
    x = car(sc->args);
    if (!is_cell(x)) {
      _s_return(sc, x);
    } else {
      sc->code = x;
      if (type(x) == T_LAZYSEQ || type(x) == T_DELAY) {
	if (!_is_realized(decode_pointer(x))) {
	  /* Should change type to closure here */
	  s_save(sc, OP_SAVE_FORCED, sc->EMPTY, sc->code);
	  sc->args = sc->EMPTY;
	  s_goto(sc, OP_APPLY);
	} else if (type(x) == T_LAZYSEQ && is_nil(car(sc->code)) && is_nil(cdr(sc->code))) {
	  s_return(sc, sc->EMPTY);
	}
      }
      if (type(x) == T_DELAY) {
	s_return(sc, cdr(sc->code));
      } else {
	s_return(sc, sc->code);
      }
    }

  case OP_SAVE_FORCED:         /* Save forced value replacing delay */
    if (!is_cell(sc->code)) {
      s_return(sc, sc->code);
    }
    _set_realized(decode_pointer(sc->code));
    if (!is_cell(sc->value)) {
      car(sc->code) = mk_nil();
      cdr(sc->code) = sc->value;
      s_return(sc, sc->value);
    } else if (sc->value.as_long == sc->EMPTY.as_long) {
      car(sc->code) = cdr(sc->code) = mk_nil();
      s_return(sc, sc->EMPTY);
    } else {
      car(sc->code) = car(sc->value);
      cdr(sc->code) = cdr(sc->value);
      s_return(sc, sc->code);
    }

  case OP_FLUSH:
    {
      nanoclj_val_t p0 = find_slot_in_env(sc, sc->envir, sc->OUT, 1);
      if (p0.as_long != sc->EMPTY.as_long) {
	nanoclj_val_t p = slot_value_in_env(p0);
	if (p.as_long != sc->EMPTY.as_long) {
	  nanoclj_port_t * pt = port_unchecked(p);
	  if (pt->kind & port_file) {
	    fflush(pt->rep.stdio.file);
	  }
	}
      }
      s_return(sc, mk_nil());
    }
    
  case OP_PR:               /* pr- */
  case OP_PRINT:            /* print- */
    if (!unpack_args_1(sc)) {
      Error_0(sc, "Error - Invalid arity");
    }
    print_primitive(sc, sc->arg0, op == OP_PR, get_out_port(sc));
    s_return(sc, mk_nil());
  
  case OP_FORMAT:{
    if (!unpack_args_1_plus(sc)) {
      Error_0(sc, "Error - Invalid arity");
    }
    const char * fmt = strvalue(sc->arg0);
    s_return(sc, mk_format(sc, fmt, decode_pointer(sc->arg_rest)));
  }

  case OP_ERR0:                /* throw */
    sc->retcode = -1;
    set_basic_style(stderr, 1);
    set_basic_style(stderr, 31);
    {
      nanoclj_val_t err = get_err_port(sc);
      assert(is_writer(err));
      strview_t sv = to_strview(car(sc->args));
      putchars(sc, sv.ptr, sv.size, err);
      putchars(sc, "\n", 1, err);
    }
    reset_color(stderr);
#if 1
    s_goto(sc, OP_T0LVL);
#else
    return sc->EMPTY;
#endif
    
  case OP_VERSION:
    s_return(sc, mk_string(sc, get_version()));

  case OP_CLOSE:        /* close */
    if (!unpack_args_1(sc)) {
      Error_0(sc, "Error - Invalid arity");
    }

    switch (type(sc->arg0)) {
    case T_READER:
    case T_WRITER:
      port_close(sc, port_unchecked(sc->arg0));
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
	  s_save(sc, OP_RDFN, sc->EMPTY, sc->EMPTY);
	}
	sc->tok = token(sc, inport);
	if (sc->tok == TOK_RPAREN) {       /* Empty list */
	  s_return(sc, sc->EMPTY);
	} else if (sc->tok == TOK_DOT) {
	  Error_0(sc, "Error - illegal dot expression");
	} else {
	  sc->nesting_stack[sc->file_i]++;
	  s_save(sc, OP_RDLIST, sc->EMPTY, sc->EMPTY);
	  s_goto(sc, OP_RDSEXPR);
	}
	
      case TOK_VEC:
	s_save(sc, OP_RDVEC, sc->EMPTY, sc->EMPTY);      
	sc->tok = token(sc, inport);
	if (sc->tok == TOK_RSQUARE) {	/* Empty vector */
	  s_return(sc, sc->EMPTY);
	} else if (sc->tok == TOK_DOT) {
	  Error_0(sc, "Error - illegal dot expression");
	} else {
	  sc->nesting_stack[sc->file_i]++;
	  s_save(sc, OP_RDVEC_ELEMENT, sc->EMPTY, sc->EMPTY);
	  s_goto(sc, OP_RDSEXPR);
	}      
	
      case TOK_SET:
      case TOK_MAP:
	if (sc->tok == TOK_SET) {
	  s_save(sc, OP_RDSET, sc->EMPTY, sc->EMPTY);
	} else {
	  s_save(sc, OP_RDMAP, sc->EMPTY, sc->EMPTY);
	}
	sc->tok = token(sc, inport);
	if (sc->tok == TOK_RCURLY) {     /* Empty map or set */
	  s_return(sc, sc->EMPTY);
	} else if (sc->tok == TOK_DOT) {
	  Error_0(sc, "Error - illegal dot expression");
	} else {
	  sc->nesting_stack[sc->file_i]++;
	  s_save(sc, OP_RDMAP_ELEMENT, sc->EMPTY, sc->EMPTY);
	  s_goto(sc, OP_RDSEXPR);
	}
	
      case TOK_QUOTE:
	s_save(sc, OP_RDQUOTE, sc->EMPTY, sc->EMPTY);
	sc->tok = token(sc, inport);
	s_goto(sc, OP_RDSEXPR);
	
      case TOK_DEREF:
	s_save(sc, OP_RDDEREF, sc->EMPTY, sc->EMPTY);
	sc->tok = token(sc, inport);
	s_goto(sc, OP_RDSEXPR);
	
      case TOK_VAR:
	s_save(sc, OP_RDVAR, sc->EMPTY, sc->EMPTY);
	sc->tok = token(sc, inport);
	s_goto(sc, OP_RDSEXPR);      
	
      case TOK_BQUOTE:
	sc->tok = token(sc, inport);
	if (sc->tok == TOK_VEC) {
	  s_save(sc, OP_RDQQUOTEVEC, sc->EMPTY, sc->EMPTY);
	  sc->tok = TOK_LPAREN; // ?
	  s_goto(sc, OP_RDSEXPR);
	} else {
	  s_save(sc, OP_RDQQUOTE, sc->EMPTY, sc->EMPTY);
	}
	s_goto(sc, OP_RDSEXPR);
	
      case TOK_COMMA:
	s_save(sc, OP_RDUNQUOTE, sc->EMPTY, sc->EMPTY);
	sc->tok = token(sc, inport);
	s_goto(sc, OP_RDSEXPR);
      case TOK_ATMARK:
	s_save(sc, OP_RDUQTSP, sc->EMPTY, sc->EMPTY);
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
	s_return(sc, cons(sc, sc->REGEX, cons(sc, x, sc->EMPTY)));
	
      case TOK_CHAR_CONST:
	if ((x = mk_char_const(sc, readstr_upto(sc, DELIMITERS))).as_long == sc->EMPTY.as_long) {
	  Error_0(sc, "Error - undefined character literal");
	} else {
	  s_return(sc, x);
	}
	
      case TOK_TAG:{
	const char * tag = readstr_upto(sc, DELIMITERS);
	nanoclj_val_t f = find_slot_in_env(sc, sc->envir, sc->TAG_HOOK, 1);
	if (f.as_long != sc->EMPTY.as_long) {
	  nanoclj_val_t hook = slot_value_in_env(f);
	  if (!is_nil(hook)) {
	    sc->code = cons(sc, hook, cons(sc, mk_string(sc, tag), sc->EMPTY));
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
	nanoclj_port_t * inport = port_unchecked(get_in_port(sc));
	int c = inchar(sc, inport);
	if (c != '\n' && c != EOF) {
	  backchar(c, inport);
	} else if (sc->load_stack[sc->file_i].kind & port_file) {
	  sc->load_stack[sc->file_i].rep.stdio.curr_line++;
	}
	sc->nesting_stack[sc->file_i]--;
	s_return(sc, reverse_in_place(sc, sc->EMPTY, sc->args));
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
      sc->args = cons(sc, sc->value, sc->args);
      sc->tok = token(sc, inport);
      if (sc->tok == TOK_EOF) {
	s_return(sc, mk_character(EOF));
      } else if (sc->tok == TOK_RSQUARE) {
	nanoclj_port_t * inport = port_unchecked(get_in_port(sc));
	int c = inchar(sc, inport);
	if (c != '\n') {
	  backchar(c, inport);
	} else if (sc->load_stack[sc->file_i].kind & port_file) {
	  sc->load_stack[sc->file_i].rep.stdio.curr_line++;
	}
	sc->nesting_stack[sc->file_i]--;
	s_return(sc, reverse_in_place(sc, sc->EMPTY, sc->args));
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
	nanoclj_port_t * inport = port_unchecked(get_in_port(sc));
	int c = inchar(sc, inport);
	if (c != '\n') {
	  backchar(c, inport);
	} else if (sc->load_stack[sc->file_i].kind & port_file) {
	  sc->load_stack[sc->file_i].rep.stdio.curr_line++;
	}
	sc->nesting_stack[sc->file_i]--;
	s_return(sc, reverse_in_place(sc, sc->EMPTY, sc->args));
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
	s_return(sc, reverse_in_place(sc, sc->value, sc->args));
      }
    }

  case OP_RDQUOTE:
    s_return(sc, cons(sc, sc->QUOTE, cons(sc, sc->value, sc->EMPTY)));

  case OP_RDDEREF:
    s_return(sc, cons(sc, sc->DEREF, cons(sc, sc->value, sc->EMPTY)));

  case OP_RDVAR:
    s_return(sc, cons(sc, sc->VAR, cons(sc, sc->value, sc->EMPTY)));
    
  case OP_RDQQUOTE:
    s_return(sc, cons(sc, sc->QQUOTE, cons(sc, sc->value, sc->EMPTY)));
    
  case OP_RDQQUOTEVEC:
    s_return(sc, cons(sc, def_symbol(sc, "apply"),
		      cons(sc, def_symbol(sc, "vector"),
			   cons(sc, cons(sc, sc->QQUOTE,
					 cons(sc, sc->value, sc->EMPTY)), sc->EMPTY))));
    
  case OP_RDUNQUOTE:
    s_return(sc, cons(sc, sc->UNQUOTE, cons(sc, sc->value, sc->EMPTY)));

  case OP_RDUQTSP:
    s_return(sc, cons(sc, sc->UNQUOTESP, cons(sc, sc->value, sc->EMPTY)));

  case OP_RDVEC:{
    /*sc->code=cons(sc,mk_proc(sc,OP_VECTOR),sc->value);
       s_goto(sc,OP_EVAL); Cannot be quoted */
    /*x=cons(sc,mk_proc(sc,OP_VECTOR),sc->value);
       s_return(sc,x); Cannot be part of pairs */
    /*sc->code=mk_proc(sc,OP_VECTOR);
       sc->args=sc->value;
       s_goto(sc,OP_APPLY); */
    nanoclj_cell_t * v = decode_pointer(sc->value);
    if (is_empty(sc, v)) {
      s_return(sc, sc->EMPTYVEC);
    } else {
      s_return(sc, mk_collection(sc, T_VECTOR, v));
    }
  }

  case OP_RDFN:{
    bool has_rest;
    int n_args = get_literal_fn_arity(sc, sc->value, &has_rest);
    size_t vsize = n_args + (has_rest ? 2 : 0);
    nanoclj_cell_t * vec = get_vector_object(sc, T_VECTOR, 0, vsize > 0 ? mk_vector_store(sc, vsize) : NULL);
    if (n_args >= 1) set_vector_elem(vec, 0, sc->ARG1);
    if (n_args >= 2) set_vector_elem(vec, 1, sc->ARG2);
    if (n_args >= 3) set_vector_elem(vec, 2, sc->ARG3);
    if (has_rest) {
      set_vector_elem(vec, n_args, sc->AMP);
      set_vector_elem(vec, n_args + 1, sc->ARG_REST);
    }
    s_return(sc, cons(sc, sc->LAMBDA, cons(sc, mk_pointer(vec), cons(sc, sc->value, sc->EMPTY))));
  }
    
  case OP_RDSET:
    if (sc->value.as_long == sc->EMPTY.as_long) {
      s_return(sc, mk_pointer(mk_sorted_set(sc, 0)));
    } else {
      s_return(sc, cons(sc, sc->SORTED_SET, sc->value));
    }
    
  case OP_RDMAP:
    if (sc->value.as_long == sc->EMPTY.as_long) {
      s_return(sc, mk_pointer(mk_arraymap(sc, 0)));
    } else {
      s_return(sc, cons(sc, sc->ARRAY_MAP, sc->value));
    }
    
  case OP_EMPTYP:
    if (!unpack_args_1(sc)) {
      Error_0(sc, "Error - Invalid arity");
    }
    if (is_cell(sc->arg0)) {
      s_retbool(is_empty(sc, decode_pointer(sc->arg0)));
    } else {
      Error_0(sc, "Error - value is not ISeqable");
    }
    
  case OP_HASH:
    if (!unpack_args_1(sc)) {
      Error_0(sc, "Error - Invalid arity");
    }
    s_return(sc, mk_int(hasheq(sc->arg0)));

  case OP_COMPARE:
    if (!unpack_args_2(sc)) {
      Error_0(sc, "Error - Invalid arity");
    }
    s_return(sc, mk_int(compare(sc->arg0, sc->arg1)));
	     
  case OP_SORT:{
    if (!unpack_args_1(sc)) {
      Error_0(sc, "Error - Invalid arity");
    }
    x = sc->arg0;
    if (!is_cell(x)) {
      s_return(sc, x);
    }
    nanoclj_cell_t * seq = decode_pointer(x);
    size_t l = seq_length(sc, seq);
    if (l == 0) {
      s_return(sc, sc->EMPTY);
    } else {
      nanoclj_cell_t * vec = get_vector_object(sc, T_VECTOR, 0, mk_vector_store(sc, l));
      for (size_t i = 0; i < l; i++, seq = rest(sc, seq)) {
	set_vector_elem(vec, i, first(sc, seq));
      }
      sort_vector_in_place(vec);
      nanoclj_cell_t * r = 0;
      for (int i = (int)l - 1; i >= 0; i--) {
	r = conj(sc, r, vector_elem(vec, i));
      }
      s_return(sc, mk_pointer(r));
    }
  }

  case OP_UTF8MAP:{
    if (!unpack_args_2(sc)) {
      Error_0(sc, "Error - Invalid arity");
    }
    nanoclj_val_t input = sc->arg0;
    nanoclj_val_t options = sc->arg1;
    utf8proc_uint8_t * dest = NULL;
    strview_t sv = to_strview(input);
    utf8proc_ssize_t s = utf8proc_map((const unsigned char *)sv.ptr,
				      (utf8proc_ssize_t)sv.size,
				      &dest,
				      (utf8proc_option_t)to_long(options)
				      );
    if (s >= 0) {
      nanoclj_val_t r = mk_counted_string(sc, (char *)dest, s);
      free(dest);
      s_return(sc, r);
    } else {
      s_return(sc, mk_nil());
    }
  }

  case OP_META:
    if (!unpack_args_1(sc)) {
      Error_0(sc, "Error - Invalid arity");
    }
    if (is_cell(sc->arg0)) {
      s_return(sc, metadata_unchecked(sc->arg0));
    } else {
      s_return(sc, mk_nil());
    }

  case OP_IN_NS:
    if (!unpack_args_1(sc)) {
      Error_0(sc, "Error - Invalid arity");
    }
    x = sc->arg0;
    y = find_slot_in_env(sc, sc->envir, x, 1);
    nanoclj_val_t ns;
    if (y.as_long != sc->EMPTY.as_long) {
      ns = slot_value_in_env(y);
    } else {
      ns = def_namespace_with_sym(sc, x);
      new_slot_spec_in_env(sc, sc->root_env, x, ns);
    }
    sc->envir = ns;
    s_set_return_envir(sc, ns);
    s_return(sc, ns);

  case OP_RE_PATTERN:
    if (!unpack_args_1(sc)) {
      Error_0(sc, "Error - Invalid arity");
    }
    x = sc->arg0;
    /* TODO: construct a regex */
    s_return(sc, x);

  case OP_ADD_WATCH:
    if (!unpack_args_3(sc)) {
      Error_0(sc, "Error - Invalid arity");
    }
    if (!is_cell(sc->arg0)) {
      Error_0(sc, "Error - Cannot add watch to a primitive");
    } else {
      nanoclj_cell_t * c = decode_pointer(sc->arg0);
      if (_type(c) != T_VAR) {
	Error_0(sc, "Error - Watches can only be added to Vars");
      } else {
	_watches_unchecked(c) = cons(sc, sc->arg2, _watches_unchecked(c));
	s_return(sc, sc->arg0);
      }      
    }
    break;

  case OP_REALIZEDP:
    if (!unpack_args_1(sc)) {
      Error_0(sc, "Error - Invalid arity");
    }
    if (is_cell(sc->arg0)) {
      nanoclj_cell_t * c = decode_pointer(sc->arg0);
      if (_type(c) == T_LAZYSEQ || _type(c) == T_DELAY) {
	s_retbool(_is_realized(c));
      }
    }
    s_retbool(true);
    
  case OP_SET_COLOR:
    if (!unpack_args_1(sc)) {
      Error_0(sc, "Error - Invalid arity");
    }
    x = car(sc->args);
    if (is_vector(x)) {
      set_color(sc, decode_pointer(x), get_out_port(sc));
    }
    s_return(sc, mk_nil());

  case OP_SET_FONT_SIZE:
    if (!unpack_args_1(sc)) {
      Error_0(sc, "Error - Invalid arity");
    }
#if NANOCLJ_HAS_CANVAS
    x = get_out_port(sc);
    if (is_canvas(x)) {
      canvas_set_font_size(canvas_unchecked(x), to_double(sc->arg0) * sc->window_scale_factor);
    }
#endif
    s_return(sc, mk_nil());
    
  case OP_SET_LINE_WIDTH:
    if (!unpack_args_1(sc)) {
      Error_0(sc, "Error - Invalid arity");
    }
#if NANOCLJ_HAS_CANVAS
    x = get_out_port(sc);
    if (is_canvas(x)) {
      /* Do not scale line width whit DPI scale factor */
      canvas_set_line_width(canvas_unchecked(x), to_double(sc->arg0));
    }
#endif
    s_return(sc, mk_nil());

  case OP_MOVETO:
    if (!unpack_args_2(sc)) {
      Error_0(sc, "Error - Invalid arity");
    }
#if NANOCLJ_HAS_CANVAS
    x = get_out_port(sc);
    if (is_canvas(x)) {
      canvas_move_to(canvas_unchecked(x), to_double(sc->arg0) * sc->window_scale_factor, to_double(sc->arg1) * sc->window_scale_factor);
    }
#endif
    s_return(sc, mk_nil());
    
  case OP_LINETO:
    if (!unpack_args_2(sc)) {
      Error_0(sc, "Error - Invalid arity");
    }
#if NANOCLJ_HAS_CANVAS
    x = get_out_port(sc);
    if (is_canvas(x)) {
      canvas_line_to(canvas_unchecked(x), to_double(sc->arg0) * sc->window_scale_factor, to_double(sc->arg1) * sc->window_scale_factor);
    }
#endif
    s_return(sc, mk_nil());

  case OP_ARC:
    if (!unpack_args_5(sc)) {
      Error_0(sc, "Error - Invalid arity");
    }
#if NANOCLJ_HAS_CANVAS
    x = get_out_port(sc);
    if (is_canvas(x)) {
      canvas_arc(canvas_unchecked(x), to_double(sc->arg0) * sc->window_scale_factor, to_double(sc->arg1) * sc->window_scale_factor,
		 to_double(sc->arg2) * sc->window_scale_factor, to_double(sc->arg3), to_double(sc->arg4));
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
    if (!unpack_args_1(sc)) {
      Error_0(sc, "Error - Invalid arity");
    }
#if NANOCLJ_HAS_CANVAS
    x = get_out_port(sc);
    if (is_canvas(x)) {
      double width, height;
      canvas_get_text_extents(canvas_unchecked(x), to_strview(sc->arg0), &width, &height);
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
      } else if (pt->rep.callback.restore) {
	pt->rep.callback.restore(sc->ext_data);
      }
    }
    s_return(sc, mk_nil());

  default:
    sprintf(sc->strbuff, "%d: illegal operator", sc->op);
    Error_0(sc, sc->strbuff);
  }
  return (nanoclj_val_t)kTRUE;                 /* NOTREACHED */
}
 
static inline const char *procname(nanoclj_val_t x) {
  int n = decode_integer(x);
  const char *name = dispatch_table[n];
  if (name == 0) {
    name = "ILLEGAL!";
  }
  return name;
}

/* kernel of this interpreter */
static void Eval_Cycle(nanoclj_t * sc, enum nanoclj_opcodes op) {
  sc->op = op;
  for (;;) {    
    ok_to_freely_gc(sc);
    if (opexe(sc, (enum nanoclj_opcodes) sc->op).as_long == sc->EMPTY.as_long) {
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
  oblist_add_item(sc, name, mk_symbol(sc, name, syntax));
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
  
  nanoclj_intern(sc, sc->global_env, sc->WINDOW_SIZE, size);
  nanoclj_intern(sc, sc->global_env, sc->WINDOW_SCALE_F, mk_real(sc->window_scale_factor));
} 

/* initialization of nanoclj_t */
#if USE_INTERFACE
static const char * checked_strvalue(nanoclj_val_t p) {
  if (type(p) == T_STRING) {
    return strvalue(p);
  } else {
    return "";
  }
}

static nanoclj_val_t checked_car(nanoclj_val_t p) {
  switch (type(p)) {
  case T_NIL:
  case T_PAIR:
  case T_LIST:
    return car(p);
  default:
    return mk_nil();
  }
}
static nanoclj_val_t checked_cdr(nanoclj_val_t p) {
  switch (type(p)) {
  case T_NIL:
  case T_PAIR:
  case T_LIST:
    return cdr(p);
  default:
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

static struct nanoclj_interface vtbl = {
  nanoclj_intern,
  cons,
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
  is_character,
  to_int,
  is_list,
  is_vector,
  size_checked,
  fill_vector_checked,
  vector_elem_checked,
  set_vector_elem_checked,
  is_reader,
  is_writer,
  is_pair,
  checked_car,
  checked_cdr,

  first_checked,
  is_empty_checked,
  rest_checked,
  
  is_symbol,
  is_keyword,
  symname,

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

#include "functions.h"
 
int nanoclj_init_custom_alloc(nanoclj_t * sc, func_alloc malloc, func_dealloc free, func_realloc realloc) {  
  int i, n = sizeof(dispatch_table) / sizeof(dispatch_table[0]);

#if USE_INTERFACE
  sc->vptr = &vtbl;
#endif
  sc->gensym_cnt = 0;
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
  sc->save_inport = sc->EMPTY;
  sc->loadport = sc->EMPTY;
  sc->nesting = 0;

  sc->active_element = mk_nil();
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
  typeflag(sc->EMPTY) = (T_NIL | T_GC_ATOM | MARK);
  car(sc->EMPTY) = cdr(sc->EMPTY) = sc->EMPTY;
  /* init sink */
  typeflag(sc->sink) = (T_PAIR | MARK);
  car(sc->sink) = sc->EMPTY;
  /* init c_nest */
  sc->c_nest = sc->EMPTY;

  sc->oblist = oblist_initial_value(sc);
  /* init global_env */
  new_frame_in_env(sc, sc->EMPTY);
  sc->root_env = sc->global_env = sc->envir;
  metadata_unchecked(sc->global_env) = mk_string(sc, "root");
  
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
  sc->WINDOW_SIZE = def_symbol(sc, "*window-size*");
  sc->WINDOW_SCALE_F = def_symbol(sc, "*window-scale-factor*");
  sc->MOUSE_POS = def_symbol(sc, "*mouse-pos*");

  sc->RECUR = def_symbol(sc, "recur");
  sc->AMP = def_symbol(sc, "&");
  sc->UNDERSCORE = def_symbol(sc, "_");
  sc->DOC = def_keyword(sc, "doc");
  sc->WIDTH = def_keyword(sc, "width");
  sc->HEIGHT = def_keyword(sc, "height");
    
  sc->SORTED_SET = def_symbol(sc, "sorted-set");
  sc->ARRAY_MAP = def_symbol(sc, "array-map");
  sc->REGEX = def_symbol(sc, "regex");
  sc->EMPTYSTR = mk_string(sc, "");
  sc->EMPTYVEC = mk_vector(sc, 0);

  nanoclj_intern(sc, sc->global_env, def_symbol(sc, "root"), sc->root_env);
  nanoclj_intern(sc, sc->global_env, def_symbol(sc, "nil"), mk_nil());
  nanoclj_intern(sc, sc->global_env, sc->MOUSE_POS, mk_nil());

  register_functions(sc);

  sc->sixel_term = has_sixels();
  sc->window_scale_factor = 1.0;

  sc->term_colors = get_term_colortype();
  
  return !sc->no_memory;
}

void nanoclj_set_input_port_file(nanoclj_t * sc, FILE * fin) {
  nanoclj_val_t inport = port_from_file(sc, fin, port_input);
  nanoclj_intern(sc, sc->global_env, sc->IN, inport);
}

void nanoclj_set_input_port_string(nanoclj_t * sc, const char *str, size_t length) {
  nanoclj_val_t inport = port_from_string(sc, (strview_t){ str, length }, port_input);
  nanoclj_intern(sc, sc->global_env, sc->IN, inport);
}

void nanoclj_set_output_port_file(nanoclj_t * sc, FILE * fout) {
  nanoclj_val_t p = port_from_file(sc, fout, port_output);
  nanoclj_intern(sc, sc->root_env, sc->OUT, p);

  update_window_info(sc, p);
}

void nanoclj_set_output_port_callback(nanoclj_t * sc,
				      void (*text) (const char *, size_t, void *),
				      void (*color) (double, double, double, void *),
				      void (*restore) (void *),
				      void (*image) (nanoclj_image_t *, void *)
				      ) {
  nanoclj_val_t p = port_from_callback(sc, text, color, restore, image, port_output);
  nanoclj_intern(sc, sc->root_env, sc->OUT, p);
  nanoclj_intern(sc, sc->global_env, sc->WINDOW_SIZE, mk_nil());
}

void nanoclj_set_output_port_string(nanoclj_t * sc, const char * str, size_t length) {
  nanoclj_val_t p = port_from_string(sc, (strview_t){ str, length }, port_output);
  nanoclj_intern(sc, sc->root_env, sc->OUT, p);
  nanoclj_intern(sc, sc->global_env, sc->WINDOW_SIZE, mk_nil());
}

void nanoclj_set_error_port_callback(nanoclj_t * sc, void (*text) (const char *, size_t, void *)) {
  nanoclj_val_t p = port_from_callback(sc, text, NULL, NULL, NULL, port_output);
  nanoclj_intern(sc, sc->global_env, sc->ERR, p);
}

void nanoclj_set_error_port_file(nanoclj_t * sc, FILE * fout) {
  nanoclj_val_t p = port_from_file(sc, fout, port_output);
  nanoclj_intern(sc, sc->global_env, sc->ERR, p);
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
  sc->global_env = sc->EMPTY;
  dump_stack_free(sc);
  sc->envir = sc->EMPTY;
  sc->code = sc->EMPTY;
  sc->args = sc->EMPTY;
  sc->value = sc->EMPTY;
#ifdef USE_RECUR_REGISTER
  sc->recur = sc->EMPTY;
#endif
  if (is_reader(sc->save_inport)) {
    typeflag(sc->save_inport) = T_GC_ATOM;
  }
  sc->save_inport = sc->EMPTY;
  if (is_reader(sc->loadport)) {
    typeflag(sc->loadport) = T_GC_ATOM;
  }
  sc->loadport = sc->EMPTY;
  sc->free(sc->strbuff);
  sc->free(sc->errbuff);
  gc(sc, sc->EMPTY, sc->EMPTY);

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
  sc->load_stack[0].backchar = -1;
  sc->load_stack[0].rep.stdio.file = fin;
  sc->loadport = mk_reader(sc, sc->load_stack);
  sc->retcode = 0;

  sc->load_stack[0].rep.stdio.curr_line = 0;
  if (fin != stdin && filename) {
    sc->load_stack[0].rep.stdio.filename = store_string(sc, strlen(filename), filename);
  } else {
    sc->load_stack[0].rep.stdio.filename = NULL;
  }

  sc->args = mk_integer(sc, sc->file_i);
  Eval_Cycle(sc, OP_T0LVL);
  typeflag(sc->loadport) = T_GC_ATOM;
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
  sc->load_stack[0].backchar = -1;
  sc->load_stack[0].rep.string.curr = buffer;
  sc->load_stack[0].rep.string.data.data = buffer;
  sc->load_stack[0].rep.string.data.size = len;
  sc->loadport = mk_reader(sc, sc->load_stack);
  sc->retcode = 0;
  sc->args = mk_integer(sc, sc->file_i);
  
  Eval_Cycle(sc, OP_T0LVL);
  typeflag(sc->loadport) = T_GC_ATOM;
  if (sc->retcode == 0) {
    sc->retcode = sc->nesting != 0;
  }
  return sc->value;
}

void nanoclj_intern(nanoclj_t * sc, nanoclj_val_t envir, nanoclj_val_t symbol, nanoclj_val_t value) {
  nanoclj_val_t x = find_slot_in_env(sc, envir, symbol, 0);
  if (x.as_long != sc->EMPTY.as_long) {
    set_slot_in_env(sc, x, value);
  } else {
    new_slot_spec_in_env(sc, envir, symbol, value);
  }
}

static inline void save_from_C_call(nanoclj_t * sc) {
  nanoclj_val_t saved_data = cons(sc,
				  car(sc->sink),
				  cons(sc,
				       sc->envir,
				       sc->dump));
  /* Push */
  sc->c_nest = cons(sc, saved_data, sc->c_nest);

  dump_stack_frame_t * next_frame = s_add_frame(sc);
  next_frame->is_c_call_return = true;
}
 
static inline void restore_from_C_call(nanoclj_t * sc) {
  car(sc->sink) = caar(sc->c_nest);
  sc->envir = cadar(sc->c_nest);
  sc->dump = cdr(cdar(sc->c_nest));
  /* Pop */
  sc->c_nest = cdr(sc->c_nest);
}

/* "func" and "args" are assumed to be already eval'ed. */
nanoclj_val_t nanoclj_call(nanoclj_t * sc, nanoclj_val_t func, nanoclj_val_t args) {
  save_from_C_call(sc);
  sc->envir = sc->global_env;
  sc->args = args;
  sc->code = func;
  sc->retcode = 0;
  Eval_Cycle(sc, OP_APPLY);
  restore_from_C_call(sc);
  
  return sc->value;
}

#if !NANOCLJ_STANDALONE
void nanoclj_register_foreign_func(nanoclj_t * sc, nanoclj_registerable * sr) {
  nanoclj_intern(sc, sc->global_env, def_symbol(sc, sr->name), mk_foreign_func(sc, sr->f));
}

void nanoclj_register_foreign_func_list(nanoclj_t * sc,
    nanoclj_registerable * list, int count) {
  int i;
  for (i = 0; i < count; i++) {
    nanoclj_register_foreign_func(sc, list + i);
  }
}

nanoclj_val_t nanoclj_apply0(nanoclj_t * sc, const char *procname) {
  return nanoclj_eval(sc, cons(sc, def_symbol(sc, procname), sc->EMPTY));
}

#endif

nanoclj_val_t nanoclj_eval(nanoclj_t * sc, nanoclj_val_t obj) {
  save_from_C_call(sc);

  sc->args = sc->EMPTY;
  sc->code = obj;
  sc->retcode = 0;

  Eval_Cycle(sc, OP_EVAL);
  restore_from_C_call(sc);
  return sc->value;
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
    printf("%s\n", get_version());
  }
  if (argc == 2 && str_eq(argv[1], "--help")) {
    printf("Usage: %s [switches] [--] [programfile] [arguments]\n", argv[0]);
    return 1;
  }
  if (argc == 2 && str_eq(argv[1], "--legal")) {
    legal();
    return 1;
  }

  nanoclj_t sc;
  if (!nanoclj_init(&sc)) {
    fprintf(stderr, "Could not initialize!\n");
    return 2;
  }
  
  nanoclj_set_input_port_file(&sc, stdin);
  nanoclj_set_output_port_file(&sc, stdout);
  nanoclj_set_error_port_file(&sc, stderr);
#if USE_DL
  nanoclj_intern(&sc, sc.global_env, def_symbol(&sc, "load-extension"),
		mk_foreign_func(&sc, scm_load_ext));
#endif

  nanoclj_val_t args = sc.EMPTY;
  for (int i = argc - 1; i >= 2; i--) {
    nanoclj_val_t value = mk_string(&sc, argv[i]);
    args = cons(&sc, value, args);
  }
  nanoclj_intern(&sc, sc.global_env, def_symbol(&sc, "*command-line-args*"), args);
  
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
    nanoclj_val_t x = find_slot_in_env(&sc, sc.envir, main, 1);
    if (x.as_long != sc.EMPTY.as_long) {
      x = slot_value_in_env(x);
      x = nanoclj_call(&sc, x, sc.EMPTY);
      return to_int(x);
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
