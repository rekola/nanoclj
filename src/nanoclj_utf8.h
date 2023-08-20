#ifndef _NANOCLJ_UTF8_H_
#define _NANOCLJ_UTF8_H_

#include <stdint.h>

#define IS_ASCII(C) (((C) & ~0x7F) == 0)

static inline const char * utf8_next(const char * p) {
  if (*p == 0) return 0;
  const unsigned char *s = (const unsigned char*) p;
  if (IS_ASCII(*s)) return p + 1;
  return p + ((*s < 0xE0) ? 2 : ((*s < 0xF0) ? 3 : 4));
}

static inline const char * utf8_prev(const char * p) {
  const unsigned char *s = (const unsigned char*) p;
  s--;
  while ( (*s & 0xC0) == 0x80 ) s--;
  return (const char *)s;
}

static inline int_fast32_t utf8_decode(const char *p) {
  const unsigned char *s = (const unsigned char*) p;
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

static inline int utf8_get_codepoint_pos(const char * p, int ci) {
  int i = 0;
  for (; *p && ci > 0; ci--, i++) {
    p = utf8_next(p);
  }
  return i;
}

int mk_wcwidth(wchar_t ucs);

static inline int utf8_num_cells(const char *p, size_t len) {
  const char * end = p + len;
  int nc = 0;
  while (p < end) {
    int c = utf8_decode(p);
    p = utf8_next(p);
    nc += mk_wcwidth(c);
  }
  return nc;
}

#endif
