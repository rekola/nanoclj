#ifndef _NANOCLJ_CAIRO_H_
#define _NANOCLJ_CAIRO_H_

#include <cairo/cairo.h>

#define NANOCLJ_HAS_CANVAS 1

static void finalize_canvas(nanoclj_t * sc, void * canvas) {
  if (canvas) cairo_destroy((cairo_t *)canvas);
}

static inline nanoclj_val_t mk_canvas(nanoclj_t * sc, int width, int height) {
  cairo_surface_t * surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
  cairo_t * cr = cairo_create(surface);
  cairo_surface_destroy(surface);
  
  struct cell * x = get_cell_x(sc, sc->EMPTY, sc->EMPTY);
  _typeflag(x) = T_CANVAS | T_GC_ATOM;
  _canvas_unchecked(x) = cr;
  _metadata_unchecked(x) = mk_nil();

  cairo_set_source_rgba(cr, 1, 1, 1, 1);
  cairo_rectangle(cr, 0, 0, width, height);
  cairo_fill(cr);
  cairo_set_source_rgba(cr, 1, 0, 0, 1);

  cairo_set_font_size(cr, 20);
  
  return mk_pointer(x);  
}

static inline nanoclj_val_t canvas_create_image(nanoclj_t * sc, void * canvas) {
  cairo_t * cr = (cairo_t *)canvas;
  cairo_surface_t * surface = cairo_get_target(cr);

  cairo_surface_flush(surface);
  
  unsigned char * data = cairo_image_surface_get_data(surface);
  int width = cairo_image_surface_get_width(surface);
  int height = cairo_image_surface_get_height(surface);
  
  return mk_image(sc, width, height, 4, data);
}

static inline void canvas_set_color(void * canvas, double r, double g, double b) {
  cairo_set_source_rgb((cairo_t *)canvas, r, g, b);
}

static inline void canvas_move_to(void * canvas, double x, double y) {
  cairo_move_to((cairo_t *)canvas, x, y);
}

static inline void canvas_line_to(void * canvas, double x, double y) {
  cairo_line_to((cairo_t *)canvas, x, y);
}

static inline void canvas_close_path(void * canvas) {
  cairo_close_path((cairo_t *)canvas);
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

static inline void canvas_show_text(void * canvas, const char * text, size_t len) {
  char * tmp = (char *)malloc(len + 1);
  if (tmp) {
    memcpy(tmp, text, len);
    tmp[len] = 0;
    cairo_show_text((cairo_t *)canvas, tmp);
    free(tmp);
  }
}

#endif
