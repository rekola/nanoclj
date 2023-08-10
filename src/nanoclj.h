#ifndef _SCHEME_H_
#define _SCHEME_H_

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define NICE_PORTS

#ifdef __cplusplus
extern "C" {
#endif

// #define USE_RECUR_REGISTER

/*
 * Default values for #define'd symbols
 */
#ifndef NANOCLOJURE_STANDALONE              /* If used as standalone interpreter */
#define NANOCLOJURE_STANDALONE 0
#endif

#ifndef _MSC_VER
#define SCHEME_EXPORT
#else
#ifdef _SCHEME_SOURCE
#define SCHEME_EXPORT __declspec(dllexport)
#else
#define SCHEME_EXPORT __declspec(dllimport)
#endif
#endif

#if USE_DL
#define USE_INTERFACE 1
#endif

#ifndef USE_LINENOISE
#define USE_LINENOISE 1
#endif
  
#ifndef USE_TRACING
#define USE_TRACING 0
#endif

/* To force system errors through user-defined error handling (see *error-hook*) */
#ifndef USE_ERROR_HOOK
#define USE_ERROR_HOOK 1
#endif

#ifndef USE_SLASH_HOOK          /* Enable qualified qualifier */
#define USE_SLASH_HOOK 1
#endif

#ifndef STDIO_ADDS_CR           /* Define if DOS/Windows */
#define STDIO_ADDS_CR 0
#endif

#ifndef USE_INTERFACE
#define USE_INTERFACE 1
#endif

#ifndef SHOW_ERROR_LINE         /* Show error line in file */
#define SHOW_ERROR_LINE 1
#endif

  typedef struct scheme scheme;
  typedef union {
    uint64_t as_uint64;
    double as_double;
  } clj_value;
  
  typedef void *(*func_alloc) (size_t);
  typedef void (*func_dealloc) (void *);
  
  SCHEME_EXPORT scheme *scheme_init_new(void);
  SCHEME_EXPORT scheme *scheme_init_new_custom_alloc(func_alloc malloc,
      func_dealloc free);
  SCHEME_EXPORT int scheme_init(scheme * sc);
  SCHEME_EXPORT int scheme_init_custom_alloc(scheme * sc, func_alloc,
      func_dealloc);
  SCHEME_EXPORT void scheme_deinit(scheme * sc);
  void scheme_set_input_port_file(scheme * sc, FILE * fin);
  void scheme_set_input_port_string(scheme * sc, char *start,
      char *past_the_end);
  SCHEME_EXPORT void scheme_set_output_port_file(scheme * sc, FILE * fin);
  SCHEME_EXPORT void scheme_set_output_port_callback(scheme * sc, void (*func) (const char *, size_t, void *));
  SCHEME_EXPORT void scheme_set_error_port_callback(scheme * sc, void (*func) (const char *, size_t, void *));
  void scheme_set_output_port_string(scheme * sc, char *start,
      char *past_the_end);
  SCHEME_EXPORT void scheme_load_file(scheme * sc, FILE * fin);
  SCHEME_EXPORT void scheme_load_named_file(scheme * sc, FILE * fin,
      const char *filename);
  SCHEME_EXPORT clj_value scheme_eval_string(scheme * sc, const char *cmd);
  SCHEME_EXPORT void scheme_load_string(scheme * sc, const char *cmd);
  SCHEME_EXPORT clj_value scheme_apply0(scheme * sc, const char *procname);
  SCHEME_EXPORT clj_value scheme_call(scheme * sc, clj_value func, clj_value args);
  SCHEME_EXPORT clj_value scheme_eval(scheme * sc, clj_value obj);
  void scheme_set_external_data(scheme * sc, void *p);
  void scheme_set_object_invoke_callback(scheme * sc, clj_value (*func) (scheme *, void *, clj_value));
  SCHEME_EXPORT void scheme_define(scheme * sc, clj_value env, clj_value symbol,
      clj_value value);

  typedef clj_value(*foreign_func) (scheme *, clj_value);

#if 0
  clj_value _cons(scheme * sc, clj_value a, clj_value b, int immutable);
  clj_value mk_boolean(scheme * sc, int v);
  clj_value mk_integer(scheme * sc, long long num);
  clj_value mk_real(scheme * sc, double num);
  clj_value mk_symbol(scheme * sc, const char *name);
  clj_value gensym(scheme * sc);
  clj_value mk_string(scheme * sc, const char *str);
  clj_value mk_counted_string(scheme * sc, const char *str, int len);
  clj_value mk_character(scheme * sc, int c);
  clj_value mk_foreign_func(scheme * sc, foreign_func f);
  int list_length(scheme * sc, clj_value a);
#endif

#if USE_INTERFACE
  struct scheme_interface {
    void (*scheme_define) (scheme * sc, clj_value env, clj_value symbol,
			   clj_value value);
    clj_value(*cons) (scheme * sc, clj_value a, clj_value b);
    clj_value(*immutable_cons) (scheme * sc, clj_value a, clj_value b);
    clj_value(*mk_integer) (scheme * sc, long long num);
    clj_value(*mk_real) (double num);
    clj_value(*mk_symbol) (scheme * sc, const char *name);
    clj_value(*gensym) (scheme * sc);
    clj_value(*mk_string) (scheme * sc, const char *str);
    clj_value(*mk_counted_string) (scheme * sc, const char *str, size_t len);
    clj_value(*mk_character) (int c);
    clj_value(*mk_vector) (scheme * sc, size_t len);
    clj_value(*mk_foreign_func) (scheme * sc, foreign_func f);
    
    bool (*is_string) (clj_value p);
    const char *(*string_value) (clj_value p);
    bool (*is_number) (clj_value p);
    long long (*to_long) (clj_value p);
    double (*to_double) (clj_value p);
    bool (*is_integer) (clj_value p);
    bool (*is_real) (clj_value p);
    bool (*is_character) (clj_value p);
    int (*to_int) (clj_value p);
    bool (*is_list) (scheme * sc, clj_value p);
    bool (*is_vector) (clj_value p);
    size_t (*list_length) (scheme * sc, clj_value vec);
    long long (*vector_length) (clj_value vec);
    void (*fill_vector) (clj_value vec, clj_value elem);
    clj_value(*vector_elem) (clj_value vec, size_t ielem);
    void(*set_vector_elem) (clj_value vec, size_t ielem, clj_value newel);
#if 0
    bool (*is_port) (clj_value p);
#endif
    
    bool (*is_pair) (clj_value p);
    clj_value(*pair_car) (clj_value p);
    clj_value(*pair_cdr) (clj_value p);

    clj_value (*first)(scheme * sc, clj_value coll);
    bool (*is_empty)(scheme * sc, clj_value coll);
    clj_value (*rest)(scheme * sc, clj_value coll);

    bool (*is_symbol) (clj_value p);
    bool (*is_keyword) (clj_value p);
    const char *(*symname) (clj_value p);

#if 0
    int (*is_syntax) (clj_value p);
#endif
    bool (*is_proc) (clj_value p);
    bool (*is_foreign) (clj_value p);
    bool (*is_closure) (clj_value p);
    bool (*is_macro) (clj_value p);
#if 0
    clj_value(*closure_code) (clj_value p);
    clj_value(*closure_env) (clj_value p);
#endif
    bool (*is_promise) (clj_value p);
    bool (*is_environment) (clj_value p);
    void (*load_file) (scheme * sc, FILE * fin);
    void (*load_string) (scheme * sc, const char *input);
    clj_value (*eval_string) (scheme * sc, const char *input);
  };
#endif
  
#if !STANDALONE
  typedef struct scheme_registerable {
    foreign_func f;
    const char *name;
  } scheme_registerable;
  
  void scheme_register_foreign_func_list(scheme * sc,
					 scheme_registerable * list, int n);

#endif                          /* !STANDALONE */

#ifdef __cplusplus
}
#endif
#endif

