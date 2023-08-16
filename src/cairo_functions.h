#ifndef _NANOCLJ_CAIRO_FUNCTIONS_H_
#define _NANOCLJ_CAIRO_FUNCTIONS_H_

#include <cairo/cairo.h>

#define NANOCLJ_HAS_CANVAS 1

struct nanoclj_canvas_t {
  cairo_surface_t * surface;
  cairo_t * cr;
};

static void finalize_canvas(nanoclj_t * sc, nanoclj_canvas_t * canvas) {
  cairo_destroy(canvas->cr);
  cairo_surface_destroy(canvas->surface);
  sc->free(canvas);
}

static inline nanoclj_val_t mk_canvas(nanoclj_t * sc, int width, int height) {
  struct cell * x = get_cell_x(sc, sc->EMPTY, sc->EMPTY);
  nanoclj_canvas_t * canvas = (nanoclj_canvas_t*)malloc(sizeof(nanoclj_canvas_t));
  _typeflag(x) = T_CANVAS | T_GC_ATOM;
  _canvas_unchecked(x) = canvas;
  _metadata_unchecked(x) = mk_nil();

  canvas->surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
  canvas->cr = cairo_create(canvas->surface);

  cairo_set_source_rgba(canvas->cr, 1, 1, 1, 1);
  cairo_rectangle(canvas->cr, 0, 0, width, height);
  cairo_fill(canvas->cr);
  cairo_set_source_rgba(canvas->cr, 1, 0, 0, 1);
  
  return mk_pointer(x);  
}

static inline nanoclj_val_t Canvas_create(nanoclj_t * sc, nanoclj_val_t args) {
  int width = to_int(car(args)), height = to_int(cadr(args));
  return mk_canvas(sc, width, height);
}

static inline nanoclj_val_t Canvas_move_to(nanoclj_t * sc, nanoclj_val_t args) {
  nanoclj_canvas_t * canvas = canvas_unchecked(car(args));
  double x = to_double(cadr(args)), y = to_double(caddr(args));

  cairo_move_to(canvas->cr, x, y);

  return (nanoclj_val_t)kTRUE;
}

static inline nanoclj_val_t Canvas_line_to(nanoclj_t * sc, nanoclj_val_t args) {
  nanoclj_canvas_t * canvas = canvas_unchecked(car(args));
  double x = to_double(cadr(args)), y = to_double(caddr(args));

  cairo_line_to(canvas->cr, x, y);

  return (nanoclj_val_t)kTRUE;
}

static inline nanoclj_val_t Canvas_stroke(nanoclj_t * sc, nanoclj_val_t args) {
  nanoclj_canvas_t * canvas = canvas_unchecked(car(args));

  cairo_stroke(canvas->cr);
  
  return (nanoclj_val_t)kTRUE;
}

static inline nanoclj_val_t Canvas_create_image(nanoclj_t * sc, nanoclj_val_t args) {
  nanoclj_canvas_t * canvas = canvas_unchecked(car(args));
  unsigned char * data = cairo_image_surface_get_data(canvas->surface);
  int width = cairo_image_surface_get_width(canvas->surface);
  int height = cairo_image_surface_get_height(canvas->surface);
  
  return mk_image(sc, width, height, 4, data);
}

#endif
