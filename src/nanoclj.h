#ifndef _NANOCLJ_H_
#define _NANOCLJ_H_

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* #define USE_RECUR_REGISTER */

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

#ifndef NANOCLJ_USE_LINENOISE
#define NANOCLJ_USE_LINENOISE 0
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
  } nanoclj_val_t;
  
  typedef void *(*func_alloc) (size_t);
  typedef void (*func_dealloc) (void *);
  
  NANOCLJ_EXPORT nanoclj_t *nanoclj_init_new(void);
  NANOCLJ_EXPORT nanoclj_t *nanoclj_init_new_custom_alloc(func_alloc malloc, func_dealloc free);
  NANOCLJ_EXPORT int nanoclj_init(nanoclj_t * sc);
  NANOCLJ_EXPORT int nanoclj_init_custom_alloc(nanoclj_t * sc, func_alloc, func_dealloc);
  NANOCLJ_EXPORT void nanoclj_deinit(nanoclj_t * sc);
  void nanoclj_set_input_port_file(nanoclj_t * sc, FILE * fin);
  void nanoclj_set_input_port_string(nanoclj_t * sc, char *start, char *past_the_end);
  NANOCLJ_EXPORT void nanoclj_set_output_port_file(nanoclj_t * sc, FILE * fin);
  NANOCLJ_EXPORT void nanoclj_set_output_port_callback(nanoclj_t * sc, void (*func) (const char *, size_t, void *));
  NANOCLJ_EXPORT void nanoclj_set_error_port_callback(nanoclj_t * sc, void (*func) (const char *, size_t, void *));
  void nanoclj_set_output_port_string(nanoclj_t * sc, char *start, char *past_the_end);
  NANOCLJ_EXPORT void nanoclj_load_file(nanoclj_t * sc, FILE * fin);
  NANOCLJ_EXPORT void nanoclj_load_named_file(nanoclj_t * sc, FILE * fin, const char *filename);
  NANOCLJ_EXPORT nanoclj_val_t nanoclj_eval_string(nanoclj_t * sc, const char *cmd);
  NANOCLJ_EXPORT void nanoclj_load_string(nanoclj_t * sc, const char *cmd);
  NANOCLJ_EXPORT nanoclj_val_t nanoclj_apply0(nanoclj_t * sc, const char *procname);
  NANOCLJ_EXPORT nanoclj_val_t nanoclj_call(nanoclj_t * sc, nanoclj_val_t func, nanoclj_val_t args);
  NANOCLJ_EXPORT nanoclj_val_t nanoclj_eval(nanoclj_t * sc, nanoclj_val_t obj);
  void nanoclj_set_external_data(nanoclj_t * sc, void *p);
  void nanoclj_set_object_invoke_callback(nanoclj_t * sc, nanoclj_val_t (*func) (nanoclj_t *, void *, nanoclj_val_t));
  NANOCLJ_EXPORT void nanoclj_intern(nanoclj_t * sc, nanoclj_val_t ns, nanoclj_val_t symbol, nanoclj_val_t value);

  typedef nanoclj_val_t(*foreign_func) (nanoclj_t *, nanoclj_val_t);

#if 0
  nanoclj_val_t _cons(nanoclj_t * sc, nanoclj_val_t a, nanoclj_val_t b, int immutable);
  nanoclj_val_t mk_boolean(nanoclj_t * sc, int v);
  nanoclj_val_t mk_integer(nanoclj_t * sc, long long num);
  nanoclj_val_t mk_real(nanoclj_t * sc, double num);
  nanoclj_val_t mk_symbol(nanoclj_t * sc, const char *name);
  nanoclj_val_t gensym(nanoclj_t * sc);
  nanoclj_val_t mk_string(nanoclj_t * sc, const char *str);
  nanoclj_val_t mk_counted_string(nanoclj_t * sc, const char *str, int len);
  nanoclj_val_t mk_character(nanoclj_t * sc, int c);
  nanoclj_val_t mk_foreign_func(nanoclj_t * sc, foreign_func f);
  int list_length(nanoclj_t * sc, nanoclj_val_t a);
#endif

