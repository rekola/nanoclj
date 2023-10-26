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

typedef struct {
  uint8_t red, green, blue, alpha;
} nanoclj_color_t;

typedef enum {
  nanoclj_colortype_none = 0,
  nanoclj_colortype_16,
  nanoclj_colortype_256,
  nanoclj_colortype_true
} nanoclj_colortype_t;

typedef enum {
  nanoclj_mode_unknown = 0,
  nanoclj_mode_inline,
  nanoclj_mode_block
} nanoclj_display_mode_t;

typedef struct {
  const uint8_t * ptr;
  uint32_t width, height, channels;
} imageview_t;

typedef struct {
  const char * ptr;
  size_t size;
} strview_t;

#endif
