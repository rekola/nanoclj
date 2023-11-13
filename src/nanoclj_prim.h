#ifndef _NANOCLJ_PRIM_H_
#define _NANOCLJ_PRIM_H_

/* Predefined primitive values */
#define kNIL	UINT64_C(18444492273895866368)
#define kTRUE	UINT64_C(9221401712017801217)
#define kFALSE	UINT64_C(9221401712017801216)

static inline nanoclj_val_t mk_nil() {
  return (nanoclj_val_t)kNIL;
}

#endif
