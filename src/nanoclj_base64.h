#ifndef _NANOCLJ_BASE64_H_
#define _NANOCLJ_BASE64_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Encode to base64 */
static inline size_t base64_encode(const uint8_t * input, size_t len, uint8_t ** out_buffer) {
  static const uint8_t alphabet[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

  uint8_t *in = input, * out = *out_buffer = malloc(len * 4 / 3 + 4);
  if (!out) return 0;
  
  for (; len >= 3; in += 3, len -= 3) {
    *out++ = alphabet[in[0] >> 2];
    *out++ = alphabet[((in[0] & 0x03) << 4) | (in[1] >> 4)];
    *out++ = alphabet[((in[1] & 0x0f) << 2) | (in[2] >> 6)];
    *out++ = alphabet[in[2] & 0x3f];
  }
  if (len == 2) {
    *out++ = alphabet[in[0] >> 2];
    *out++ = alphabet[((in[0] & 0x03) << 4) | (in[1] >> 4)];
    *out++ = alphabet[(in[1] & 0x0f) << 2];
    *out++ = '=';
  } else if (len == 1) {
    *out++ = alphabet[in[0] >> 2];
    *out++ = alphabet[(in[0] & 0x03) << 4];
    *out++ = '=';
    *out++ = '=';
  }

  return out - *out_buffer;
}

#endif
