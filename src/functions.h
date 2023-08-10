#ifndef _FUNCTIONS_H_
#define _FUNCTIONS_H_

#include <sys/utsname.h>
#include <stdint.h>
#include <pwd.h>
#include <unistd.h>

#ifdef WIN32
#include <direct.h>
#define getcwd _getcwd
#endif

/* Thread */

static clj_value thread_sleep(scheme * sc, clj_value args) {
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

static clj_value system_exit(scheme * sc, clj_value args) {
  exit(to_int(car(args)));
}

static clj_value system_currentTimeMillis(scheme * sc, clj_value args) {
  return mk_long(sc, system_time() / 1000);
}

static clj_value system_nanoTime(scheme * sc, clj_value args) {
  return mk_long(sc, 1000 * system_time());
}

static clj_value system_gc(scheme * sc, clj_value args) {
  gc(sc, sc->EMPTY, sc->EMPTY);
  return sc->T;
}

static clj_value system_getenv(scheme * sc, clj_value args) {
  if (args.as_uint64 != sc->EMPTY.as_uint64) {
    const char * v = getenv(strvalue(car(args)));
    return v ? mk_string(sc, v) : mk_nil();
  } else {
    extern char **environ;
    
    int l = 0;
    for ( ; environ[l]; l++) { }
    struct cell * map = get_vector_object(sc, l);
    _typeflag(map) = (T_ARRAYMAP | T_ATOM);
    for (int i = 0; i < l; i++) {
      char * p = environ[i];
      int j = 0;
      for (; p[j] != '=' && p[j] != 0; j++) {
	
      }
      clj_value key = mk_counted_string(sc, p, j);
      clj_value val = p[j] == '=' ? mk_string(sc, p + j + 1) : mk_nil();
      set_vector_elem(map, i, cons(sc, key, val));
    }

    return mk_pointer(map);
  }
}

static clj_value system_getProperty(scheme * sc, clj_value args) {
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
  
  char * pw_buf = malloc(pw_bufsize);
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

  clj_value r;
  if (args.as_uint64 == sc->EMPTY.as_uint64) {
    clj_value l = sc->EMPTY;
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

  free(pw_buf);

  return r;
}

/* Math */

static clj_value math_sin(scheme * sc, clj_value args) {
  return mk_real(sin(to_double(car(args))));
}

static clj_value math_cos(scheme * sc, clj_value args) {
  return mk_real(cos(to_double(car(args))));
}

static clj_value math_exp(scheme * sc, clj_value args) {
  return mk_real(exp(to_double(car(args))));
}

static clj_value math_log(scheme * sc, clj_value args) {
  return mk_real(log(to_double(car(args))));
}

static clj_value math_log10(scheme * sc, clj_value args) {
  return mk_real(log10(to_double(car(args))));
}

static clj_value math_tan(scheme * sc, clj_value args) {
  return mk_real(tan(to_double(car(args))));
}

static clj_value math_asin(scheme * sc, clj_value args) {
  return mk_real(asin(to_double(car(args))));
}

static clj_value math_acos(scheme * sc, clj_value args) {
  return mk_real(acos(to_double(car(args))));
}

static clj_value math_atan(scheme * sc, clj_value args) {
  if (cdr(args).as_uint64 != sc->EMPTY.as_uint64) {
    clj_value x = car(sc->args);
    clj_value y = cadr(sc->args);
    return mk_real(atan2(to_double(x), to_double(y)));
  } else {
    clj_value x = car(args);
    return mk_real(atan(to_double(x)));
  }
}

static clj_value math_sqrt(scheme * sc, clj_value args) {
  return mk_real(sqrt(to_double(car(args))));
}

static clj_value math_cbrt(scheme * sc, clj_value args) {
  return mk_real(cbrt(to_double(car(args))));
}

static clj_value math_pow(scheme * sc, clj_value args) {
  return mk_real(pow(to_double(car(sc->args)), to_double(cadr(sc->args))));
}

static clj_value math_floor(scheme * sc, clj_value args) {
  return mk_real(floor(to_double(car(args))));
}

static clj_value math_ceil(scheme * sc, clj_value args) {
  return mk_real(ceil(to_double(car(args))));
}

static clj_value math_round(scheme * sc, clj_value args) {
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

static clj_value numeric_tower_expt(scheme * sc, clj_value args) {
  clj_value x = car(sc->args);
  clj_value y = cadr(sc->args);

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

static inline clj_value browse_url(scheme * sc, clj_value args) {
  const char * url = to_cstr(car(args));
#ifdef WIN32
  ShellExecute(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
  return sc->T;
#else
  pid_t r = fork();
  if (r == 0) {
    const char * cmd = "xdg-open";
    execlp(cmd, cmd, url, NULL);
    exit(1);
  } else {
    return r < 0 ? sc->F : sc->T;
  }
#endif
}

static inline void register_functions(scheme * sc) {
  clj_value Thread = def_namespace(sc, "Thread");
  clj_value System = def_namespace(sc, "System");
  clj_value Math = def_namespace(sc, "Math");
  clj_value numeric_tower = def_namespace(sc, "clojure.math.numeric-tower");
  clj_value clojure_java_browse = def_namespace(sc, "clojure.java.browse");
  
  scheme_define(sc, Thread, def_symbol(sc, "sleep"), mk_foreign_func_with_arity(sc, thread_sleep, 1, 1));
  
  scheme_define(sc, System, def_symbol(sc, "exit"), mk_foreign_func_with_arity(sc, system_exit, 1, 1));
  scheme_define(sc, System, def_symbol(sc, "currentTimeMillis"), mk_foreign_func_with_arity(sc, system_currentTimeMillis, 0, 0));
  scheme_define(sc, System, def_symbol(sc, "nanoTime"), mk_foreign_func_with_arity(sc, system_nanoTime, 0, 0));
  scheme_define(sc, System, def_symbol(sc, "gc"), mk_foreign_func_with_arity(sc, system_gc, 0, 0));
  scheme_define(sc, System, def_symbol(sc, "getenv"), mk_foreign_func_with_arity(sc, system_getenv, 0, 1));
  scheme_define(sc, System, def_symbol(sc, "getProperty"), mk_foreign_func_with_arity(sc, system_getProperty, 0, 1));
  
  scheme_define(sc, Math, def_symbol(sc, "sin"), mk_foreign_func_with_arity(sc, math_sin, 1, 1));
  scheme_define(sc, Math, def_symbol(sc, "cos"), mk_foreign_func_with_arity(sc, math_cos, 1, 1));
  scheme_define(sc, Math, def_symbol(sc, "exp"), mk_foreign_func_with_arity(sc, math_exp, 1, 1));
  scheme_define(sc, Math, def_symbol(sc, "log"), mk_foreign_func_with_arity(sc, math_log, 1, 1));
  scheme_define(sc, Math, def_symbol(sc, "log10"), mk_foreign_func_with_arity(sc, math_log10, 1, 1));
  scheme_define(sc, Math, def_symbol(sc, "tan"), mk_foreign_func_with_arity(sc, math_tan, 1, 1));
  scheme_define(sc, Math, def_symbol(sc, "asin"), mk_foreign_func_with_arity(sc, math_asin, 1, 1));
  scheme_define(sc, Math, def_symbol(sc, "acos"), mk_foreign_func_with_arity(sc, math_acos, 1, 1));
  scheme_define(sc, Math, def_symbol(sc, "atan"), mk_foreign_func_with_arity(sc, math_atan, 1, 2));
  scheme_define(sc, Math, def_symbol(sc, "sqrt"), mk_foreign_func_with_arity(sc, math_sqrt, 1, 1));
  scheme_define(sc, Math, def_symbol(sc, "cbrt"), mk_foreign_func_with_arity(sc, math_cbrt, 1, 1));
  scheme_define(sc, Math, def_symbol(sc, "pow"), mk_foreign_func_with_arity(sc, math_pow, 2, 2));
  scheme_define(sc, Math, def_symbol(sc, "floor"), mk_foreign_func_with_arity(sc, math_floor, 1, 1));
  scheme_define(sc, Math, def_symbol(sc, "ceil"), mk_foreign_func_with_arity(sc, math_ceil, 1, 1));
  scheme_define(sc, Math, def_symbol(sc, "round"), mk_foreign_func_with_arity(sc, math_round, 1, 1));

  scheme_define(sc, Math, def_symbol(sc, "E"), mk_real(M_E));
  scheme_define(sc, Math, def_symbol(sc, "PI"), mk_real(M_PI));
  scheme_define(sc, Math, def_symbol(sc, "SQRT2"), mk_real(M_SQRT2));

  scheme_define(sc, numeric_tower, def_symbol(sc, "expt"), mk_foreign_func_with_arity(sc, numeric_tower_expt, 2, 2));

  scheme_define(sc, clojure_java_browse, def_symbol(sc, "browse-url"), mk_foreign_func_with_arity(sc, browse_url, 1, 1));
}

#endif
