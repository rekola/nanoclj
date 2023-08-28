#ifndef _NANOCLJ_UTILS_H_
#define _NANOCLJ_UTILS_H_

#include <time.h>

static inline int clamp(int v, int min, int max) {
  if (v < min) return min;
  if (v > max) return max;
  return v;
}

static inline void ms_sleep(long long ms) {
  struct timespec t, t2;
  t.tv_sec = ms / 1000;
  t.tv_nsec = (ms % 1000) * 1000000;
  nanosleep(&t, &t2);
}

#endif

