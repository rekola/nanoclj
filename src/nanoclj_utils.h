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

static inline nanoclj_color_t mk_color4i(int red, int green, int blue, int alpha) {
  return (nanoclj_color_t){
    clamp(red, 0, 255),
    clamp(green, 0, 255),
    clamp(blue, 0, 255),
    clamp(alpha, 0, 255) };
}

static inline nanoclj_color_t mix(nanoclj_color_t x, nanoclj_color_t y, float a) {
  return mk_color4i(x.red * (1 - a) + y.red * a,
		    x.green * (1 - a) + y.green * a,
		    x.blue * (1 - a) + y.blue * a,
		    x.alpha * (1 - a) + y.alpha * a
		    );
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

static inline int get_format_channels(nanoclj_internal_format_t f) {
  switch (f) {
  case nanoclj_r8: return 1;
  case nanoclj_ra8: return 2;
  case nanoclj_rgb8: return 3;
  case nanoclj_rgba8: return 4;
  case nanoclj_bgr8_32: return 3;
  case nanoclj_bgra8: return 4;
  }
  return 0;
}

static inline int get_format_bpp(nanoclj_internal_format_t f) {
  switch (f) {
  case nanoclj_r8:
    return 1;
  case nanoclj_ra8:
    return 2;
  case nanoclj_rgb8:
    return 3;
  case nanoclj_rgba8:
  case nanoclj_bgr8_32:
  case nanoclj_bgra8:
    return 4;
  }
  return 0;
}

static inline size_t get_image_size(size_t height, size_t stride, nanoclj_internal_format_t format) {
  return height * stride;
}

static inline uint8_t * convert_imageview(imageview_t iv, nanoclj_internal_format_t to) {
  uint8_t * output = malloc(get_image_size(iv.height, iv.stride, to));
  if (iv.format == nanoclj_bgr8_32 && to == nanoclj_rgb8) {
    for (size_t i = 0; i < iv.width * iv.height; i++) {
      output[3 * i + 0] = iv.ptr[4 * i + 2];
      output[3 * i + 1] = iv.ptr[4 * i + 1];
      output[3 * i + 2] = iv.ptr[4 * i + 0];
    }
  } else if (iv.format == nanoclj_bgra8 && to == nanoclj_rgba8) {
    for (size_t i = 0; i < iv.width * iv.height; i++) {
      output[4 * i + 0] = iv.ptr[4 * i + 2];
      output[4 * i + 1] = iv.ptr[4 * i + 1];
      output[4 * i + 2] = iv.ptr[4 * i + 0];
      output[4 * i + 3] = iv.ptr[4 * i + 3];
    }
  } else {
    free(output);
    return NULL;
  }
  return output;
}

#endif

