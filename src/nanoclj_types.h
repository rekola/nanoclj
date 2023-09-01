#ifndef _NANOCLJ_TYPES_H_
#define _NANOCLJ_TYPES_H_

typedef struct {
  int32_t width, height, channels;
  unsigned char * data;
} nanoclj_image_t;

typedef struct {
  int32_t frames, channels, sample_rate;
  float * data;
} nanoclj_audio_t;

typedef enum {
  nanoclj_colortype_16 = 0,
  nanoclj_colortype_256,
  nanoclj_colortype_true
} nanoclj_colortype_t;

#endif
