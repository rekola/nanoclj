#ifndef _NANOCLJ_TERM_H_
#define _NANOCLJ_TERM_H_

static inline void reset_color(FILE * fh) {
  fputs("\033[00m", fh);
}

static inline void set_basic_style(FILE * fh, int style) {
  fprintf(fh, "\033[%dm", style);
}

#endif
