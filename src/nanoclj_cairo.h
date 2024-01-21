#ifndef _NANOCLJ_CAIRO_H_
#define _NANOCLJ_CAIRO_H_

#include <cairo/cairo.h>
#include <cairo/cairo-pdf.h>

#define NANOCLJ_HAS_CANVAS 1

static inline void canvas_free(void * canvas) {
  if (canvas) cairo_destroy((cairo_t *)canvas);
}

static inline cairo_format_t get_suitable_cairo_format(int channels) {
  cairo_format_t f;
  switch (channels) {
  case 1: return CAIRO_FORMAT_A8;
  case 3: return CAIRO_FORMAT_RGB24;
  case 4: return CAIRO_FORMAT_ARGB32;
  default: return CAIRO_FORMAT_INVALID;
  }
}

static inline nanoclj_tensor_t * mk_canvas_backing_tensor(int width, int height, int channels) {
  if (channels == 1 && (width % 4) != 0) {
    width += 4 - (width % 4);
  }
  cairo_format_t f = get_suitable_cairo_format(channels);
  int stride = cairo_format_stride_for_width(f, width);
  nanoclj_tensor_t * tensor = mk_tensor_3d(nanoclj_i8, channels, channels == 1 ? 1 : 4, width, stride, height);
  tensor_mutate_fill_i8(tensor, 0);
  return tensor;
}

static inline void * mk_canvas(nanoclj_tensor_t * tensor, nanoclj_color_t fg, nanoclj_color_t bg) {
  int channels = tensor->ne[0], width = tensor->ne[1], height = tensor->ne[2];
  cairo_format_t f;
  switch (channels) {
  case 1: f = CAIRO_FORMAT_A8; break;
  case 3: f = CAIRO_FORMAT_RGB24; break;
  case 4: f = CAIRO_FORMAT_ARGB32; break;
  default: return NULL;
  }
  
  cairo_surface_t * surface = cairo_image_surface_create_for_data(tensor->data, f, width, height, tensor->nb[2]);
  cairo_t * cr = cairo_create(surface);
  cairo_surface_destroy(surface);

  if (channels >= 3 && bg.alpha > 0 && width > 0 && height > 0) {
    cairo_set_source_rgba(cr, bg.red / 255.0, bg.green / 255.0, bg.blue / 255.0, bg.alpha / 255.0);
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_fill(cr);
  }
  cairo_set_source_rgba(cr, fg.red / 255.0, fg.green / 255.0, fg.blue / 255.0, fg.alpha / 255.0);

  cairo_font_options_t * options = cairo_font_options_create();
  if (channels >= 3) {
    cairo_font_options_set_antialias(options, CAIRO_ANTIALIAS_SUBPIXEL);
  } else {
    cairo_font_options_set_antialias(options, CAIRO_ANTIALIAS_GRAY);
  }
  cairo_font_options_set_hint_style(options, CAIRO_HINT_STYLE_FULL);
  cairo_font_options_set_hint_metrics(options, CAIRO_HINT_METRICS_ON);
  cairo_set_font_options(cr, options);
  cairo_font_options_destroy(options);

  return cr;
}

static inline void * mk_canvas_pdf(double width, double height, strview_t fn0, nanoclj_color_t fg, nanoclj_color_t bg) {
  char * fn = alloc_c_str(fn0);

  cairo_surface_t * surface = cairo_pdf_surface_create(fn, width, height);
  free(fn);
  
  cairo_t * cr = cairo_create(surface);
  cairo_surface_destroy(surface);
  
  if (bg.alpha > 0 && (bg.red != 255 || bg.green != 255 || bg.blue != 255) && width > 0 && height > 0) {
    cairo_set_source_rgba(cr, bg.red / 255.0, bg.green / 255.0, bg.blue / 255.0, bg.alpha / 255.0);
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_fill(cr);
  }
  cairo_set_source_rgba(cr, fg.red / 255.0, fg.green / 255.0, fg.blue / 255.0, fg.alpha / 255.0);
  return cr;
}

static inline bool canvas_has_error(void * canvas) {
  return cairo_status((cairo_t *)canvas) != 0;
}

static inline const char * canvas_get_error_text(void * canvas) {
  return cairo_status_to_string(cairo_status((cairo_t *)canvas));
}

static inline int canvas_get_chan(void * canvas) {
  cairo_surface_t * surface = cairo_get_target((cairo_t *)canvas);
  switch (cairo_image_surface_get_format(surface)) {
  case CAIRO_FORMAT_A8: return 1;
  case CAIRO_FORMAT_RGB24: return 3;
  case CAIRO_FORMAT_ARGB32: return 4;
  default: return 0;
  }
}

