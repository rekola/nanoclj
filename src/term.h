#ifndef _NANOCLJ_TERM_H_
#define _NANOCLJ_TERM_H_

#include <string.h>

#include "nanoclj_utils.h"

static inline void reset_color(FILE * fh) {
  fputs("\033[00m", fh);
}

static inline void set_basic_style(FILE * fh, int style) {
  fprintf(fh, "\033[%dm", style);
}

static inline void set_truecolor(FILE * fh, double r, double g, double b) {
  fprintf(fh, "\033[%d;2;%d;%d;%dm",
	  38,
	  clamp((int)(r * 255), 0, 255),
	  clamp((int)(g * 255), 0, 255),
	  clamp((int)(b * 255), 0, 255));
}

static inline bool has_truecolor() {
  const char * ev = getenv("COLORTERM");
  return strcmp(ev, "truecolor") == 0 || strcmp(ev, "24bit") == 0;
}

#endif
