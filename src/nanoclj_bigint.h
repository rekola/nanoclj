#ifndef _NANOCLJ_BIGINT_H_
#define _NANOCLJ_BIGINT_H_

#include "nanoclj_tensor.h"

static inline long long bigintview_to_long(bigintview_t bv) {
  uint64_t v = 0;
  if (bv.size) {
    const uint32_t * limbs = bv.limbs;
    for (int64_t i = bv.size - 1; i >= 0; i--) {
      v <<= 32;
      v += limbs[i];
    }
  }
  return v * bv.sign;
}

static inline double bigintview_to_double(bigintview_t bv) {
  double v = 0;
  if (bv.size) {
    const uint32_t * limbs = bv.limbs;
    for (int64_t i = bv.size - 1; i >= 0; i--) {
      v *= 0x100000000;
      v += limbs[i];
    }
  }
  return bv.sign * v;
}

static inline nanoclj_tensor_t * bigintview_to_tensor(bigintview_t bv) {
  nanoclj_tensor_t * t = mk_tensor_1d(nanoclj_i32, bv.size);
  memcpy(t->data, bv.limbs, bv.size * sizeof(uint32_t));
  return t;
}

static inline size_t bigintview_to_string(bigintview_t bv, char ** buff) {
  nanoclj_tensor_t * tensor = bigintview_to_tensor(bv);
  nanoclj_tensor_t * digits = mk_tensor_1d_padded(nanoclj_i8, 0, 256);
  while (!tensor_is_empty(tensor)) {
    /* divide the bigint by 1e9 and put the remainder in r */
    uint64_t r = tensor_bigint_mutate_idivmod(tensor, 1000000000);
    /* print the remainder */
    for (int_fast8_t i = 0; i < 9 && (!tensor_is_empty(tensor) || r > 0); i++, r /= 10) {
      uint8_t c = '0' + (r % 10);
      tensor_mutate_append_bytes(digits, &c, 1);
    }
  }
  if (bv.sign == -1) {
    tensor_mutate_append_bytes(digits, (const uint8_t *)"-", 1);
  }
  size_t n = digits->ne[0];
  *buff = malloc(n + 1);
  for (size_t i = 0; i < n; i++) (*buff)[n - 1 - i] = *((char *)digits->data + i);
  (*buff)[n] = 0;
  tensor_free(digits);
  tensor_free(tensor);
  return n;
}

static inline uint32_t bigint_hashcode(bigintview_t bv) {
  uint32_t h = 0;
  const uint32_t * data = bv.limbs;
  /* In Java BigInts are Big-endian */
  for (int64_t i = bv.size - 1; i >= 0; i--) {
    h = 31 * h + data[i];
  }
  return bv.sign * (int32_t)h;
}

static inline bool bigint_eq(bigintview_t a, bigintview_t b) {
  if (a.sign != b.sign) return false;
  uint32_t n = a.size;
  if (n != b.size) return false;
  for (int64_t i = 0; i < n; i++) {
    if (a.limbs[i] != b.limbs[i]) return false;
  }
  return true;
}

static inline int bigint_cmp(bigintview_t a, bigintview_t b) {
  if (a.sign < b.sign) return -1;
  if (a.sign > b.sign) return +1;
  
  int64_t n = a.size;
  if (n < b.size) {
    return -1;
  } else if (n > b.size) {
    return +1;
  } else {
    const uint32_t * a_limbs = a.limbs, * b_limbs = b.limbs;
    while (n > 0) {
      n--;
      if (a_limbs[n] > b_limbs[n]) return a.sign;
      else if (a_limbs[n] < b_limbs[n]) return -a.sign;
    }
    return 0;
  }
}

/* compares bigint a with integer b0 */
static inline int bigint_icmp(bigintview_t a, int64_t b0) {
  int64_t sign = b0 < 0 ? -1 : (b0 > 0 ? 1 : 0);
  if (a.sign < sign) return -1;
  if (a.sign > sign) return +1;

  int64_t n = a.size;
  if (n > 2) {
    return sign;
  } else if (n == 0) {
    return b0 == 0 ? 0 : -sign;
  } else {
    uint64_t b = b0 == LLONG_MIN ? (uint64_t)LLONG_MAX + 1 : llabs(b0);
    const uint32_t * limbs = a.limbs;
    if (n == 1) {
      if (limbs[0] > b) return sign;
      if (limbs[0] < b) return -sign;
      return 0;
    }
    if (b <= 0xffffffff) return sign;
    uint64_t b_hi = b >> 32, b_lo = b & 0xffffffff;
    if (limbs[1] > b_hi) return sign;
    if (limbs[1] < b_hi) return -sign;
    if (limbs[0] > b_lo) return sign;
    if (limbs[0] < b_lo) return -sign;
    return 0;
  }
}

static inline nanoclj_tensor_t * bigint_inc(bigintview_t bv) {
  nanoclj_tensor_t * r = bigintview_to_tensor(bv);
  if (bv.sign >= 0) {
    tensor_bigint_mutate_add_int(r, 1);
  } else {
    tensor_bigint_mutate_sub_int(r, 1);
  }
  return r;
}

static inline nanoclj_tensor_t * bigint_dec(bigintview_t bv) {
  nanoclj_tensor_t * r = bigintview_to_tensor(bv);
  if (bv.sign > 0) {
    tensor_bigint_mutate_sub_int(r, 1);
  } else {
    tensor_bigint_mutate_add_int(r, 1);
  }
  return r;
}

static inline void bigint_mutate_and(nanoclj_tensor_t * a, bigintview_t b) {
  if (a->ne[0] > b.size) a->ne[0] = b.size;
  uint32_t * limbs_a = a->data;
  const uint32_t * limbs_b = b.limbs;
  for (int64_t i = 0; i < a->ne[0]; i++) {
    limbs_a[i] &= limbs_b[i];
  }
}

static inline void bigint_mutate_or(nanoclj_tensor_t * a, bigintview_t b) {
  while (a->ne[0] < b.size) {
    tensor_mutate_append_i32(a, 0);
  }
  uint32_t * limbs_a = a->data;
  const uint32_t * limbs_b = b.limbs;
  for (int64_t i = 0; i < b.size; i++) {
    limbs_a[i] |= limbs_b[i];
  }
}

static inline bool bigint_is_representable_as_long(bigintview_t bv) {
  if (bv.size > 2) return false;
  if (bv.size <= 1) return true;
  uint64_t v = bv.limbs[0] + ((uint64_t)bv.limbs[1] << 32);
  if (v <= (uint64_t)LLONG_MAX) return true;
  if (bv.sign == -1 && v <= (uint64_t)LLONG_MAX + 1) return true;
  return false;
}

#endif
