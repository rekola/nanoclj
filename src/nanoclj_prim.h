#ifndef _NANOCLJ_PRIM_H_
#define _NANOCLJ_PRIM_H_

/* Masks for NaN packing */
#define MASK_SIGN		UINT64_C(0x8000)
#define MASK_EXPONENT		UINT64_C(0x7ff0)
#define MASK_QUIET		UINT64_C(0x0008)
#define MASK_TYPE		UINT64_C(0x0007)

#define MASK_PAYLOAD		0x0000ffffffffffff

/* Signatures for primitive types */
#define SIGNATURE_CELL		(MASK_EXPONENT | MASK_QUIET | MASK_SIGN | 0)
#define SIGNATURE_EMPTYLIST	(MASK_EXPONENT | MASK_QUIET | MASK_SIGN | 1)
#define SIGNATURE_NIL		(MASK_EXPONENT | MASK_QUIET | MASK_SIGN | 7)
#define SIGNATURE_NAN		(MASK_EXPONENT | MASK_QUIET | 0)
#define SIGNATURE_BOOLEAN	(MASK_EXPONENT | MASK_QUIET | 1)
#define SIGNATURE_INTEGER	(MASK_EXPONENT | MASK_QUIET | 2)
#define SIGNATURE_CODEPOINT	(MASK_EXPONENT | MASK_QUIET | 3)
#define SIGNATURE_PROC		(MASK_EXPONENT | MASK_QUIET | 4)
#define SIGNATURE_KEYWORD	(MASK_EXPONENT | MASK_QUIET | 5)
#define SIGNATURE_SYMBOL	(MASK_EXPONENT | MASK_QUIET | 6)

/* Predefined primitive values */
#define kNIL	UINT64_C(0xffffffffffffffff)
#define kEMPTY	UINT64_C(0xfff9000000000000)
#define kNAN	UINT64_C(0x7ff8000000000000)
#define kFALSE	UINT64_C(0x7ff9000000000000)
#define kTRUE	UINT64_C(0x7ff9000000000001)

static inline bool is_nil(nanoclj_val_t v) {
  return v.as_long == kNIL;
}

static inline bool is_emptylist(nanoclj_val_t v) {
  return v.as_long == kEMPTY;
}

static inline bool is_cell(nanoclj_val_t v) {
  return (v.as_long >> 48) == SIGNATURE_CELL;
}

static inline nanoclj_val_t mk_nil() {
  return (nanoclj_val_t)kNIL;
}

static inline nanoclj_val_t mk_byte(int8_t n) {
  return (nanoclj_val_t)((SIGNATURE_INTEGER << 48) | (UINT64_C(0) << 32) | (uint32_t)(int32_t)n);
}

static inline nanoclj_val_t mk_short(int16_t n) {
  return (nanoclj_val_t)((SIGNATURE_INTEGER << 48) | (UINT64_C(1) << 32) | (uint32_t)(int32_t)n);
}

static inline nanoclj_val_t mk_int(int32_t num) {
  return (nanoclj_val_t)((SIGNATURE_INTEGER << 48) | (UINT64_C(2) << 32) | (uint32_t)num);
}

/* Creates a primitive for utf8 codepoint */
static inline nanoclj_val_t mk_codepoint(int c) {
  return (nanoclj_val_t)((SIGNATURE_CODEPOINT << 48) | (uint32_t)c);
}

static inline nanoclj_val_t mk_proc(enum nanoclj_opcode op) {
  return (nanoclj_val_t)((SIGNATURE_PROC << 48) | (uint32_t)op);
}

static inline nanoclj_val_t mk_emptylist() {
  return (nanoclj_val_t)(SIGNATURE_EMPTYLIST << 48);
}

static inline nanoclj_val_t mk_boolean(bool b) {
  nanoclj_val_t v;
  v.as_long = b ? kTRUE : kFALSE;
  return v;
}

static inline nanoclj_val_t mk_double(double n) {
  /* Canonize all kinds of NaNs such as -NaN to ##NaN */
  return (nanoclj_val_t)(isnan(n) ? NAN : n);
}

static inline nanoclj_val_t mk_pointer(const nanoclj_cell_t * ptr) {
  if (ptr) {
    return (nanoclj_val_t)((SIGNATURE_CELL << 48) | (uint64_t)ptr);
  } else {
    return mk_nil();
  }
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

static inline bool is_symbol(nanoclj_val_t v) {
  return (v.as_long >> 48) == SIGNATURE_SYMBOL;
}

static inline bool is_keyword(nanoclj_val_t v) {
  return (v.as_long >> 48) == SIGNATURE_KEYWORD;
}

static inline bool is_boolean(nanoclj_val_t v) {
  return (v.as_long >> 48) == SIGNATURE_BOOLEAN;
}

#endif
