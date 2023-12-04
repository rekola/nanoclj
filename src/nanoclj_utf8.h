#ifndef _NANOCLJ_UTF8_H_
#define _NANOCLJ_UTF8_H_

#include <stdint.h>
#include <stdbool.h>
#include <utf8proc.h>

static inline size_t utf8_sequence_length(uint8_t lead) {
  if (lead < 0x80) return 1;
  else if ((lead >> 5) == 0x6) return 2;
  else if ((lead >> 4) == 0xe) return 3;
  else if ((lead >> 3) == 0x1e) return 4;
  else return 0;
}

static inline const char * utf8_next(const char * p) {
  size_t n = utf8_sequence_length((uint8_t)*p);
  return p + (n != 0 ? n : 1);
}

static inline const char * utf8_prev(const char * p) {
  const unsigned char *s = (const unsigned char*) p;
  s--;
  while ( (*s & 0xC0) == 0x80 ) s--;
  return (const char *)s;
}

static inline int32_t decode_utf8(const char *s) {
  const uint8_t * p = (const uint8_t *)s;
  size_t n = utf8_sequence_length(*p);
  if (n == 0) return 0xfffd; /* return REPLACEMENT CHARACTER */
  uint32_t codepoint = *p;
  switch (n) {
  case 2:
    p++;
    codepoint = ((codepoint << 6) & 0x7ff) + (*p & 0x3f);
    break;
  case 3:
    p++;
    codepoint = ((codepoint << 12) & 0xffff) + ((*p << 6) & 0xfff);
    p++;
    codepoint += *p & 0x3f;
    break;
  case 4:
    p++;
    codepoint = ((codepoint << 18) & 0x1fffff) + ((*p << 12) & 0x3ffff);
    p++;
    codepoint += (*p << 6) & 0xfff;
    p++;
    codepoint += *p & 0x3f;
    break;
  }
  return codepoint;
}

/* Returns the number of codepoints in utf8 string */
static inline long long utf8_num_codepoints(const char *s, size_t size) {
  const char * end = s + size;
  long long count = 0;
  for (; s < end; s++) {
    if ((*s & 0xC0) != 0x80) count++;
  }
  return count;
}

static inline int utf8_num_cells(const char *p, size_t len) {
  const char * end = p + len;
  int nc = 0;
  while (p < end) {
    nc += utf8proc_charwidth(decode_utf8(p));
    p = utf8_next(p);
  }
  return nc;
}

#endif
