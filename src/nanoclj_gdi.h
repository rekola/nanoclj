#ifndef _NANOCLJ_GDI_H_
#define _NANOCLJ_GDI_H_

#include <wingdi.h>
#include <winuser.h>

#include "nanoclj_utils.h"

#define NANOCLJ_HAS_CANVAS 1

static void canvas_free(void * canvas) {
  if (canvas) {
    HDC hdc = (HDC)canvas;
    DeleteDC(hdc);
  }
}

static inline void * mk_canvas(int width, int height) {
  HDC hdc = CreateCompatibleDC(NULL);
  HBITMAP bitmap = CreateBitmap(width, height, 3, 24, NULL);
  SelectObject(hdc, bitmap);
    
  return (void *)hdc;
}

static inline nanoclj_val_t canvas_create_image(void * canvas) {
  HDC hdc = (HDC)canvas;
  return mk_nil();
}

static inline void canvas_flush(void * canvas) {

}

static inline void canvas_set_color(void * canvas, double r0, double g0, double b0) {
  HDC hdc = (HDC)canvas;
  int r = clamp((int)(r0 / 255), 0, 255), g = clamp((int)(g0 / 255), 0, 255), b = clamp((int)(b0 / 255), 0, 255);
  SetDCBrushColor(hdc, RGB(r, g, b));
  SetDCPenColor(hdc, RGB(r, g, b));
}

static inline void canvas_set_font_size(void * canvas, double size) {

}

static inline void canvas_set_line_width(void * canvas, double w) {

}

static inline void canvas_move_to(void * canvas, double x, double y) {
  MoveToEx((HDC)canvas, (int)x, (int)y, NULL);  
}

static inline void canvas_line_to(void * canvas, double x, double y) {
  LineTo((HDC)canvas, (int)x, (int)y);  
}

static inline void canvas_arc(void * canvas, double xc, double yc, double radius, double angle1, double angle2) {

}

static inline void canvas_stroke(void * canvas) {

}

static inline void canvas_fill(void * canvas) {

}

static inline void canvas_close_path(void * canvas) {

}

static inline void canvas_save(void * canvas) {

}

static inline void canvas_restore(void * canvas) {

}

static inline void canvas_show_text(void * canvas, strview_t text) {
  HDC hdc = (HDC)canvas;

#if 0
  DrawText(hdc,
	   [in, out] LPCTSTR lpchText,
	   [in]      int     cchText,
	   [in, out] LPRECT  lprc,
	   [in]      UINT    format
	   );
#endif
}

static inline void canvas_get_text_extents(void * canvas, strview_t text, double * width, double * height) {
  *width = *height = 0;  
}

#endif
