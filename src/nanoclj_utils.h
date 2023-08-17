#ifndef _NANOCLJ_UTILS_H_
#define _NANOCLJ_UTILS_H_

static inline int clamp(int v, int min, int max) {
  if (v < min) return min;
  if (v > max) return max;
  return v;
}

#endif