static inline void canvas_flush(void * canvas) {
  cairo_surface_flush(cairo_get_target((cairo_t *)canvas));
}

static inline void canvas_set_color(void * canvas, nanoclj_color_t color) {
  cairo_set_source_rgba((cairo_t *)canvas, color.red / 255.0, color.green / 255.0, color.blue / 255.0, color.alpha / 255.0);
}

static inline void canvas_set_linear_gradient(void * canvas, nanoclj_tensor_t * tensor, double x0, double y0, double x1, double y1) {
  cairo_pattern_t * pat = cairo_pattern_create_linear(x0, y0, x1, y1);
  size_t n = tensor->ne[1];
  for (size_t i = 0; i < n; i++) {
    float pos = tensor_get_f32_2d(tensor, 0, i), red = tensor_get_f32_2d(tensor, 1, i);
    float green = tensor_get_f32_2d(tensor, 2, i), blue = tensor_get_f32_2d(tensor, 3, i);
    float alpha = tensor_get_f32_2d(tensor, 4, i);
    cairo_pattern_add_color_stop_rgba(pat, pos, red, green, blue, alpha);
  }
  cairo_set_source((cairo_t *)canvas, pat);
  cairo_pattern_destroy(pat);
}

static inline void canvas_set_font_size(void * canvas, double size) {
  cairo_set_font_size((cairo_t *)canvas, size);
}

static inline void canvas_set_font_face(void * canvas, strview_t family, bool is_italic, bool is_bold) {
  char * fam = alloc_c_str(family);
  cairo_select_font_face((cairo_t *)canvas, fam,
			 is_italic ? CAIRO_FONT_SLANT_ITALIC : CAIRO_FONT_SLANT_NORMAL,
			 is_bold ? CAIRO_FONT_WEIGHT_BOLD : CAIRO_FONT_WEIGHT_NORMAL);
  free(fam);
}

static inline void canvas_set_line_width(void * canvas, double w) {
  cairo_set_line_width((cairo_t *)canvas, w);
}

static inline void canvas_move_to(void * canvas, double x, double y) {
  cairo_move_to((cairo_t *)canvas, x, y);
}

static inline void canvas_line_to(void * canvas, double x, double y) {
  cairo_line_to((cairo_t *)canvas, x, y);
}

static inline void canvas_arc(void * canvas, double xc, double yc, double radius, double angle1, double angle2) {
  cairo_arc((cairo_t *)canvas, xc, yc, radius, angle1, angle2);
}

static inline void canvas_new_path(void * canvas) {
  cairo_new_path((cairo_t *)canvas);
}

static inline void canvas_close_path(void * canvas) {
  cairo_close_path((cairo_t *)canvas);
}

static inline void canvas_clip(void * canvas) {
  cairo_clip((cairo_t *)canvas);
}

static inline void canvas_reset_clip(void * canvas) {
  cairo_reset_clip((cairo_t *)canvas);
}

static inline void canvas_stroke(void * canvas, bool preserve) {
  if (preserve) {
    cairo_stroke_preserve((cairo_t *)canvas);
  } else {
    cairo_stroke((cairo_t *)canvas);
  }
}

static inline void canvas_fill(void * canvas, bool preserve) {
  if (preserve) {
    cairo_fill_preserve((cairo_t *)canvas);
  } else {
    cairo_fill((cairo_t *)canvas);
  }
}

static inline void canvas_save(void * canvas) {
  cairo_save((cairo_t *)canvas);
}

static inline void canvas_restore(void * canvas) {
  cairo_restore((cairo_t *)canvas);
}

static inline void canvas_show_text(void * canvas, strview_t text) {
  char * tmp = alloc_c_str(text);
  cairo_show_text((cairo_t *)canvas, tmp);
  free(tmp);
}

static inline void canvas_get_text_extents(void * canvas, strview_t text, double * width, double * height) {
  char * tmp = alloc_c_str(text);
  cairo_text_extents_t extents;
  cairo_text_extents((cairo_t *)canvas, tmp, &extents);
  *width = extents.width;
  *height = extents.height;
  /* double x_bearing;
     double y_bearing;
     double x_advance;
     double y_advance; */
  free(tmp);
}

static inline void canvas_text_path(void * canvas, strview_t text) {
  char * tmp = alloc_c_str(text);
  cairo_text_path((cairo_t *)canvas, tmp);
  free(tmp);
}

static inline void canvas_translate(void * canvas, double x, double y) {
  cairo_translate((cairo_t *)canvas, x, y);
}

static inline void canvas_scale(void * canvas, double x, double y) {
  cairo_scale((cairo_t *)canvas, x, y);
}

static inline void canvas_rotate(void * canvas, double angle) {
  cairo_rotate((cairo_t *)canvas, angle);
}

#endif
