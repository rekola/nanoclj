#ifndef _NANOCLJ_TERM_H_
#define _NANOCLJ_TERM_H_

#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#include <io.h>

#define isatty _isatty
#define write _write

#endif /* _WIN32 */

#include <string.h>

#include "nanoclj_utils.h"
#include "nanoclj_types.h"

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

static inline int convert_to_256color(int r, int g, int b) {
  if ((r / 10) == (g / 10) && (g / 10) == (b / 10)) {
    r /= 10;
    if (r == 0) {
      return 0;
    } else if (r >= 25) {
      return 15;
    } else {
      return 232 + (r - 1);
    }
  } else {
    r /= 43;
    g /= 43;
    b /= 43;
    return 16 + r * 6 * 6 + g * 6 + b;
  }
}

static inline void set_term_fg_color(FILE * fh, double r0, double g0, double b0, nanoclj_colortype_t colors) {
  if (isatty(fileno(fh))) {
    int r = clamp((int)(r0 * 255), 0, 255);
    int g = clamp((int)(g0 * 255), 0, 255);
    int b = clamp((int)(b0 * 255), 0, 255);

    switch (colors) {
    case nanoclj_colortype_16:
      break;
    case nanoclj_colortype_256:
      fprintf(fh, "\033[38:5:%dm", convert_to_256color(r, g, b));    
      break;
    case nanoclj_colortype_true:
      fprintf(fh, "\033[38;2;%d;%d;%dm", r, g, b);
      break;
    }
  }
}

static inline void set_term_bg_color(FILE * fh, double r0, double g0, double b0, nanoclj_colortype_t colors) {
  if (isatty(fileno(fh))) {
    int r = clamp((int)(r0 * 255), 0, 255);
    int g = clamp((int)(g0 * 255), 0, 255);
    int b = clamp((int)(b0 * 255), 0, 255);

    switch (colors) {
    case nanoclj_colortype_16:
      break;
    case nanoclj_colortype_256:
      fprintf(fh, "\033[48:5:%dm", convert_to_256color(r, g, b));    
      break;
    case nanoclj_colortype_true:
      fprintf(fh, "\033[48;2;%d;%d;%dm", r, g, b);
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
  
  return orig_term;
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
    unsigned int i = 0;
    
    struct termios orig_term = set_raw_mode(fileno(in));
    
    if (write(fileno(out), "\033[14t", 5) != 5) {
      return false;
    }
    
    while (i < sizeof(buf)-1) {
      if (read(fileno(in), buf+i, 1) != 1) {
	break;
      }
      if (buf[i] == 't') {
	break;
      }
      i++;
    }
    buf[i] = '\0';
    
    if (buf[0] == 27 && buf[1] == '[' && buf[2] == '4' && buf[3] == ';' &&
	sscanf(buf + 4, "%d;%dt", height, width) == 2) {

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
    unsigned int i = 0;
    while (i < sizeof(buf)-1) {
      if (read(fileno(in), buf+i, 1) != 1) break;
      if (buf[i] == 'R') break;
      i++;
    }
    buf[i] = 0;
    
    bool r = false;
    if (buf[0] == 0x27 && buf[1] == '[' && sscanf(buf+2, "%d;%d", y, x) == 2) {
      r = true;
    }
    
    tcsetattr(STDIN_FILENO, TCSADRAIN, &orig_term);
    return r;
  } else {
    return false;
  }
}

static inline bool get_bg_color(FILE * in, FILE * out, double * r, double * g, double * b) {
  if (!isatty(fileno(out))) {
    return false;
  }

  char buf[32];
  unsigned int i = 0;
  
  struct termios orig_term = set_raw_mode(fileno(in));
  
  if (write(fileno(out), "\033]11;?\a", 7) != 7) {
    return false;
  }
  ms_sleep(100);
    
  // ^[]11;rgb:0000/0000/0000^
  while (i < sizeof(buf)-1) {
    if (read(fileno(in), buf+i, 1) != 1) {
      break;
    }
    if (buf[i] == '\\') {
      break;
    }
    i++;
  }
  buf[i] = '\0';
  tcsetattr(STDIN_FILENO, TCSADRAIN, &orig_term);
  
  fprintf(stderr, "color = %s, i = %d\n", buf, i);
  return false;
}

static inline nanoclj_colortype_t get_term_colortype() {
#ifdef _WIN32
  return nanoclj_colortype_16;
#else
  const char * colorterm = getenv("COLORTERM");
  const char * mlterm = getenv("MLTERM");
  if (mlterm ||
      (colorterm && (strcmp(colorterm, "truecolor") == 0 || strcmp(colorterm, "24bit") == 0))) {
    return nanoclj_colortype_true;
  } else {
    const char * term = getenv("TERM");
    if (term && strcmp(term, "xterm-256color") == 0) {
      return nanoclj_colortype_256;
    } else {
      return nanoclj_colortype_16;
    }
  }
#endif
}

static inline bool has_sixels() {
#ifdef _WIN32
  return false;
#else
  return true;
#endif
}

#endif
