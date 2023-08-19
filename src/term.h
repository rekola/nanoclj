#ifndef _NANOCLJ_TERM_H_
#define _NANOCLJ_TERM_H_

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#include <io.h>

#define isatty _isatty
#define write _write

#endif /* _WIN32 */

#include <string.h>

#include "nanoclj_utils.h"

static inline void reset_color(FILE * fh) {
  if (isatty(fileno(fh))) {
    fputs("\033[00m", fh);
  }
}

static inline void set_basic_style(FILE * fh, int style) {
  if (isatty(fileno(fh))) {
    fprintf(fh, "\033[%dm", style);
  }
}

static inline void set_truecolor(FILE * fh, double r, double g, double b) {
  if (isatty(fileno(fh))) {
    fprintf(fh, "\033[%d;2;%d;%d;%dm",
	    38,
	    clamp((int)(r * 255), 0, 255),
	    clamp((int)(g * 255), 0, 255),
	    clamp((int)(b * 255), 0, 255));
  }
}

static inline bool has_truecolor() {
#ifdef _WIN32
  return false;
#else
  const char * ev = getenv("COLORTERM");
  return ev && (strcmp(ev, "truecolor") == 0 || strcmp(ev, "24bit") == 0);
#endif
}

#endif
