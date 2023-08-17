#ifndef _NANOCLJ_CAIRO_FUNCTIONS_H_
#define _NANOCLJ_CAIRO_FUNCTIONS_H_

#include <cairo/cairo.h>

#define NANOCLJ_HAS_CANVAS 1

static void finalize_canvas(nanoclj_t * sc, void * canvas) {
  cairo_destroy((cairo_t *)canvas);
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
  
  return mk_pointer(x);  
}

static inline nanoclj_val_t Canvas_create(nanoclj_t * sc, nanoclj_val_t args) {
  int width = to_int(car(args)), height = to_int(cadr(args));
  return mk_canvas(sc, width, height);
}

static inline nanoclj_val_t Canvas_move_to(nanoclj_t * sc, nanoclj_val_t args) {
  cairo_t * cr = (cairo_t *)canvas_unchecked(car(args));
  double x = to_double(cadr(args)), y = to_double(caddr(args));

  cairo_move_to(cr, x, y);

  return (nanoclj_val_t)kTRUE;
}

static inline nanoclj_val_t Canvas_line_to(nanoclj_t * sc, nanoclj_val_t args) {
  cairo_t * cr = (cairo_t *)canvas_unchecked(car(args));
  double x = to_double(cadr(args)), y = to_double(caddr(args));

  cairo_line_to(cr, x, y);

  return (nanoclj_val_t)kTRUE;
}

static inline nanoclj_val_t Canvas_stroke(nanoclj_t * sc, nanoclj_val_t args) {
  cairo_t * cr = (cairo_t *)canvas_unchecked(car(args));

  cairo_stroke(cr);
  
  return (nanoclj_val_t)kTRUE;
}

static inline nanoclj_val_t Canvas_create_image(nanoclj_t * sc, nanoclj_val_t args) {
  cairo_t * cr = (cairo_t *)canvas_unchecked(car(args));
  cairo_surface_t * surface = cairo_get_target(cr);
  
  unsigned char * data = cairo_image_surface_get_data(surface);
  int width = cairo_image_surface_get_width(surface);
  int height = cairo_image_surface_get_height(surface);
  
  return mk_image(sc, width, height, 4, data);
}

#endif
