#ifndef _NANOCLJ_PRIVATE_H
#define _NANOCLJ_PRIVATE_H

#include "nanoclj.h"

// Memory limits, by default enough
// for list of over 100k elements
// also controlled by env vars of the same name

#ifndef CELL_SEGSIZE
#define CELL_SEGSIZE    262144
#endif
#ifndef CELL_NSEGMENT
#define CELL_NSEGMENT   12
#endif

#define NANOCLJ_SMALL_VEC_SIZE 2
#define NANOCLJ_SMALL_STR_SIZE 24

#ifndef MAXFIL
#define MAXFIL 64
#endif

#ifndef GC_VERBOSE
#define GC_VERBOSE 0
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "nanoclj_types.h"

  struct pcre2_real_code_8;
	
  typedef enum {
    port_free = 0,
    port_file = 1,
    port_string = 2,
    port_callback = 3,
    port_canvas = 4
  } nanoclj_port_type_t;

  /* operator code */
  enum nanoclj_opcodes {
#define _OP_DEF(A,OP) OP,
#include "nanoclj_opdf.h"
    OP_MAXDEFINED
  };
  
  typedef struct nanoclj_byte_array_t {
    char *data;
    size_t size, reserved, refcnt;
  } nanoclj_byte_array_t;

  typedef struct nanoclj_vector_t {
    nanoclj_val_t * data;
    size_t size, reserved, refcnt;
    struct nanoclj_cell_t * meta;
  } nanoclj_vector_t;

  typedef struct nanoclj_float_array_t {
    float * data;
    size_t size, reserved, refcnt;
  } nanoclj_float_array_t;
  
  typedef struct {
    nanoclj_color_t fg;
    nanoclj_color_t bg;
  } nanoclj_term_state_t;
  
  typedef union {
    struct {
      FILE *file;
      char *filename;
      int32_t line, column;
      nanoclj_color_t fg;
      nanoclj_color_t bg;
      int32_t num_states;
      nanoclj_term_state_t states[256];
    } stdio;
    struct {
      char *curr;
      nanoclj_byte_array_t data;
    } string;
    struct {
      void (*text) (const char *, size_t, void*);
      void (*color) (double r, double g, double b, void*);
      void (*restore) (void*);
      void (*image) (nanoclj_image_t*, void*);
    } callback;
    struct {
      void * impl;
    } canvas;
  } nanoclj_port_rep_t;

