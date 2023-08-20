#ifndef _NANOCLJ_FUNCTIONS_H_
#define _NANOCLJ_FUNCTIONS_H_

#include <sys/utsname.h>
#include <stdint.h>
#include <pwd.h>
#include <unistd.h>
#include <glob.h>

#include "linenoise.h"

#ifdef WIN32
#include <direct.h>
#define getcwd _getcwd
#define environ _environ
#endif

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_RESIZE_STATIC
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"

/* Thread */

static nanoclj_val_t Thread_sleep(nanoclj_t * sc, nanoclj_val_t args) {
  long long ms = to_long(car(args));
  if (ms > 0) {
    struct timespec t, t2;
    t.tv_sec = ms / 1000;
    t.tv_nsec = (ms % 1000) * 1000000;
    nanosleep(&t, &t2);
  }
  return mk_nil();
}

/* System */

static nanoclj_val_t System_exit(nanoclj_t * sc, nanoclj_val_t args) {
  exit(to_int(car(args)));
}

static nanoclj_val_t System_currentTimeMillis(nanoclj_t * sc, nanoclj_val_t args) {
  return mk_long(sc, system_time() / 1000);
}

static nanoclj_val_t System_nanoTime(nanoclj_t * sc, nanoclj_val_t args) {
  return mk_long(sc, 1000 * system_time());
}

static nanoclj_val_t System_gc(nanoclj_t * sc, nanoclj_val_t args) {
  gc(sc, sc->EMPTY, sc->EMPTY);
  return (nanoclj_val_t)kTRUE;
}

static nanoclj_val_t System_getenv(nanoclj_t * sc, nanoclj_val_t args) {
  if (args.as_uint64 != sc->EMPTY.as_uint64) {
    const char * v = getenv(strvalue(car(args)));
    return v ? mk_string(sc, v) : mk_nil();
  } else {
    extern char **environ;
    
    int l = 0;
    for ( ; environ[l]; l++) { }
    struct cell * map = mk_arraymap(sc, l);
    for (int i = 0; i < l; i++) {
      char * p = environ[i];
      int j = 0;
      for (; p[j] != '=' && p[j] != 0; j++) {
	
      }
      nanoclj_val_t key = mk_counted_string(sc, p, j);
      nanoclj_val_t val = p[j] == '=' ? mk_string(sc, p + j + 1) : mk_nil();
      set_vector_elem(map, i, cons(sc, key, val));
    }

    return mk_pointer(map);
  }
}

