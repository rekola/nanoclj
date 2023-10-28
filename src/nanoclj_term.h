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
#include "nanoclj_base64.h"

#if NANOCLJ_SIXEL
#include <sixel.h>

static int write_sixel_output(char *data, int size, void *priv) {
  return fwrite(data, 1, size, (FILE *)priv);
}

static inline bool write_sixel(int pf, const uint8_t * pal, const uint8_t * ptr, int width, int height) {
  sixel_dither_t * dither;
  if (pf == SIXEL_PIXELFORMAT_PAL8) {
    sixel_dither_new(&dither, 256, NULL);
    sixel_dither_set_palette(dither, (uint8_t*)pal);
    sixel_dither_set_pixelformat(dither, pf);
  } else {
    sixel_dither_new(&dither, -1, NULL);
    sixel_dither_initialize(dither, (uint8_t*)ptr, width, height, pf, SIXEL_LARGE_NORM, SIXEL_REP_CENTER_BOX, SIXEL_QUALITY_HIGHCOLOR);
  }
  
  sixel_output_t * output;
  sixel_output_new(&output, write_sixel_output, stdout, NULL);
  
  /* convert pixels into sixel format and write it to output context */
  sixel_encode((uint8_t*)ptr,
	       width,
	       height,
	       8,
	       dither,
	       output);

  sixel_output_destroy(output);
  sixel_dither_destroy(dither);
  
  return true;
}
  
static inline bool print_image_sixel(imageview_t iv, nanoclj_color_t fg, nanoclj_color_t bg) {
  switch (iv.format) {
  case nanoclj_r8:
    uint8_t pal[3 * 256];
    for (size_t i = 0; i < 256; i++) {
      nanoclj_color_t c = mix(bg, fg, i / 255.0f);
      pal[3 * i + 0] = c.red;
      pal[3 * i + 1] = c.green;
      pal[3 * i + 2] = c.blue;
    }
    return write_sixel(SIXEL_PIXELFORMAT_PAL8, pal, iv.ptr, iv.width, iv.height);
  case nanoclj_rgb8:
    return write_sixel(SIXEL_PIXELFORMAT_RGB888, NULL, iv.ptr, iv.width, iv.height);
  case nanoclj_rgba8:
    return write_sixel(SIXEL_PIXELFORMAT_RGBA8888, NULL, iv.ptr, iv.width, iv.height);
  case nanoclj_argb8:
    return write_sixel(SIXEL_PIXELFORMAT_BGRA8888, NULL, iv.ptr, iv.width, iv.height);
  case nanoclj_rgb8_32:{
    uint8_t * tmp = malloc(iv.width * iv.height * 3);
    for (size_t i = 0; i < iv.width * iv.height; i++) {
      tmp[3 * i + 0] = iv.ptr[4 * i + 0];
      tmp[3 * i + 1] = iv.ptr[4 * i + 1];
      tmp[3 * i + 2] = iv.ptr[4 * i + 2];
    }
    bool r = write_sixel(SIXEL_PIXELFORMAT_BGR888, NULL, tmp, iv.width, iv.height);
    free(tmp);
    return r;
  }
  default: return false;
  }
}
#endif

static inline void write_kitty(const uint8_t * ptr, int width, int height, int channels) {
  uint8_t * base64;
  size_t base64_len = base64_encode(ptr, width * height * channels, &base64);
  for (size_t i = 0; i < base64_len; i += 4096) {
    int m = i + 4096 < base64_len ? 1 : 0;
    if (i == 0) printf("\033_Ga=T,f=%d,s=%d,v=%d,m=%d;", channels * 8, width, height, m);
    else printf("\033_Gm=%d;", m);
    fwrite(base64 + i, i + 4096 < base64_len ? 4096 : base64_len - i, 1, stdout);
    printf("\033\\");
  }
  fflush(stdout);
  free(base64);
}

static inline bool print_image_kitty(imageview_t iv, nanoclj_color_t fg, nanoclj_color_t bg) {
  size_t n = iv.width * iv.height;
  switch (iv.format) {
  case nanoclj_r8:{
    uint8_t * ptr = malloc(3 * n);
    for (size_t i = 0; i < n; i++) {
      nanoclj_color_t c = mix(bg, fg, iv.ptr[i] / 255.0f);
      ptr[3 * i + 0] = c.red;
      ptr[3 * i + 1] = c.green;
      ptr[3 * i + 2] = c.blue;
    }
    write_kitty(ptr, iv.width, iv.height, 3);
    free(ptr);
    return true;
  }

  case nanoclj_rgb8:
  case nanoclj_rgba8:{
    int channels = get_format_channels(iv.format);
    uint8_t * transposed = malloc(n * channels);
    transpose_red_blue(iv.ptr, transposed, iv.width, iv.height, channels);
    write_kitty(transposed, iv.width, iv.height, channels);
    free(transposed);
  }
    return true;
  }
  return false;
}

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
  if (write(fileno(out), "\033]11;?\033\\", 8) == 8 && term_read_upto(fileno(in), buf, sizeof(buf), '\\')) {
    if (sscanf(buf, "\033]11;rgb:%2x/%2x/%2x", &r, &g, &b) == 3) {
      *bg = mk_color3i(r, g, b);
      has_bg = true;
    } else if (sscanf(buf, "\033]11;rgb:%x/%x/%x", &r, &g, &b) == 3) {
      *bg = mk_color3i(r >> 8, g >> 8, b >> 8);
      has_bg = true;
    }
  }

  bool has_fg = false;
  if (has_bg && write(fileno(out), "\033]10;?\033\\", 8) == 8 && term_read_upto(fileno(in), buf, sizeof(buf), '\\')) {
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

static inline nanoclj_graphics_t get_term_graphics(FILE * in, FILE * out) {
#ifndef _WIN32
  if (isatty(fileno(out))) {
    const char * term = getenv("TERM");    
    if (term && strcmp(term, "xterm-kitty") == 0) {
      return nanoclj_kitty;
    } else if (getenv("KONSOLE_VERSION")) {
      return nanoclj_kitty;
    } else {
#if NANOCLJ_SIXEL
      return nanoclj_sixel;
#endif
    }
  }
#endif
  return nanoclj_no_gfx;
}

#endif
