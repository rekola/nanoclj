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

static inline bool get_window_size(FILE * in, FILE * out, int * width, int * height) {
  bool r = false;
  if (isatty(fileno(out))) {
    struct winsize ws;
    
    if (ioctl(1, TIOCGWINSZ, &ws) != -1 && ws.ws_xpixel && ws.ws_ypixel) {
      *width = ws.ws_xpixel;
      *height = ws.ws_ypixel;
      return true;
    } else {
      char buf[32];
      unsigned int i = 0;

      struct termios orig_term = set_raw_mode(fileno(in));
      
      if (write(fileno(out), "\033[14t", 5) != 5) {
	return false;
      }

      while (i < sizeof(buf)-1) {
        if (read(fileno(in), buf+i, 1) != 1) {
	  fprintf(stderr, "read ended at %d\n", i);
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
	r = true;
      }

      tcsetattr(STDIN_FILENO, TCSADRAIN, &orig_term);
    }
  }
  return r;
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

static inline bool has_truecolor() {
#ifdef _WIN32
  return false;
#else
  const char * ev = getenv("COLORTERM");
  return ev && (strcmp(ev, "truecolor") == 0 || strcmp(ev, "24bit") == 0);
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
