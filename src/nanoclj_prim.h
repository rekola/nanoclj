#ifndef _NANOCLJ_PRIM_H_
#define _NANOCLJ_PRIM_H_

#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Masks for NaN packing */
#define MASK_SIGN		UINT64_C(0x8000)
#define MASK_EXPONENT		UINT64_C(0x7ff0)
#define MASK_QUIET		UINT64_C(0x0008)
#define MASK_TYPE		UINT64_C(0x0007)

#define MASK_SIGNATURE		0xffff000000000000
#define MASK_PAYLOAD		0x0000ffffffffffff

/* Signatures for primitive types */
#define SIGNATURE(x)		((MASK_EXPONENT | MASK_QUIET | (x)) << 48)

#define SIGNATURE_NNAN		SIGNATURE(MASK_SIGN | 0)
#define SIGNATURE_CELL		SIGNATURE(MASK_SIGN | 1)
#define SIGNATURE_EMPTYLIST	SIGNATURE(MASK_SIGN | 2)
#define SIGNATURE_REGEX		SIGNATURE(MASK_SIGN | 3)
#define SIGNATURE_NOTFOUND      SIGNATURE(MASK_SIGN | 4)
#define SIGNATURE_NIL		SIGNATURE(MASK_SIGN | 7)
#define SIGNATURE_NAN		SIGNATURE(0)
#define SIGNATURE_BOOLEAN	SIGNATURE(1)
#define SIGNATURE_INTEGER	SIGNATURE(2)
#define SIGNATURE_CODEPOINT	SIGNATURE(3)
#define SIGNATURE_PROC		SIGNATURE(4)
#define SIGNATURE_KEYWORD	SIGNATURE(5)
#define SIGNATURE_SYMBOL	SIGNATURE(6)
#define SIGNATURE_ALIAS		SIGNATURE(7)

/* Predefined primitive values */
#define kNAN	  UINT64_C(0x7ff8000000000000)
#define kFALSE	  UINT64_C(0x7ff9000000000000)
#define kTRUE	  UINT64_C(0x7ff9000000000001)
#define kNNAN	  UINT64_C(0xfff8000000000000)
#define kEMPTY	  UINT64_C(0xfffa000000000000)
#define kNOTFOUND UINT64_C(0xfffc000000000000)
#define kNIL	  UINT64_C(0xffffffffffffffff)

static inline bool is_nil(nanoclj_val_t v) {
  return v.as_long == kNIL;
}

static inline bool is_emptylist(nanoclj_val_t v) {
  return v.as_long == kEMPTY;
}

static inline nanoclj_val_t mk_val_from_long(uint64_t l) {
  nanoclj_val_t v;
  v.as_long = l;
  return v;
}

static inline nanoclj_val_t mk_nil() {
  return mk_val_from_long(kNIL);  
}

static inline nanoclj_val_t mk_notfound() {
  return mk_val_from_long(kNOTFOUND);
}

static inline nanoclj_val_t mk_byte(int8_t n) {
  return mk_val_from_long(SIGNATURE_INTEGER | (UINT64_C(0) << 32) | (uint32_t)(int32_t)n);
}

static inline nanoclj_val_t mk_short(int16_t n) {
  return mk_val_from_long(SIGNATURE_INTEGER | (UINT64_C(1) << 32) | (uint32_t)(int32_t)n);
}

static inline nanoclj_val_t mk_int(int32_t num) {
  return mk_val_from_long(SIGNATURE_INTEGER | (UINT64_C(2) << 32) | (uint32_t)num);
}

static inline nanoclj_val_t mk_long_prim(int32_t num) {
  return mk_val_from_long(SIGNATURE_INTEGER | (UINT64_C(3) << 32) | (uint32_t)num);
}

/* Creates a primitive for utf8 codepoint */
static inline nanoclj_val_t mk_codepoint(int c) {
  return mk_val_from_long(SIGNATURE_CODEPOINT | (uint32_t)c);
}

static inline nanoclj_val_t mk_proc(uint32_t op) {
  return mk_val_from_long(SIGNATURE_PROC | op);
}

static inline nanoclj_val_t mk_emptylist() {
  return mk_val_from_long(SIGNATURE_EMPTYLIST);
}

static inline nanoclj_val_t mk_boolean(bool b) {
  return mk_val_from_long(b ? kTRUE : kFALSE);
}

static inline nanoclj_val_t mk_double(double d) {
  nanoclj_val_t v;
  v.as_double = d;
  return v;
}

static inline nanoclj_val_t mk_pointer(const nanoclj_cell_t * ptr) {
  if (ptr) {
    return mk_val_from_long(SIGNATURE_CELL | (uint64_t)ptr);
  } else {
    return mk_nil();
  }
}

static inline nanoclj_val_t mk_symbol_pointer(void * ptr) {
  return mk_val_from_long(SIGNATURE_SYMBOL | (uint64_t)ptr);
}

static inline nanoclj_val_t mk_keyword_pointer(void * ptr) {
  return mk_val_from_long(SIGNATURE_KEYWORD | (uint64_t)ptr);
}

static inline nanoclj_val_t mk_alias_pointer(void * ptr) {
  return mk_val_from_long(SIGNATURE_ALIAS | (uint64_t)ptr);
}

static inline nanoclj_val_t mk_regex_pointer(void * ptr) {
  return mk_val_from_long(SIGNATURE_REGEX | (uint64_t)ptr);
}

static inline nanoclj_val_t mk_list(const nanoclj_cell_t * ptr) {
  return ptr ? mk_pointer(ptr) : mk_emptylist();
}

static inline nanoclj_cell_t * decode_pointer(nanoclj_val_t value) {
  if (value.as_long == kNIL) return 0;
  return (nanoclj_cell_t *)(value.as_long & MASK_PAYLOAD);
}

static inline int32_t decode_integer(nanoclj_val_t value) {
  return (uint32_t)(value.as_long & 0xffffffff);
}

static inline bool is_cell(nanoclj_val_t v) {
  return (v.as_long & MASK_SIGNATURE) == SIGNATURE_CELL;
}

static inline bool is_symbol(nanoclj_val_t v) {
  return (v.as_long & MASK_SIGNATURE) == SIGNATURE_SYMBOL;
}

static inline bool is_alias(nanoclj_val_t v) {
  return (v.as_long & MASK_SIGNATURE) == SIGNATURE_ALIAS;
}

static inline bool is_keyword(nanoclj_val_t v) {
  return (v.as_long & MASK_SIGNATURE) == SIGNATURE_KEYWORD;
}

static inline bool is_boolean(nanoclj_val_t v) {
  return (v.as_long & MASK_SIGNATURE) == SIGNATURE_BOOLEAN;
}

static inline bool is_regex(nanoclj_val_t v) {
  return (v.as_long & MASK_SIGNATURE) == SIGNATURE_REGEX;
}

static inline bool is_notfound(nanoclj_val_t v) {
  return v.as_long == kNOTFOUND;
}

static inline bool is_found(nanoclj_val_t v) {
  return v.as_long != kNOTFOUND;
}

#ifdef __cplusplus
}
#endif
#endif
