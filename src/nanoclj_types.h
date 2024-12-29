#ifndef _NANOCLJ_TYPES_H_
#define _NANOCLJ_TYPES_H_

struct nanoclj_cell_t;
typedef struct nanoclj_cell_t nanoclj_cell_t;
typedef struct nanoclj_tensor_s nanoclj_tensor_t;
typedef struct nanoclj_graph_array_s nanoclj_graph_array_t;

#define NANOCLJ_MAX_DIMS 3

typedef union {
  uint64_t as_long;
  double as_double;
} nanoclj_val_t;

typedef enum {
  nanoclj_boolean = 1, /* 8-bit */
  nanoclj_i8,
  nanoclj_i16,
  nanoclj_i32,
  nanoclj_f32,
  nanoclj_f64,
  nanoclj_val,
} nanoclj_tensor_type_t;

typedef enum {
  nanoclj_r8 = 1,
  nanoclj_ra8,
  nanoclj_rgb8,
  nanoclj_rgba8,
  /* Cairo internal formats */
  nanoclj_bgr8_32,
  nanoclj_bgra8
} nanoclj_internal_format_t;

typedef struct {
  nanoclj_tensor_t * tensor;
  int32_t sign;
} nanoclj_bigint_t;

typedef struct {
  nanoclj_tensor_t * numerator, * denominator;
  int32_t sign;
} nanoclj_ratio_t;

typedef struct {
  int n_dims;
  int64_t ne[NANOCLJ_MAX_DIMS];
  size_t nb[NANOCLJ_MAX_DIMS + 1];
  void * data;
  nanoclj_tensor_type_t type;
} tensorview_t;

typedef struct {
  bool is_valid;
  char * scheme;
  char * host;
  char * path;
  int port;
} nanoclj_uri_t;

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
  nanoclj_no_gfx = 0,
  nanoclj_sixel,
  nanoclj_kitty
} nanoclj_graphics_t;

typedef enum {
  nanoclj_mode_unknown = 0,
  nanoclj_mode_inline,
  nanoclj_mode_block
} nanoclj_display_mode_t;

typedef struct {
  nanoclj_color_t fg;
  nanoclj_color_t bg;
  nanoclj_graphics_t gfx;
  nanoclj_colortype_t color;
} nanoclj_termdata_t;

typedef struct {
  const uint8_t * ptr;
  uint32_t width, height, stride;
  nanoclj_internal_format_t format;
} imageview_t;

typedef struct {
  const uint32_t * limbs;
  int32_t size, sign;
} bigintview_t;

typedef struct {
  const char * ptr;
  int size;
} strview_t;

typedef struct {
  float x, y;
} nanoclj_vec2f;

typedef struct {
  nanoclj_vec2f pos, ppos;
  nanoclj_val_t data;
} nanoclj_node_t;

typedef struct {
  uint32_t source, target;
  nanoclj_val_t data;
} nanoclj_edge_t;

typedef struct {
  size_t num_rows, num_columns;
} nanoclj_table_t;

#endif
