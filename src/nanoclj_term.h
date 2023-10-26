#ifndef _NANOCLJ_TERM_H_
#define _NANOCLJ_TERM_H_

#include <unistd.h>

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#include <io.h>

#define isatty _isatty
#define write _write

#else

#include <termios.h>
#include <sys/ioctl.h>

#endif

#include <string.h>

#include "nanoclj_utils.h"
#include "nanoclj_types.h"

#if NANOCLJ_SIXEL
#include <sixel.h>

static int sixel_write(char *data, int size, void *priv) {
  return fwrite(data, 1, size, (FILE *)priv);
}

static inline bool print_image_sixel(imageview_t iv) {
  int pf = 0;
  switch (iv.channels) {
  case 1: pf = SIXEL_PIXELFORMAT_G8; break;
  case 3: pf = SIXEL_PIXELFORMAT_BGR888; break;
  case 4: pf = SIXEL_PIXELFORMAT_BGRA8888; break;
  default: return false;
  }

  sixel_dither_t * dither;
  sixel_dither_new(&dither, -1, NULL);
  sixel_dither_initialize(dither, (uint8_t*)iv.ptr, iv.width, iv.height, pf, SIXEL_LARGE_NORM, SIXEL_REP_CENTER_BOX, SIXEL_QUALITY_HIGHCOLOR);
  
  sixel_output_t * output;
  sixel_output_new(&output, sixel_write, stdout, NULL);
  
  /* convert pixels into sixel format and write it to output context */
  sixel_encode((uint8_t*)iv.ptr,
	       iv.width,
	       iv.height,
	       8,
	       dither,
	       output);

  sixel_output_destroy(output);
  sixel_dither_destroy(dither);

  return true;
}
#endif

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

static inline int convert_to_256color(nanoclj_color_t color) {
  if ((color.red / 10) == (color.green / 10) && (color.green / 10) == (color.blue / 10)) {
    int grey = color.red /= 10;
    if (grey == 0) {
      return 0;
    } else if (grey >= 25) {
      return 15;
    } else {
      return 232 + (grey - 1);
    }
  } else {
    int r = color.red / 43;
    int g = color.green / 43;
    int b = color.blue / 43;
    return 16 + r * 6 * 6 + g * 6 + b;
  }
}

static inline void set_term_fg_color(FILE * fh, nanoclj_color_t color, nanoclj_colortype_t colors) {
  if (isatty(fileno(fh))) {
    switch (colors) {
    case nanoclj_colortype_none:
    case nanoclj_colortype_16:
      break;
    case nanoclj_colortype_256:
      fprintf(fh, "\033[38:5:%dm", convert_to_256color(color));    
      break;
    case nanoclj_colortype_true:
      fprintf(fh, "\033[38;2;%d;%d;%dm", color.red, color.green, color.blue);
      break;
    }
  }
}

static inline void set_term_bg_color(FILE * fh, nanoclj_color_t color, nanoclj_colortype_t colors) {
  if (isatty(fileno(fh))) {
    switch (colors) {
    case nanoclj_colortype_none:
    case nanoclj_colortype_16:
      break;
    case nanoclj_colortype_256:
      fprintf(fh, "\033[48:5:%dm", convert_to_256color(color));    
      break;
    case nanoclj_colortype_true:
      fprintf(fh, "\033[48;2;%d;%d;%dm", color.red, color.green, color.blue);
      break;
    }
  }
}

static inline void set_display_mode(FILE * fh, nanoclj_display_mode_t mode) {
  if (isatty(fileno(fh))) {
    switch (mode) {
    case nanoclj_mode_unknown:
      break;
    case nanoclj_mode_inline:
      fputs("\033[?8452h", stdout);
      break;
    case nanoclj_mode_block:
      fputs("\033[?8452l", stdout);
      break;
    }
  }
}

static inline struct termios set_raw_mode(int fd) {
  struct termios orig_term;
  tcgetattr(fd, &orig_term);

  struct termios raw = orig_term;
  
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= CS8;
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  raw.c_cc[VMIN] = 1;
  raw.c_cc[VTIME] = 0;
  tcsetattr(fd, TCSANOW, &raw);

  return orig_term;
}

static inline bool term_read_upto(int fd, char * buf, size_t n, char delim) {
  size_t i = 0;
  while (i < n - 1) {
    if (read(fd, buf + i, 1) != 1) {
      break;
    }
    if (buf[i] == delim) {
      buf[i] = '\0';
      return true;
    }
    i++;
  }
  return false;
}


