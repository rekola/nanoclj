#ifndef _NANOCLJ_UTILS_H_
#define _NANOCLJ_UTILS_H_

#include "nanoclj_types.h"

static inline int clamp(int v, int min, int max) {
  if (v < min) return min;
  if (v > max) return max;
  return v;
}

static inline nanoclj_color_t mk_color4d(double red, double green, double blue, double alpha) {
  return (nanoclj_color_t){
    clamp((int)(red * 255), 0, 255),
    clamp((int)(green * 255), 0, 255),
    clamp((int)(blue * 255), 0, 255),
    clamp((int)(alpha * 255), 0, 255) };
}

static inline nanoclj_color_t mk_color3i(int red, int green, int blue) {
  return (nanoclj_color_t){
    clamp(red, 0, 255),
    clamp(green, 0, 255),
    clamp(blue, 0, 255),
    255 };
}

static inline int digit(int32_t c, int radix) {
  if (c >= '0' && c <= '9') {
    c -= '0';
  } else if (c >= 'a' && c <= 'z') {
    c -= 'a' - 10;
  } else if (c >= 'A' && c <= 'Z') {
    c -= 'A' - 10;
  } else if (c >= 0xff21 && c <= 0xff3a) { /* fullwidth uppercase Latin letters */
    c -= 0xff21 - 10;
  } else if (c >= 0xff41 && c <= 0xff5a) { /* fullwidth lowercase Latin letters */
    c -= 0xff41 - 10;
  } else {
    return -1;
  }
  if (c < radix) return c;
  else return -1;
}

static inline bool strview_eq(strview_t a, strview_t b) {
  return a.size == b.size && memcmp(a.ptr, b.ptr, a.size) == 0;
}

static inline int strview_cmp(strview_t a, strview_t b) {
  if (a.size < b.size) {
    return -1;
  } else if (a.size > b.size) {
    return +1;
  } else {
    return memcmp(a.ptr, b.ptr, a.size);
  }
}

#endif

