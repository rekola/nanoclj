#ifndef _NANOCLJ_H_
#define _NANOCLJ_H_

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "nanoclj_types.h"
  
/* #define USE_RECUR_REGISTER */

/*
 * Default values for #define'd symbols
 */
#ifndef NANOCLJ_STANDALONE              /* If used as standalone interpreter */
#define NANOCLJ_STANDALONE 0
#endif

#ifndef NANOCLJ_SIXEL
#define NANOCLJ_SIXEL NANOCLJ_STANDALONE
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

#ifndef STDIO_ADDS_CR           /* Define if DOS/Windows */
#define STDIO_ADDS_CR 0
#endif

#ifndef USE_INTERFACE
#define USE_INTERFACE 1
#endif

  typedef struct nanoclj_s nanoclj_t;
  typedef union {
    uint64_t as_long;
    double as_double;
  } nanoclj_val_t;
  
  typedef void *(*func_alloc) (size_t);
  typedef void (*func_dealloc) (void *);
  typedef void *(*func_realloc) (void *, size_t);
  
  NANOCLJ_EXPORT nanoclj_t *nanoclj_init_new(void);
  NANOCLJ_EXPORT nanoclj_t *nanoclj_init_new_custom_alloc(func_alloc malloc, func_dealloc free, func_realloc realloc);
  NANOCLJ_EXPORT bool nanoclj_init(nanoclj_t * sc);
  NANOCLJ_EXPORT bool nanoclj_init_custom_alloc(nanoclj_t * sc, func_alloc, func_dealloc, func_realloc);
  NANOCLJ_EXPORT void nanoclj_deinit(nanoclj_t * sc);
  void nanoclj_set_input_port_file(nanoclj_t * sc, FILE * fin);
  NANOCLJ_EXPORT void nanoclj_set_output_port_file(nanoclj_t * sc, FILE * fin);
  NANOCLJ_EXPORT void nanoclj_set_output_port_callback(nanoclj_t * sc,
						       void (*text) (const char*, size_t, void*),
						       void (*color) (double, double, double, void*),
						       void (*restore) (void*),
						       void (*image) (nanoclj_image_t*, void*));
  NANOCLJ_EXPORT void nanoclj_set_error_port_callback(nanoclj_t * sc, void (*text) (const char *, size_t, void *));
  NANOCLJ_EXPORT bool nanoclj_load_named_file(nanoclj_t * sc, const char *filename);
  NANOCLJ_EXPORT nanoclj_val_t nanoclj_eval_string(nanoclj_t * sc, const char *cmd, size_t len);
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
  nanoclj_val_t mk_codepoint(nanoclj_t * sc, int c);
  nanoclj_val_t mk_foreign_func(nanoclj_t * sc, foreign_func f);
  int list_length(nanoclj_t * sc, nanoclj_val_t a);
#endif

#if USE_INTERFACE
  struct nanoclj_interface {
    void (*nanoclj_intern) (nanoclj_t * sc, nanoclj_val_t ns, nanoclj_val_t symbol, nanoclj_val_t value);
    nanoclj_val_t (*cons) (nanoclj_t * sc, nanoclj_val_t head, nanoclj_val_t tail);
    nanoclj_val_t (*mk_integer) (nanoclj_t * sc, long long num);
    nanoclj_val_t (*mk_real) (double num);
    nanoclj_val_t (*mk_symbol) (nanoclj_t * sc, const char *name);
    nanoclj_val_t (*mk_string) (nanoclj_t * sc, const char *str);
    nanoclj_val_t (*mk_counted_string) (nanoclj_t * sc, const char *str, size_t len);
    nanoclj_val_t (*mk_codepoint) (int c);
    nanoclj_val_t (*mk_vector) (nanoclj_t * sc, size_t len);
    nanoclj_val_t (*mk_foreign_func) (nanoclj_t * sc, foreign_func f);
    nanoclj_val_t (*mk_boolean) (bool b);
    
    bool (*is_string) (nanoclj_val_t p);
    const char *(*string_value) (nanoclj_val_t p);
    bool (*is_number) (nanoclj_val_t p);
    long long (*to_long) (nanoclj_val_t p);
    double (*to_double) (nanoclj_val_t p);
    int (*to_int) (nanoclj_val_t p);
    bool (*is_vector) (nanoclj_val_t p);
    size_t (*size) (nanoclj_t * sc, nanoclj_val_t vec);
    void (*fill_vector) (nanoclj_val_t vec, nanoclj_val_t elem);
    nanoclj_val_t (*vector_elem) (nanoclj_val_t vec, size_t ielem);
    void (*set_vector_elem) (nanoclj_val_t vec, size_t ielem, nanoclj_val_t newel);
    
    bool (*is_list) (nanoclj_val_t p);
    nanoclj_val_t (*pair_car) (nanoclj_val_t p);
    nanoclj_val_t (*pair_cdr) (nanoclj_val_t p);

    nanoclj_val_t (*first)(nanoclj_t * sc, nanoclj_val_t coll);
    bool (*is_empty)(nanoclj_t * sc, nanoclj_val_t coll);
    nanoclj_val_t (*rest)(nanoclj_t * sc, nanoclj_val_t coll);

    bool (*is_symbol) (nanoclj_val_t p);
    bool (*is_keyword) (nanoclj_val_t p);

    bool (*is_closure) (nanoclj_val_t p);
    bool (*is_macro) (nanoclj_val_t p);
    bool (*is_mapentry) (nanoclj_val_t p);
    bool (*is_environment) (nanoclj_val_t p);
    nanoclj_val_t (*eval_string) (nanoclj_t * sc, const char *input, size_t len);
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
