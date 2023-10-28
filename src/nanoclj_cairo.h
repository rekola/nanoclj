#ifndef _NANOCLJ_CAIRO_H_
#define _NANOCLJ_CAIRO_H_

#include <cairo/cairo.h>

#define NANOCLJ_HAS_CANVAS 1

static void finalize_canvas(nanoclj_t * sc, void * canvas) {
  if (canvas) cairo_destroy((cairo_t *)canvas);
}

static inline void * mk_canvas(nanoclj_t * sc, int width, int height, int channels, nanoclj_color_t fg, nanoclj_color_t bg) {
  cairo_format_t f;
  switch (channels) {
  case 1: f = CAIRO_FORMAT_A8; break;
  case 3: f = CAIRO_FORMAT_RGB24; break;
  case 4: f = CAIRO_FORMAT_ARGB32; break;
  default: return NULL;
  }

  if (channels == 1 && (width % 4) != 0) {
    width += 4 - (width % 4);
  }
    
  cairo_surface_t * surface = cairo_image_surface_create(f, width, height);
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

static inline imageview_t canvas_get_imageview(void * canvas) {
  cairo_t * cr = (cairo_t *)canvas;
  cairo_surface_t * surface = cairo_get_target(cr);

  cairo_surface_flush(surface);
  
  uint8_t * data = cairo_image_surface_get_data(surface);
  int width = cairo_image_surface_get_width(surface);
  int height = cairo_image_surface_get_height(surface);
  int stride = cairo_image_surface_get_stride(surface);

  nanoclj_internal_format_t f;
  switch (cairo_image_surface_get_format(surface)) {
  case CAIRO_FORMAT_A8: f = nanoclj_r8; break;
  case CAIRO_FORMAT_RGB24: f = nanoclj_bgr8_32; break;
  case CAIRO_FORMAT_ARGB32: f = nanoclj_bgra8; break;
  default: return (imageview_t){0};
  }
  
  return (imageview_t){ data, width, height, stride, f };
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

static inline void canvas_set_linear_gradient(void * canvas, nanoclj_cell_t * p0, nanoclj_cell_t * p1, nanoclj_cell_t * colormap) {
  cairo_pattern_t * pat = cairo_pattern_create_linear(to_double(vector_elem(p0, 0)), to_double(vector_elem(p0, 1)),
						      to_double(vector_elem(p1, 0)), to_double(vector_elem(p1, 1)));
  size_t n = get_size(colormap);
  for (size_t i = 0; i < n; i++) {
    nanoclj_cell_t * e = decode_pointer(vector_elem(colormap, i));
    double pos = to_double(vector_elem(e, 0));
    nanoclj_cell_t * color = decode_pointer(vector_elem(e, 1));
    cairo_pattern_add_color_stop_rgb(pat, pos,
				     to_double(vector_elem(color, 0)),
				     to_double(vector_elem(color, 1)),
				     to_double(vector_elem(color, 2)));
  }

  cairo_set_source((cairo_t *)canvas, pat);
  cairo_pattern_destroy(pat);
}

static inline void canvas_set_font_size(void * canvas, double size) {
  cairo_set_font_size((cairo_t *)canvas, size);
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

static inline void canvas_stroke(void * canvas) {
  cairo_stroke((cairo_t *)canvas);
}

static inline void canvas_fill(void * canvas) {
  cairo_fill((cairo_t *)canvas);
}

static inline void canvas_save(void * canvas) {
  cairo_save((cairo_t *)canvas);
}

static inline void canvas_restore(void * canvas) {
  cairo_restore((cairo_t *)canvas);
}

static inline void canvas_show_text(nanoclj_t * sc, void * canvas, strview_t text) {
  char * tmp = alloc_c_str(sc, text);
  cairo_show_text((cairo_t *)canvas, tmp);
  sc->free(tmp);
}

static inline void canvas_get_text_extents(nanoclj_t * sc, void * canvas, strview_t text, double * width, double * height) {
  char * tmp = alloc_c_str(sc, text);
  cairo_text_extents_t extents;
  cairo_text_extents((cairo_t *)canvas, tmp, &extents);
  *width = extents.width;
  *height = extents.height;
  /* double x_bearing;
     double y_bearing;
     double x_advance;
     double y_advance; */
  sc->free(tmp);
}

#endif