static nanoclj_val_t System_getProperty(nanoclj_t * sc, nanoclj_val_t args) {
  const char *user_home = NULL, *user_name = NULL;
  const char *os_name = NULL, *os_version = NULL, *os_arch = NULL;

#ifndef _WIN32
  struct utsname buf;
  if (uname(&buf) == 0) {
    os_name = buf.sysname;
    os_version = buf.version;
    os_arch = buf.machine;
  }
  struct passwd pwd;
  struct passwd *result;
    
  size_t pw_bufsize = sysconf(_SC_GETPW_R_SIZE_MAX);
  if (pw_bufsize == -1) pw_bufsize = 16384;
  
  char * pw_buf = sc->malloc(pw_bufsize);
  if (!pw_buf) {
    return mk_nil();
  }
   
  getpwuid_r(getuid(), &pwd, pw_buf, pw_bufsize, &result);
  if (result) {
    user_name = result->pw_name;
    user_home = result->pw_dir;
  }
#endif

  char cwd_buf[FILENAME_MAX];
  const char * user_dir = getcwd(cwd_buf, FILENAME_MAX);

#ifdef _WIN32
  const char * file_separator = "\\";
  const char * path_separator = ";";
  const char * line_separator = "\r\n";
#else
  const char * file_separator = "/";
  const char * path_separator = ":";
  const char * line_separator = "\n";
#endif

  nanoclj_val_t r;
  if (args.as_uint64 == sc->EMPTY.as_uint64) {
    nanoclj_val_t l = sc->EMPTY;
    l = cons(sc, mk_string(sc, user_name), l);
    l = cons(sc, mk_string(sc, "user.name"), l);
    l = cons(sc, mk_string(sc, line_separator), l);
    l = cons(sc, mk_string(sc, "line.separator"), l);
    l = cons(sc, mk_string(sc, file_separator), l);
    l = cons(sc, mk_string(sc, "file.separator"), l);
    l = cons(sc, mk_string(sc, path_separator), l);
    l = cons(sc, mk_string(sc, "path.separator"), l);
    l = cons(sc, mk_string(sc, user_home), l);
    l = cons(sc, mk_string(sc, "user.home"), l);
    l = cons(sc, mk_string(sc, user_dir), l);
    l = cons(sc, mk_string(sc, "user.dir"), l);
    l = cons(sc, mk_string(sc, os_name), l);
    l = cons(sc, mk_string(sc, "os.name"), l);
    l = cons(sc, mk_string(sc, os_version), l);
    l = cons(sc, mk_string(sc, "os.version"), l);
    l = cons(sc, mk_string(sc, os_arch), l);
    l = cons(sc, mk_string(sc, "os.arch"), l);
    r = mk_collection(sc, T_ARRAYMAP, l);
  } else {
    const char * pname = strvalue(car(args));
    if (strcmp(pname, "os.name") == 0) {
      r = mk_string(sc, os_name);
    } else if (strcmp(pname, "os.version") == 0) {
      r = mk_string(sc, os_version);
    } else if (strcmp(pname, "os.arch") == 0) {
      r = mk_string(sc, os_arch);
    } else if (strcmp(pname, "user.home") == 0) {
      r = mk_string(sc, user_home);
    } else if (strcmp(pname, "user.dir") == 0) {
      r = mk_string(sc, user_dir);
    } else if (strcmp(pname, "user.name") == 0) {
      r = mk_string(sc, user_name);
    } else if (strcmp(pname, "path.separator") == 0) {
      r = mk_string(sc, path_separator);
    } else if (strcmp(pname, "file.separator") == 0) {
      r = mk_string(sc, file_separator);
    } else if (strcmp(pname, "line.separator") == 0) {
      r = mk_string(sc, line_separator);
    } else {
      r = mk_nil();
    }
  }

  sc->free(pw_buf);

  return r;
}

static inline nanoclj_val_t System_glob(nanoclj_t * sc, nanoclj_val_t args) {
  const char * pattern = to_cstr(car(args));

  glob_t gstruct;
  int r = glob(pattern, GLOB_ERR, NULL, &gstruct);
  nanoclj_val_t x = sc->EMPTY;
  if (r == 0) {
    for (char ** found = gstruct.gl_pathv; *found; found++) {
      x = cons(sc, mk_string(sc, *found), x);
    }
  }
  globfree(&gstruct);
  
  if (r != 0 && r != GLOB_NOMATCH) {
    return mk_nil();
  } else {
    return x;
  }
}

/* Math */

static nanoclj_val_t Math_sin(nanoclj_t * sc, nanoclj_val_t args) {
  return mk_real(sin(to_double(car(args))));
}

static nanoclj_val_t Math_cos(nanoclj_t * sc, nanoclj_val_t args) {
  return mk_real(cos(to_double(car(args))));
}

static nanoclj_val_t Math_exp(nanoclj_t * sc, nanoclj_val_t args) {
  return mk_real(exp(to_double(car(args))));
}

static nanoclj_val_t Math_log(nanoclj_t * sc, nanoclj_val_t args) {
  return mk_real(log(to_double(car(args))));
}

static nanoclj_val_t Math_log10(nanoclj_t * sc, nanoclj_val_t args) {
  return mk_real(log10(to_double(car(args))));
}

static nanoclj_val_t Math_tan(nanoclj_t * sc, nanoclj_val_t args) {
  return mk_real(tan(to_double(car(args))));
}

static nanoclj_val_t Math_asin(nanoclj_t * sc, nanoclj_val_t args) {
  return mk_real(asin(to_double(car(args))));
}

static nanoclj_val_t Math_acos(nanoclj_t * sc, nanoclj_val_t args) {
  return mk_real(acos(to_double(car(args))));
}

static nanoclj_val_t Math_atan(nanoclj_t * sc, nanoclj_val_t args) {
  if (cdr(args).as_uint64 != sc->EMPTY.as_uint64) {
    nanoclj_val_t x = car(sc->args);
    nanoclj_val_t y = cadr(sc->args);
    return mk_real(atan2(to_double(x), to_double(y)));
  } else {
    nanoclj_val_t x = car(args);
    return mk_real(atan(to_double(x)));
  }
}