static inline bool get_window_size(FILE * in, FILE * out, int * cols, int * rows, int * width, int * height) {
  if (!isatty(fileno(out))) {
    return false;
  }
  
  struct winsize ws;
  
  if (ioctl(1, TIOCGWINSZ, &ws) != -1) {
    *cols = ws.ws_col;
    *rows = ws.ws_row;
    *width = ws.ws_xpixel;
    *height = ws.ws_ypixel;
  } else {
    *cols = 80;
    *rows = 25;
    *width = 0;
    *height = 0;
  }
  
  if (!*width || !*height) {
    char buf[32];
    struct termios orig_term = set_raw_mode(fileno(in));
    
    if (write(fileno(out), "\033[14t", 5) != 5) {
      return false;
    }

    if (term_read_upto(fileno(in), buf, sizeof(buf), 't') &&
	sscanf(buf, "\033[4;%d;%dt", height, width) == 2) {
      /* success */
    }
    
    tcsetattr(STDIN_FILENO, TCSADRAIN, &orig_term);
  }

  return true;
}
	   
static bool get_cursor_position(FILE * in, FILE * out, int * x, int * y) {
  if (isatty(fileno(in)) && isatty(fileno(out))) {
    struct termios orig_term = set_raw_mode(fileno(in));
    
    if (write(fileno(out), "\x1b[6n", 4) != 4) {
      return false;
    }

    char buf[32];
    bool r = false;
    if (term_read_upto(fileno(in), buf, sizeof(buf), 'R') &&
	sscanf(buf, "\033[%d;%d", y, x) == 2) {
      r = true;
    }
    
    tcsetattr(STDIN_FILENO, TCSADRAIN, &orig_term);
    return r;
  } else {
    return false;
  }
}

/* Gets the current therminal fg and bg colors, or guesses them. */
static inline bool get_current_colors(FILE * in, FILE * out, nanoclj_color_t * fg, nanoclj_color_t * bg) {
  if (!isatty(fileno(out))) {
    return false;
  }
  struct termios orig_term = set_raw_mode(fileno(in));

  char buf[32];   
  bool has_bg = false;
  uint32_t r, g, b;
  if (write(fileno(out), "\033]11;?\a", 7) == 7 && term_read_upto(fileno(in), buf, sizeof(buf), '\a')) {
    if (sscanf(buf, "\033]11;rgb:%2x/%2x/%2x", &r, &g, &b) == 3) {
      *bg = mk_color3i(r, g, b);
      has_bg = true;
    } else if (sscanf(buf, "\033]11;rgb:%x/%x/%x", &r, &g, &b) == 3) {
      *bg = mk_color3i(r >> 8, g >> 8, b >> 8);
      has_bg = true;
    }
  }

  if (has_bg) {
    bool has_fg = false;
    if (write(fileno(out), "\033]10;?\a", 7) == 7 && term_read_upto(fileno(in), buf, sizeof(buf), '\a')) {
      if (sscanf(buf, "\033]10;rgb:%2x/%2x/%2x", &r, &g, &b) == 3) {
	*fg = mk_color3i(r, g, b);
	has_fg = true;
      } else if (sscanf(buf, "\033]10;rgb:%x/%x/%x", &r, &g, &b) == 3) {
	*fg = mk_color3i(r >> 8, g >> 8, b >> 8);
	has_fg = true;
      }
    }
    if (!has_fg) {
      if (bg->red < 128 && bg->green < 128 && bg->blue < 128) {
	*fg = mk_color3i(255, 255, 255);
      } else {
	*fg = mk_color3i(0, 0, 0);
      }
    }
  }
  
  tcsetattr(STDIN_FILENO, TCSADRAIN, &orig_term);

  return has_bg;
}

static inline nanoclj_colortype_t get_term_colortype(FILE * stdout) {
#ifdef _WIN32
  return nanoclj_colortype_16;
#else
  if (!isatty(fileno(stdout))) {
    return nanoclj_colortype_none;
  } else {
    const char * colorterm = getenv("COLORTERM");
    if ((colorterm &&
	 (strcmp(colorterm, "truecolor") == 0 || strcmp(colorterm, "24bit") == 0)) ||
	getenv("MLTERM")) {
      return nanoclj_colortype_true;
    }
    
    const char * term_program = getenv("TERM_PROGRAM");
    if (term_program && (strcmp(term_program, "iTerm.app") == 0 ||
			 strcmp(term_program, "HyperTerm") == 0 ||
			 strcmp(term_program, "Hyper") == 0 ||
			 strcmp(term_program, "MacTerm") == 0)) {
      return nanoclj_colortype_true;
    } else if (term_program && strcmp(term_program, "Apple_Terminal") == 0) {
      return nanoclj_colortype_256;
    }
    
    const char * term = getenv("TERM");
    if (!term || strcmp(term, "dumb") == 0) {
      return nanoclj_colortype_none;
    } else if (strstr(term, "256color") || getenv("TMUX")) {
      return nanoclj_colortype_256;
    }

    /* CI platforms */
    if (getenv("CI") || getenv("TEAMCITY_VERSION")) {
      if (getenv("TRAVIS")) {
	return nanoclj_colortype_256;
      } else {
	return nanoclj_colortype_none;
      }
    } else {
      return nanoclj_colortype_16;
    }
  }
#endif
}

static inline bool has_sixels(FILE * in, FILE * out) {
#ifdef _WIN32
  return false;
#else
  return isatty(fileno(out));
#endif
}

#endif
