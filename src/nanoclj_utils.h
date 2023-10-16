#ifndef _NANOCLJ_UTILS_H_
#define _NANOCLJ_UTILS_H_

#include <time.h>

#include "nanoclj_types.h"

static inline int clamp(int v, int min, int max) {
  if (v < min) return min;
  if (v > max) return max;
  return v;
}

static inline nanoclj_color_t mk_color(double red, double green, double blue, double alpha) {
  return (nanoclj_color_t){
    clamp((int)(red * 255), 0, 255),
    clamp((int)(green * 255), 0, 255),
    clamp((int)(blue * 255), 0, 255),
    clamp((int)(alpha * 255), 0, 255) };
}

#endif

