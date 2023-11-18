#ifndef _NANOCLJ_CHAR_H_
#define _NANOCLJ_CHAR_H_

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

#endif