static nanoclj_val_t Math_sqrt(nanoclj_t * sc, nanoclj_val_t args) {
  return mk_real(sqrt(to_double(car(args))));
}

static nanoclj_val_t Math_cbrt(nanoclj_t * sc, nanoclj_val_t args) {
  return mk_real(cbrt(to_double(car(args))));
}

static nanoclj_val_t Math_pow(nanoclj_t * sc, nanoclj_val_t args) {
  return mk_real(pow(to_double(car(sc->args)), to_double(cadr(sc->args))));
}

static nanoclj_val_t Math_floor(nanoclj_t * sc, nanoclj_val_t args) {
  return mk_real(floor(to_double(car(args))));
}

static nanoclj_val_t Math_ceil(nanoclj_t * sc, nanoclj_val_t args) {
  return mk_real(ceil(to_double(car(args))));
}

static nanoclj_val_t Math_round(nanoclj_t * sc, nanoclj_val_t args) {
  return mk_real(round(to_double(car(args))));
}

/* clojure.math.numeric-tower */

#define SQRT_INT64_MAX (INT64_C(0xB504F333))

static inline bool ipow_overflow(long long base, int_fast8_t exp, long long * res) {
  if (base == 1) {
    *res = 1;
    return false;
  }
  if (!exp) {
    *res = 1;
    return false;
  }
  if (!base) {
    *res = 0;
    return false;
  }

  long long result = 1;
  if (exp & 1) result *= base;
  exp >>= 1;
  while (exp) {
    if (base > SQRT_INT64_MAX) return true;
    base *= base;
    if (exp & 1) result *= base;
    exp >>= 1;
  }

  *res = result;
  return false;
}

static nanoclj_val_t numeric_tower_expt(nanoclj_t * sc, nanoclj_val_t args) {
  nanoclj_val_t x = car(sc->args);
  nanoclj_val_t y = cadr(sc->args);

  int tx = prim_type(x), ty = prim_type(y);

  if (tx == T_INTEGER && ty == T_INTEGER) {
    int base = decode_integer(x), exp = decode_integer(y);
    long long res;
    if (base == 0 || base == 1) {
      return mk_int(base);
    } else if (exp >= 0 && exp < 256 && !ipow_overflow(base, exp, &res)) {
      return mk_integer(sc, res);
    } else if (exp > -256 && exp < 0 && !ipow_overflow(base, -exp, &res)) {
      return mk_ratio_long(sc, 1, res);
    }
  } else if (tx != T_REAL && ty != T_REAL) {
    tx = expand_type(x, tx);
    ty = expand_type(y, ty);

    if (tx == T_RATIO && (ty == T_INTEGER || ty == T_LONG)) {
      long exp = to_long(y);
      if (exp > -256 && exp < 256) {
	long long den;
	long long num = get_ratio(x, &den);
	if (exp < 0) {
	  exp = -exp;
	  long long tmp = den;
	  den = num;
	  num = tmp;
	}
	long long num2, den2;
	if (!ipow_overflow(num, exp, &num2) && !ipow_overflow(den, exp, &den2)) {
	  if (den2 == 1) return mk_integer(sc, num2);
	  else return mk_ratio_long(sc, num2, den2);
	}
      }
    } else if (tx != T_RATIO && ty != T_RATIO) {
      long long base = to_long(x), exp = to_long(y);
      long long res;
      if (base == 0 || base == 1) {
	return mk_integer(sc, base);
      } else if (exp >= 0 && exp < 256 && !ipow_overflow(base, exp, &res)) {
	return mk_integer(sc, res);
      } else if (exp > -256 && exp < 0 && !ipow_overflow(base, -exp, &res)) {
	return mk_ratio_long(sc, 1, res);
      }
    }
  }

  return mk_real(pow(to_double(x), to_double(y)));
}

