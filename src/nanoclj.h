#ifndef _NANOCLJ_H_
#define _NANOCLJ_H_

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
#define NANOCLJ_EXPORT
#else
#ifdef _NANOCLJ_SOURCE
#define NANOCLJ_EXPORT __declspec(dllexport)
#else
#define NANOCLJ_EXPORT __declspec(dllimport)
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

#ifndef STDIO_ADDS_CR           /* Define if DOS/Windows */
#define STDIO_ADDS_CR 0
#endif

#ifndef USE_INTERFACE
#define USE_INTERFACE 1
#endif

#ifndef SHOW_ERROR_LINE         /* Show error line in file */
#define SHOW_ERROR_LINE 1
#endif

  typedef struct nanoclj_s nanoclj_t;
  typedef union {
    uint64_t as_uint64;
    double as_double;
  } clj_value;
  
  typedef void *(*func_alloc) (size_t);
  typedef void (*func_dealloc) (void *);
  
  NANOCLJ_EXPORT nanoclj_t *nanoclj_init_new(void);
  NANOCLJ_EXPORT nanoclj_t *nanoclj_init_new_custom_alloc(func_alloc malloc,
      func_dealloc free);
  NANOCLJ_EXPORT int nanoclj_init(nanoclj_t * sc);
  NANOCLJ_EXPORT int nanoclj_init_custom_alloc(nanoclj_t * sc, func_alloc,
      func_dealloc);
  NANOCLJ_EXPORT void nanoclj_deinit(nanoclj_t * sc);
  void nanoclj_set_input_port_file(nanoclj_t * sc, FILE * fin);
  void nanoclj_set_input_port_string(nanoclj_t * sc, char *start,
      char *past_the_end);
  NANOCLJ_EXPORT void nanoclj_set_output_port_file(nanoclj_t * sc, FILE * fin);
  NANOCLJ_EXPORT void nanoclj_set_output_port_callback(nanoclj_t * sc, void (*func) (const char *, size_t, void *));
  NANOCLJ_EXPORT void nanoclj_set_error_port_callback(nanoclj_t * sc, void (*func) (const char *, size_t, void *));
  void nanoclj_set_output_port_string(nanoclj_t * sc, char *start,
      char *past_the_end);
  NANOCLJ_EXPORT void nanoclj_load_file(nanoclj_t * sc, FILE * fin);
  NANOCLJ_EXPORT void nanoclj_load_named_file(nanoclj_t * sc, FILE * fin,
      const char *filename);
  NANOCLJ_EXPORT clj_value nanoclj_eval_string(nanoclj_t * sc, const char *cmd);
  NANOCLJ_EXPORT void nanoclj_load_string(nanoclj_t * sc, const char *cmd);
  NANOCLJ_EXPORT clj_value nanoclj_apply0(nanoclj_t * sc, const char *procname);
  NANOCLJ_EXPORT clj_value nanoclj_call(nanoclj_t * sc, clj_value func, clj_value args);
  NANOCLJ_EXPORT clj_value nanoclj_eval(nanoclj_t * sc, clj_value obj);
  void nanoclj_set_external_data(nanoclj_t * sc, void *p);
  void nanoclj_set_object_invoke_callback(nanoclj_t * sc, clj_value (*func) (nanoclj_t *, void *, clj_value));
  NANOCLJ_EXPORT void nanoclj_define(nanoclj_t * sc, clj_value env, clj_value symbol,
      clj_value value);

  typedef clj_value(*foreign_func) (nanoclj_t *, clj_value);

#if 0
  clj_value _cons(nanoclj_t * sc, clj_value a, clj_value b, int immutable);
  clj_value mk_boolean(nanoclj_t * sc, int v);
  clj_value mk_integer(nanoclj_t * sc, long long num);
  clj_value mk_real(nanoclj_t * sc, double num);
  clj_value mk_symbol(nanoclj_t * sc, const char *name);
  clj_value gensym(nanoclj_t * sc);
  clj_value mk_string(nanoclj_t * sc, const char *str);
  clj_value mk_counted_string(nanoclj_t * sc, const char *str, int len);
  clj_value mk_character(nanoclj_t * sc, int c);
  clj_value mk_foreign_func(nanoclj_t * sc, foreign_func f);
  int list_length(nanoclj_t * sc, clj_value a);
#endif

#if USE_INTERFACE
  struct nanoclj_interface {
    void (*nanoclj_define) (nanoclj_t * sc, clj_value env, clj_value symbol,
			   clj_value value);
    clj_value(*cons) (nanoclj_t * sc, clj_value a, clj_value b);
    clj_value(*mk_integer) (nanoclj_t * sc, long long num);
    clj_value(*mk_real) (double num);
    clj_value(*mk_symbol) (nanoclj_t * sc, const char *name);
    clj_value(*mk_string) (nanoclj_t * sc, const char *str);
    clj_value(*mk_counted_string) (nanoclj_t * sc, const char *str, size_t len);
    clj_value(*mk_character) (int c);
    clj_value(*mk_vector) (nanoclj_t * sc, size_t len);
    clj_value(*mk_foreign_func) (nanoclj_t * sc, foreign_func f);
    clj_value(*mk_boolean) (bool b);
    
    bool (*is_string) (clj_value p);
    const char *(*string_value) (clj_value p);
    bool (*is_number) (clj_value p);
    long long (*to_long) (clj_value p);
    double (*to_double) (clj_value p);
    bool (*is_integer) (clj_value p);
    bool (*is_real) (clj_value p);
    bool (*is_character) (clj_value p);
    int (*to_int) (clj_value p);
    bool (*is_list) (clj_value p);
    bool (*is_vector) (clj_value p);
    size_t (*size) (nanoclj_t * sc, clj_value vec);
    void (*fill_vector) (clj_value vec, clj_value elem);
    clj_value(*vector_elem) (clj_value vec, size_t ielem);
    void(*set_vector_elem) (clj_value vec, size_t ielem, clj_value newel);
    bool (*is_reader) (clj_value p);
    bool (*is_writer) (clj_value p);
    
    bool (*is_pair) (clj_value p);
    clj_value(*pair_car) (clj_value p);
    clj_value(*pair_cdr) (clj_value p);

    clj_value (*first)(nanoclj_t * sc, clj_value coll);
    bool (*is_empty)(nanoclj_t * sc, clj_value coll);
    clj_value (*rest)(nanoclj_t * sc, clj_value coll);

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
    bool (*is_mapentry) (clj_value p);
#if 0
    clj_value(*closure_code) (clj_value p);
    clj_value(*closure_env) (clj_value p);
#endif
    bool (*is_promise) (clj_value p);
    bool (*is_environment) (clj_value p);
    void (*load_file) (nanoclj_t * sc, FILE * fin);
    void (*load_string) (nanoclj_t * sc, const char *input);
    clj_value (*eval_string) (nanoclj_t * sc, const char *input);
  };
#endif
  
#if !STANDALONE
  typedef struct nanoclj_registerable {
    foreign_func f;
    const char *name;
  } nanoclj_registerable;
  
  void nanoclj_register_foreign_func_list(nanoclj_t * sc,
					  nanoclj_registerable * list, int n);

#endif                          /* !STANDALONE */

#ifdef __cplusplus
}
#endif
#endif

