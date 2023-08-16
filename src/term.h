#ifndef _NANOCLJ_TERM_H_
#define _NANOCLJ_TERM_H_

#include <string.h>

static inline void reset_color(FILE * fh) {
  fputs("\033[00m", fh);
}

static inline void set_basic_style(FILE * fh, int style) {
  fprintf(fh, "\033[%dm", style);
}

static inline void set_truecolor(FILE * fh, int r, int g, int b) {
  fprintf(fh, "\033[%d;2;%d;%d;%dm", 38, r, g, b);
}

static bool has_truecolor() {
  const char * ev = getenv("COLORTERM");
  return strcmp(ev, "truecolor") == 0;
}

#endif