/* cell structure */
  typedef struct nanoclj_cell_t {
    int32_t hasheq;
    uint16_t type;
    uint8_t flags;
    uint8_t so_size;    
    union {
      struct {
	char data[NANOCLJ_SMALL_STR_SIZE];
      } _small_byte_array;      
      struct {
	size_t offset, size;
	union {
	  nanoclj_vector_t * _vec;
	  nanoclj_byte_array_t * _str;
	} _store;
      } _collection;
      struct {
        nanoclj_val_t data[NANOCLJ_SMALL_VEC_SIZE];
	struct nanoclj_cell_t * meta;
      } _small_collection;
      struct {
	void * impl;
      } _tensor;
      long long _lvalue;
      struct pcre2_real_code_8 * _re;
      struct {
	nanoclj_port_rep_t * rep;
	int backchar[2];
	int nesting;
	uint8_t type, flags;
      } _port;
      nanoclj_image_t * _image;
      nanoclj_audio_t * _audio;
      struct {
	int min_arity, max_arity;
	foreign_func ptr;
      } _ff;
      struct {
	int min_arity, max_arity;
	void * ptr;
      } _fo;
      struct {
        nanoclj_val_t car, cdr;
	struct nanoclj_cell_t * meta;
      } _cons;
    } _object;
  } nanoclj_cell_t;

  /* this structure holds all the interpreter's registers */
  typedef struct dump_stack_frame_t {
    enum nanoclj_opcodes op;
    nanoclj_cell_t * args;
    nanoclj_cell_t * envir;
    nanoclj_val_t code;
#ifdef USE_RECUR_REGISTER
    nanoclj_val_t recur;
#endif
  } dump_stack_frame_t;

  struct nanoclj_s {
/* arrays for segments */
    func_alloc malloc;
    func_dealloc free;
    func_realloc realloc;

/* return code */
    int retcode;
    int tracing;

    char **alloc_seg;
    nanoclj_val_t *cell_seg;
    int last_cell_seg;

/* We use 5 registers. */
    nanoclj_cell_t * args;               /* register for arguments of function */
    nanoclj_cell_t * envir;              /* stack register for current environment */
    nanoclj_val_t code;               /* register for current code */
    size_t dump;               /* stack register for next evaluation */
#ifdef USE_RECUR_REGISTER    
    nanoclj_val_t recur;		  /* recursion point */
#endif
    
    nanoclj_cell_t _sink;
    nanoclj_val_t sink;               /* when mem. alloc. fails */
    nanoclj_cell_t _EMPTY;
    nanoclj_val_t EMPTY;              /* special cell representing empty list */
    nanoclj_cell_t * oblist;         /* pointer to symbol table */
    nanoclj_cell_t * global_env;         /* pointer to global environment */
    nanoclj_cell_t * root_env;		/* pointer to the initial root env */
    nanoclj_vector_t * types;

/* global pointers to special symbols */
    nanoclj_val_t LAMBDA;             /* pointer to syntax lambda */
    nanoclj_val_t DO;                 /* pointer to syntax do */
    nanoclj_val_t QUOTE;              /* pointer to syntax quote */
    nanoclj_val_t ARG1;		  /* first arg in literal functions */
    nanoclj_val_t ARG2;		  /* second arg in literal functions */
    nanoclj_val_t ARG3;		  /* third arg in literal functions */
    nanoclj_val_t ARG_REST;	  	  /* rest args in literal functions */
    
    nanoclj_val_t DEREF;		  /* pointer to symbol deref */
    nanoclj_val_t VAR;		  /* pointer to symbol var */
    nanoclj_val_t QQUOTE;             /* pointer to symbol quasiquote */
    nanoclj_val_t UNQUOTE;            /* pointer to symbol unquote */
    nanoclj_val_t UNQUOTESP;          /* pointer to symbol unquote-splicing */
    nanoclj_val_t SLASH_HOOK;         /* *slash-hook* */
    nanoclj_val_t TAG_HOOK;           /* *default-data-reader-fn* */
    nanoclj_val_t COMPILE_HOOK;       /* *compile-hook* */
    nanoclj_val_t IN;		  /* *in* */
    nanoclj_val_t OUT;		  /* *out* */
    nanoclj_val_t ERR;		  /* *err* */
    nanoclj_val_t NS_SYM;	  /* *ns* */
    nanoclj_val_t ENV;	  	  /* *env* */
    nanoclj_val_t WINDOW_SIZE;    /* *window-size* */
    nanoclj_val_t WINDOW_SCALE_F;   /* *window-scale-factor* */
    nanoclj_val_t MOUSE_POS;      /* *mouse-pos* */
    
    nanoclj_val_t RECUR;		  /* recur */
    nanoclj_val_t AMP;		  /* & */
    nanoclj_val_t UNDERSCORE;         /* _ */
    nanoclj_val_t DOC;		  /* :doc */
    nanoclj_val_t WIDTH;	  /* :width */
    nanoclj_val_t HEIGHT;	  /* :height */
    nanoclj_val_t CHANNELS;	  /* :channels */
    nanoclj_val_t WATCHES;	  /* :watches */
    nanoclj_val_t NAME;		  /* :name */
    nanoclj_val_t TYPE;		  /* :type */
    nanoclj_val_t LINE;		  /* :line */
    nanoclj_val_t COLUMN;	  /* :column */
    nanoclj_val_t FILE;		  /* :file */
    nanoclj_val_t NS_KEYWORD;	  /* :ns */
    
    nanoclj_val_t SORTED_SET;	  /* sorted-set */
    nanoclj_val_t ARRAY_MAP;	  /* array-map */
    nanoclj_cell_t * EMPTYVEC;
    
    nanoclj_cell_t * free_cell;      /* pointer to top of free cells */
    long fcells;                  /* # of free cells */
    
    nanoclj_val_t save_inport;

    nanoclj_val_t load_stack[MAXFIL];    /* Stack of open files for port -1 (LOADing) */
    int file_i;
    
    bool no_memory;             /* Whether mem. alloc. has failed */

    char *strbuff;
    size_t strbuff_size;

    char *errbuff;
    size_t errbuff_size;

    int tok;
    nanoclj_val_t value;
    int op;

    void *ext_data;             /* For the benefit of foreign functions */
    nanoclj_val_t (*object_invoke_callback) (nanoclj_t *, void *, nanoclj_val_t);

    size_t gensym_cnt, gentypeid_cnt;

    struct nanoclj_interface *vptr;
    dump_stack_frame_t * dump_base;            /* pointer to base of allocated dump stack */
    size_t dump_size;              /* number of frames allocated for dump stack */
        
    bool sixel_term;
    nanoclj_colortype_t term_colors;
    int window_lines, window_columns;
    double window_scale_factor;
    
    /* Dynamic printing */
    nanoclj_val_t active_element, active_element_target;
    int active_element_x, active_element_y;

    nanoclj_cell_t * properties;

    void * tensor_ctx;
  };

#if 0
  int is_string(nanoclj_val_t p);
  const char *string_value(nanoclj_val_t p);
  int is_number(nanoclj_val_t p);
  int64_t ivalue(nanoclj_val_t p);
  double rvalue(nanoclj_val_t p);
  int is_integer(nanoclj_val_t p);
  int is_real(nanoclj_val_t p);
  int is_character(nanoclj_val_t p);
  int is_vector(nanoclj_val_t p);
  int is_port(nanoclj_val_t p);
  int is_pair(nanoclj_val_t p);
  nanoclj_val_t pair_car(nanoclj_val_t p);
  nanoclj_val_t pair_cdr(nanoclj_val_t p);
  nanoclj_val_t set_car(nanoclj_val_t p, nanoclj_val_t q);
  nanoclj_val_t set_cdr(nanoclj_val_t p, nanoclj_val_t q);

  int is_symbol(nanoclj_val_t p);
  const char *symname(nanoclj_val_t p);

  int is_syntax(nanoclj_val_t p);
  
  const char *syntaxname(nanoclj_val_t p);
  int is_closure(nanoclj_val_t p);
  nanoclj_val_t closure_code(nanoclj_val_t p);
  nanoclj_val_t closure_env(nanoclj_val_t p);

  int is_environment(nanoclj_val_t p);
#endif

#ifdef __cplusplus
}
#endif
#endif

