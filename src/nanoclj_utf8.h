#ifndef _NANOCLJ_UTF8_H_
#define _NANOCLJ_UTF8_H_

#include <stdint.h>
#include <stdbool.h>

static inline size_t utf8_sequence_length(uint8_t lead) {
  if (lead < 0x80) return 1;
  else if ((lead >> 5) == 0x6) return 2;
  else if ((lead >> 4) == 0xe) return 3;
  else if ((lead >> 3) == 0x1e) return 4;
  else return 0;
}

static inline const char * utf8_next(const char * p) {
  return p + utf8_sequence_length((uint8_t)*p);
}

static inline const char * utf8_prev(const char * p) {
  const unsigned char *s = (const unsigned char*) p;
  s--;
  while ( (*s & 0xC0) == 0x80 ) s--;
  return (const char *)s;
}

static inline int32_t decode_utf8(const char *s) {
  const uint8_t * p = (const uint8_t *)s;
  uint32_t codepoint = *p;
  switch (utf8_sequence_length(*p)) {
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

/* Encodes a codepoint to utf8 and returns the encoded size */
static inline size_t encode_utf8(int32_t c, char *p) {
  if (c > 0) {
    if (c < 0x80) {
      *p++ = c;
      return 1;
    } else if (c < 0x800) {
      *p++ = (c >> 6) | 0xc0;
      *p++ = (c & 0x3f) | 0x80;
      return 2;
    } else if (c < 0x10000) {
      *p++ = (c >> 12) | 0xe0;
      *p++ = ((c >> 6) & 0x3f) | 0x80;
      *p++ = (c & 0x3f) | 0x80;
      return 3;
    } else if (c < 0x10ffff) {
      *p++ = (c >> 18) | 0xf0;
      *p++ = ((c >> 12) & 0x3f)| 0x80;
      *p++ = ((c >> 6) & 0x3f) | 0x80;
      *p++ = (c & 0x3f) | 0x80;
      return 4;
    }
  }
  // invalid character, store placeholder
  *p++ = 0xef;  
  *p++ = 0xbf;
  *p++ = 0xbd;
  return 3;
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

int mk_wcwidth(int32_t ucs);

static inline int utf8_num_cells(const char *p, size_t len) {
  const char * end = p + len;
  int nc = 0;
  while (p < end) {
    nc += mk_wcwidth(decode_utf8(p));
    p = utf8_next(p);
  }
  return nc;
}

/* returns true if c is space or invisible character */
static inline bool unicode_isspace(int32_t c) {
  switch (c) {
  case 0x0020: /* space */
  case 0x00a0: /* nbsp */
  case 0x1680: /* ogham space mark */
  case 0x2000: /* en quad */
  case 0x2001: /* em quad */
  case 0x2002: /* en space */
  case 0x2003: /* em space */
  case 0x2004: /* three-per-em space */
  case 0x2005: /* four-per-em space */
  case 0x2006: /* six-per-em space */
  case 0x2007: /* figure space */
  case 0x2008: /* punctuation space */
  case 0x2009: /* thin space */
  case 0x200a: /* hair space */
  case 0x200b: /* zero width space */
  case 0x200c: /* zero width non-joiner */
  case 0x200d: /* zero width joiner */
  case 0x202f: /* narrow no-break space */
  case 0x205f: /* medium mathematical space */
  case 0x3000: /* ideographic space */
  case 0xfeff: /* zero width no-break space */    
    return true;
  }
  return false;
}

/* returns true if c is a control character */
static inline bool unicode_isctrl(int32_t c) {
  if (c < ' ' || (c >= 0x7f && c < 0xa0)) {
    return true;
  }
  switch (c) {
  case 0x200e: /* left-to-right mark */
  case 0x200f: /* right-to-left mark */
  case 0x202c: /* pop directional formatting */
  case 0xfe0f: /* variation selector-16 */
    return true;
  }
  return false;
}

#endif
