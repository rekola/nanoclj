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
  for (int64_t i = 0; i < bv.size; i++) {
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

#endif