#if USE_INTERFACE
  struct nanoclj_interface {
    void (*nanoclj_intern) (nanoclj_t * sc, nanoclj_val_t ns, nanoclj_val_t symbol, nanoclj_val_t value);
    nanoclj_val_t(*cons) (nanoclj_t * sc, nanoclj_val_t a, nanoclj_val_t b);
    nanoclj_val_t(*mk_integer) (nanoclj_t * sc, long long num);
    nanoclj_val_t(*mk_real) (double num);
    nanoclj_val_t(*mk_symbol) (nanoclj_t * sc, const char *name);
    nanoclj_val_t(*mk_string) (nanoclj_t * sc, const char *str);
    nanoclj_val_t(*mk_counted_string) (nanoclj_t * sc, const char *str, size_t len);
    nanoclj_val_t(*mk_character) (int c);
    nanoclj_val_t(*mk_vector) (nanoclj_t * sc, size_t len);
    nanoclj_val_t(*mk_foreign_func) (nanoclj_t * sc, foreign_func f);
    nanoclj_val_t(*mk_boolean) (bool b);
    
    bool (*is_string) (nanoclj_val_t p);
    const char *(*string_value) (nanoclj_val_t p);
    bool (*is_number) (nanoclj_val_t p);
    long long (*to_long) (nanoclj_val_t p);
    double (*to_double) (nanoclj_val_t p);
    bool (*is_integer) (nanoclj_val_t p);
    bool (*is_real) (nanoclj_val_t p);
    bool (*is_character) (nanoclj_val_t p);
    int (*to_int) (nanoclj_val_t p);
    bool (*is_list) (nanoclj_val_t p);
    bool (*is_vector) (nanoclj_val_t p);
    size_t (*size) (nanoclj_t * sc, nanoclj_val_t vec);
    void (*fill_vector) (nanoclj_val_t vec, nanoclj_val_t elem);
    nanoclj_val_t(*vector_elem) (nanoclj_val_t vec, size_t ielem);
    void(*set_vector_elem) (nanoclj_val_t vec, size_t ielem, nanoclj_val_t newel);
    bool (*is_reader) (nanoclj_val_t p);
    bool (*is_writer) (nanoclj_val_t p);
    
    bool (*is_pair) (nanoclj_val_t p);
    nanoclj_val_t(*pair_car) (nanoclj_val_t p);
    nanoclj_val_t(*pair_cdr) (nanoclj_val_t p);

    nanoclj_val_t (*first)(nanoclj_t * sc, nanoclj_val_t coll);
    bool (*is_empty)(nanoclj_t * sc, nanoclj_val_t coll);
    nanoclj_val_t (*rest)(nanoclj_t * sc, nanoclj_val_t coll);

    bool (*is_symbol) (nanoclj_val_t p);
    bool (*is_keyword) (nanoclj_val_t p);
    const char *(*symname) (nanoclj_val_t p);

#if 0
    int (*is_syntax) (nanoclj_val_t p);
#endif
    bool (*is_proc) (nanoclj_val_t p);
    bool (*is_foreign) (nanoclj_val_t p);
    bool (*is_closure) (nanoclj_val_t p);
    bool (*is_macro) (nanoclj_val_t p);
    bool (*is_mapentry) (nanoclj_val_t p);
#if 0
    nanoclj_val_t(*closure_code) (nanoclj_val_t p);
    nanoclj_val_t(*closure_env) (nanoclj_val_t p);
#endif
    bool (*is_promise) (nanoclj_val_t p);
    bool (*is_environment) (nanoclj_val_t p);
    void (*load_file) (nanoclj_t * sc, FILE * fin);
    void (*load_string) (nanoclj_t * sc, const char *input);
    nanoclj_val_t (*eval_string) (nanoclj_t * sc, const char *input);
    nanoclj_val_t (*def_symbol) (nanoclj_t * sc, const char *name);
  };
#endif
  
#if !STANDALONE
  typedef struct nanoclj_registerable {
    foreign_func f;
    const char *name;
  } nanoclj_registerable;
  
  void nanoclj_register_foreign_func_list(nanoclj_t * sc, nanoclj_registerable * list, int n);

#endif                          /* !STANDALONE */

#ifdef __cplusplus
}
#endif
#endif

