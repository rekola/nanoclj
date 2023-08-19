#ifndef _NANOCLJ_UTF8_H_
#define _NANOCLJ_UTF8_H_

#include <stdint.h>

#define IS_ASCII(C) (((C) & ~0x7F) == 0)

static inline const char * utf8_next(const char * p) {
  if (*p == 0) return 0;
  unsigned char *s = (unsigned char*) p;
  if (IS_ASCII(*s)) return p + 1;
  return p + ((*s < 0xE0) ? 2 : ((*s < 0xF0) ? 3 : 4));
}

static inline int_fast32_t utf8_decode(const char *p) {
  unsigned char *s = (unsigned char*) p;
  if (IS_ASCII(*s)) {
    return *s;
  }
  int_fast32_t bytes = (*s < 0xE0) ? 2 : ((*s < 0xF0) ? 3 : 4);
  int_fast32_t c = *s & ((0x100 >> bytes) - 1);
  while (--bytes) {
    c = (c << 6) | (*(++s) & 0x3F);
  }
  return c;
}

static inline int utf8_num_codepoints(const char *s) {
  int count = 0;
  while (*s) {
    count += (*s++ & 0xC0) != 0x80;
  }
  return count;
}

static inline int utf8_get_codepoint_pos(const char * s, int ci) {
  const char * p = s;
  for (; *p && ci > 0; ci--) {
    p = utf8_next(p);
  }
  return (int)(p - s);
}

#endif
