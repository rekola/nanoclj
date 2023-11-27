#ifndef _NANOCLJ_PRIM_H_
#define _NANOCLJ_PRIM_H_

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
#define SIGNATURE_INTEGER	(MASK_EXPONENT | MASK_QUIET | 2)
#define SIGNATURE_CODEPOINT	(MASK_EXPONENT | MASK_QUIET | 3)
#define SIGNATURE_PROC		(MASK_EXPONENT | MASK_QUIET | 4)
#define SIGNATURE_KEYWORD	(MASK_EXPONENT | MASK_QUIET | 5)
#define SIGNATURE_SYMBOL	(MASK_EXPONENT | MASK_QUIET | 6)
#define SIGNATURE_UNASSIGNED	(MASK_EXPONENT | MASK_QUIET | 7)

/* Predefined primitive values */
#define kNIL	UINT64_C(18444492273895866368)
#define kTRUE	UINT64_C(9221401712017801217)
#define kFALSE	UINT64_C(9221401712017801216)

static inline nanoclj_val_t mk_nil() {
  return (nanoclj_val_t)kNIL;
}

static inline bool is_cell(nanoclj_val_t v) {
  return (v.as_long >> 48) == SIGNATURE_CELL;
}

static inline bool is_unassigned(nanoclj_val_t v) {
  return (v.as_long >> 48) == SIGNATURE_UNASSIGNED;
}

static inline nanoclj_val_t mk_unassigned() {
  return (nanoclj_val_t)(SIGNATURE_UNASSIGNED << 48);
}

#endif