static inline nanoclj_val_t browse_url(nanoclj_t * sc, nanoclj_val_t args) {
  const char * url = to_cstr(car(args));
#ifdef WIN32
  ShellExecute(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
  return (nanoclj_val_t)kTRUE;
#else
  pid_t r = fork();
  if (r == 0) {
    const char * cmd = "xdg-open";
    execlp(cmd, cmd, url, NULL);
    exit(1);
  } else {
    return r < 0 ? (nanoclj_val_t)kFALSE : (nanoclj_val_t)kTRUE;
  }
#endif
}

static inline nanoclj_val_t shell_sh(nanoclj_t * sc, nanoclj_val_t args0) {
  struct cell * args = decode_pointer(args0);
  size_t n = seq_length(sc, args);
  if (n >= 1) {
#ifdef WIN32
    /* TODO */
    return mk_nil();
#else
    pid_t r = fork();
    if (r == 0) {
      const char * cmd = to_cstr(first(sc, args));
      args = rest(sc, args);
      n--;
      char ** output_args = (char **)sc->malloc(n * sizeof(char*));
      for (size_t i = 0; i < n; i++, args = rest(sc, args)) {
	output_args[i] = to_cstr(first(sc, args));
      }
      execvp(cmd, output_args);
      exit(1);
    } else {
      return r < 0 ? (nanoclj_val_t)kFALSE : (nanoclj_val_t)kTRUE;
    }
  } else {
    return mk_nil();
  }
#endif
}

static inline nanoclj_val_t Image_load(nanoclj_t * sc, nanoclj_val_t args) {
  const char * filename = to_cstr(car(args));
	    
  int w, h, channels;
  unsigned char * data = stbi_load(filename, &w, &h, &channels, 0);
  if (!data) {
    return mk_nil();
  }
  
  nanoclj_val_t image = mk_image(sc, w, h, channels, data);
  
  stbi_image_free(data);

  return image;
}

static inline nanoclj_val_t Image_resize(nanoclj_t * sc, nanoclj_val_t args) {
  nanoclj_val_t image00 = car(args);
  nanoclj_val_t target_width0 = cadr(args), target_height0 = caddr(args);
  if (!is_image(image00) || !is_number(target_width0) || !is_number(target_height0)) {
    return mk_nil();
  }
  
  int target_width = to_int(target_width0), target_height = to_int(target_height0);  
  struct cell * image0 = decode_pointer(image00);
  nanoclj_image_t * image = _image_unchecked(image0);
  
  size_t target_size = target_width * target_height * image->channels;

  unsigned char * tmp = (unsigned char *)sc->malloc(target_size);
  
  stbir_resize_uint8(image->data, image->width, image->height, 0, tmp, target_width, target_height, 0, image->channels);

  nanoclj_val_t target_image = mk_image(sc, target_width, target_height, image->channels, tmp);

  sc->free(tmp);
  
  return target_image;
}

#if NANOCLJ_USE_LINENOISE
#define MAX_COMPLETION_SYMBOLS 65535

static nanoclj_t * linenoise_sc = NULL;
static int num_completion_symbols = 0;
static const char * completion_symbols[MAX_COMPLETION_SYMBOLS];
 
static inline void completion(const char *input, linenoiseCompletions *lc) {
  if (!num_completion_symbols) {
    const char * expr = "(ns-interns *ns*)";
    nanoclj_val_t sym = nanoclj_eval_string(linenoise_sc, expr, strlen(expr));
    for ( ; sym.as_uint64 != linenoise_sc->EMPTY.as_uint64 && num_completion_symbols < MAX_COMPLETION_SYMBOLS; sym = cdr(sym)) {
      nanoclj_val_t v = car(sym);
      if (is_symbol(v)) {
	completion_symbols[num_completion_symbols++] = symname(v);
      } else if (is_mapentry(v)) {
	completion_symbols[num_completion_symbols++] = symname(car(v));
      }
    }
  }

  int i, n = strlen(input);
  for (i = n; i > 0 && !(isspace(input[i - 1]) ||
			 input[i - 1] == '(' ||
			 input[i - 1] == '[' ||
			 input[i - 1] == '{'); i--) { }
  char buffer[1024];
  int prefix_len = 0;
  if (i > 0) {
    prefix_len = i;
    strncpy(buffer, input, i);
    buffer[prefix_len] = 0;
    input += i;
    n -= i;
  }
  for (i = 0; i < num_completion_symbols; i++) {
    const char * s = completion_symbols[i];
    if (strncmp(input, s, n) == 0) {
      strncpy(buffer + prefix_len, s, 1024 - prefix_len);
      buffer[1023] = 0;
      linenoiseAddCompletion(lc, buffer);
    }
  }
}

static inline char *complete_parens(const char * input) {
  char nest[1024];
  int si = 0;
  for (int i = 0, n = strlen(input); i < n; i++) {
    if (input[i] == '(') {
      nest[si++] = ')';
    } else if (input[i] == '[') {
      nest[si++] = ']';
    } else if (input[i] == '{') {
      nest[si++] = '}';
    } else if (input[i] == '"' && (si == 0 || nest[si - 1] != '"')) {
      nest[si++] = '"';
    } else if (input[i] == ')' || input[i] == ']' || input[i] == '}' || input[i] == '"') {
      if (si > 0 && nest[si - 1] == input[i]) {
	si--;
      } else {
	return NULL;
      }
    } else if (input[i] == '\\' && i + 1 < n && input[i + 1] == '"') {
      i++;
    }
  }
  if (si == 0) {
    return NULL;
  }
  char * h = (char *)linenoise_sc->malloc(si);
  int hi = 0;
  for (int i = si - 1; i >= 0; i--) {
    h[hi++] = nest[i];
  }
  h[hi] = 0;

  return h;
}
 
static inline char *hints(const char *input, int *color, int *bold) {
  char * h = complete_parens(input);
  if (!h) return NULL;
  
  *color = 35;
  *bold = 0;
  return h;  
}

static inline void free_hints(void * ptr) {
  linenoise_sc->free(ptr);
}

static inline nanoclj_val_t linenoise_readline(nanoclj_t * sc, nanoclj_val_t args) {
  if (!linenoise_sc) {
    linenoise_sc = sc;

    linenoiseSetupSigWinchHandler();
    linenoiseSetMultiLine(0);
    linenoiseSetCompletionCallback(completion);
    linenoiseSetHintsCallback(hints);
    linenoiseSetFreeHintsCallback(free_hints);
    linenoiseHistorySetMaxLen(10000);
    linenoiseHistoryLoad(".nanoclj_history");
  }
  char * line = linenoise(to_cstr(car(args)));
  if (line == NULL) return mk_nil();
  
  linenoiseHistoryAdd(line);
  linenoiseHistorySave(".nanoclj_history");

  nanoclj_val_t r;
  char * missing_parens = complete_parens(line);
  if (!missing_parens) {
    r = mk_string(sc, line);
  } else {
    int l1 = strlen(line), l2 = strlen(missing_parens);
    char * tmp = (char *)sc->malloc(l1 + l2 + 1);
    strcpy(tmp, line);
    strcpy(tmp + l1, missing_parens);
    sc->free(missing_parens);
    r = mk_string(sc, tmp);
    sc->free(tmp);
  }
  sc->free(line);

  return r;
}
#endif

static inline void register_functions(nanoclj_t * sc) {
  nanoclj_val_t Thread = def_namespace(sc, "Thread");
  nanoclj_val_t System = def_namespace(sc, "System");
  nanoclj_val_t Math = def_namespace(sc, "Math");
  nanoclj_val_t numeric_tower = def_namespace(sc, "clojure.math.numeric-tower");
  nanoclj_val_t clojure_java_browse = def_namespace(sc, "clojure.java.browse");
  nanoclj_val_t clojure_java_shell = def_namespace(sc, "clojure.java.shell");
  nanoclj_val_t Image = def_namespace(sc, "Image");
  
  nanoclj_intern(sc, Thread, def_symbol(sc, "sleep"), mk_foreign_func_with_arity(sc, Thread_sleep, 1, 1));
  
  nanoclj_intern(sc, System, def_symbol(sc, "exit"), mk_foreign_func_with_arity(sc, System_exit, 1, 1));
  nanoclj_intern(sc, System, def_symbol(sc, "currentTimeMillis"), mk_foreign_func_with_arity(sc, System_currentTimeMillis, 0, 0));
  nanoclj_intern(sc, System, def_symbol(sc, "nanoTime"), mk_foreign_func_with_arity(sc, System_nanoTime, 0, 0));
  nanoclj_intern(sc, System, def_symbol(sc, "gc"), mk_foreign_func_with_arity(sc, System_gc, 0, 0));
  nanoclj_intern(sc, System, def_symbol(sc, "getenv"), mk_foreign_func_with_arity(sc, System_getenv, 0, 1));
  nanoclj_intern(sc, System, def_symbol(sc, "getProperty"), mk_foreign_func_with_arity(sc, System_getProperty, 0, 1));
  nanoclj_intern(sc, System, def_symbol(sc, "glob"), mk_foreign_func_with_arity(sc, System_glob, 1, 1));
  
  nanoclj_intern(sc, Math, def_symbol(sc, "sin"), mk_foreign_func_with_arity(sc, Math_sin, 1, 1));
  nanoclj_intern(sc, Math, def_symbol(sc, "cos"), mk_foreign_func_with_arity(sc, Math_cos, 1, 1));
  nanoclj_intern(sc, Math, def_symbol(sc, "exp"), mk_foreign_func_with_arity(sc, Math_exp, 1, 1));
  nanoclj_intern(sc, Math, def_symbol(sc, "log"), mk_foreign_func_with_arity(sc, Math_log, 1, 1));
  nanoclj_intern(sc, Math, def_symbol(sc, "log10"), mk_foreign_func_with_arity(sc, Math_log10, 1, 1));
  nanoclj_intern(sc, Math, def_symbol(sc, "tan"), mk_foreign_func_with_arity(sc, Math_tan, 1, 1));
  nanoclj_intern(sc, Math, def_symbol(sc, "asin"), mk_foreign_func_with_arity(sc, Math_asin, 1, 1));
  nanoclj_intern(sc, Math, def_symbol(sc, "acos"), mk_foreign_func_with_arity(sc, Math_acos, 1, 1));
  nanoclj_intern(sc, Math, def_symbol(sc, "atan"), mk_foreign_func_with_arity(sc, Math_atan, 1, 2));
  nanoclj_intern(sc, Math, def_symbol(sc, "sqrt"), mk_foreign_func_with_arity(sc, Math_sqrt, 1, 1));
  nanoclj_intern(sc, Math, def_symbol(sc, "cbrt"), mk_foreign_func_with_arity(sc, Math_cbrt, 1, 1));
  nanoclj_intern(sc, Math, def_symbol(sc, "pow"), mk_foreign_func_with_arity(sc, Math_pow, 2, 2));
  nanoclj_intern(sc, Math, def_symbol(sc, "floor"), mk_foreign_func_with_arity(sc, Math_floor, 1, 1));
  nanoclj_intern(sc, Math, def_symbol(sc, "ceil"), mk_foreign_func_with_arity(sc, Math_ceil, 1, 1));
  nanoclj_intern(sc, Math, def_symbol(sc, "round"), mk_foreign_func_with_arity(sc, Math_round, 1, 1));

  nanoclj_intern(sc, Math, def_symbol(sc, "E"), mk_real(M_E));
  nanoclj_intern(sc, Math, def_symbol(sc, "PI"), mk_real(M_PI));
  nanoclj_intern(sc, Math, def_symbol(sc, "SQRT2"), mk_real(M_SQRT2));

  nanoclj_intern(sc, numeric_tower, def_symbol(sc, "expt"), mk_foreign_func_with_arity(sc, numeric_tower_expt, 2, 2));

  nanoclj_intern(sc, clojure_java_browse, def_symbol(sc, "browse-url"), mk_foreign_func_with_arity(sc, browse_url, 1, 1));

  nanoclj_intern(sc, clojure_java_shell, def_symbol(sc, "sh"), mk_foreign_func(sc, shell_sh));

  nanoclj_intern(sc, Image, def_symbol(sc, "load"), mk_foreign_func_with_arity(sc, Image_load, 1, 1));
  nanoclj_intern(sc, Image, def_symbol(sc, "resize"), mk_foreign_func_with_arity(sc, Image_resize, 3, 3));
  
#if NANOCLJ_USE_LINENOISE
  nanoclj_val_t linenoise = def_namespace(sc, "linenoise");
  nanoclj_intern(sc, linenoise, def_symbol(sc, "read-line"), mk_foreign_func_with_arity(sc, linenoise_readline, 1, 1));
#endif  
}

#endif
