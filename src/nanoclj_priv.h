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

#ifndef MAXFIL
#define MAXFIL 64
#endif

#ifndef GC_VERBOSE
#define GC_VERBOSE 0
#endif

#ifndef STRBUFF_INITIAL_SIZE
#define STRBUFF_INITIAL_SIZE 128
#endif
#ifndef STRBUFF_MAX_SIZE
#define STRBUFF_MAX_SIZE 65536
#endif
#ifndef AUXBUFF_SIZE
#define AUXBUFF_SIZE 256
#endif

#ifdef __cplusplus
extern "C" {
#endif

  enum nanoclj_port_kind {
    port_free = 0,
    port_file = 1,
    port_string = 2,
    port_callback = 8,
    port_input = 16,
    port_output = 32,
    port_saw_EOF = 64,
    port_binary = 128
  };

  typedef struct port {
    unsigned char kind;
    int backchar;
    union {
      struct {
        FILE *file;
        int closeit;
#if SHOW_ERROR_LINE
        int curr_line;
        char *filename;
#endif
      } stdio;
      struct {
        char *start;
        char *past_the_end;
        char *curr;
      } string;
      struct {
	void (*print) (const char *, size_t, void*);
	void (*color) (int r, int g, int b, void*);
	void (*reset_color) (void*);
      } callback;
    } rep;
  } port;

  typedef struct clj_image {
    int32_t width, height, channels;
    unsigned char * data;
  } clj_image;

/* cell structure */
  struct cell {
    unsigned short _flag;
    nanoclj_val_t _metadata;
    union {
      struct {
        char *_svalue;
        int _length;
      } _string;
      struct {
	char _svalue[15];
	char _length;
      } _small_string;
      struct {
	size_t _size;
	nanoclj_val_t * _data;
      } _collection;
      long long _lvalue;
      port *_port;
      clj_image * _image;
      struct {
	int _min_arity;
	int _max_arity;
	foreign_func _ptr;
      } _ff;
      struct {
	int _min_arity;
	int _max_arity;
	void * _ptr;
      } _fo;
      struct {
        nanoclj_val_t _car;
        nanoclj_val_t _cdr;
      } _cons;
      struct {
	struct cell * _origin;
	size_t _pos;
      } _seq;
    } _object;
  };

  struct nanoclj_s {
/* arrays for segments */
    func_alloc malloc;
    func_dealloc free;

/* return code */
    int retcode;
    int tracing;

    char **alloc_seg;
    nanoclj_val_t *cell_seg;
    int last_cell_seg;

/* We use 5 registers. */
    nanoclj_val_t args;               /* register for arguments of function */
    nanoclj_val_t envir;              /* stack register for current environment */
    nanoclj_val_t code;               /* register for current code */
    nanoclj_val_t dump;               /* stack register for next evaluation */
#ifdef USE_RECUR_REGISTER    
    nanoclj_val_t recur;		  /* recursion point */
#endif

/* Temporary registers for argument unpacking */
    nanoclj_val_t arg0;
    nanoclj_val_t arg1;
    nanoclj_val_t arg2;
    nanoclj_val_t arg_rest;
    
    struct cell _sink;
    nanoclj_val_t sink;               /* when mem. alloc. fails */
    struct cell _EMPTY;
    nanoclj_val_t EMPTY;              /* special cell representing empty list */
    struct cell * oblist;         /* pointer to symbol table */
    nanoclj_val_t global_env;         /* pointer to global environment */
    nanoclj_val_t target_env;
    nanoclj_val_t root_env;		/* pointer to the initial root env */
    nanoclj_val_t c_nest;             /* stack for nested calls from C */

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
    nanoclj_val_t ERROR_HOOK;         /* *error-hook* */
    nanoclj_val_t TAG_HOOK;           /* *default-data-reader-fn* */
    nanoclj_val_t COMPILE_HOOK;       /* *compile-hook* */
    nanoclj_val_t IN;		  /* *in* */
    nanoclj_val_t OUT;		  /* *out* */
    nanoclj_val_t ERR;		  /* *err* */
    nanoclj_val_t NS;		  /* *ns* */
    nanoclj_val_t RECUR;		  /* recur */
    nanoclj_val_t AMP;		  /* & */
    nanoclj_val_t UNDERSCORE;         /* _ */
    nanoclj_val_t DOC;		  /* :doc */
    
    nanoclj_val_t ANSI;		  /* :ansi */
    nanoclj_val_t RGB;		  /* :rgb */
    nanoclj_val_t WEIGHT;	  /* :weight */

    nanoclj_val_t SORTED_SET;	  /* sorted-set */
    nanoclj_val_t ARRAY_MAP;	  /* array-map */
    nanoclj_val_t REGEX;		  /* regex */
    nanoclj_val_t EMPTYSTR;		  /* "" */
    
    struct cell * free_cell;      /* pointer to top of free cells */
    long fcells;                  /* # of free cells */
    
    nanoclj_val_t save_inport;
    nanoclj_val_t loadport;

    port load_stack[MAXFIL];    /* Stack of open files for port -1 (LOADing) */
    int nesting_stack[MAXFIL];
    int file_i;
    int nesting;

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

    long gensym_cnt;

    struct nanoclj_interface *vptr;
    void *dump_base;            /* pointer to base of allocated dump stack */
    int dump_size;              /* number of frames allocated for dump stack */

    bool truecolor_term;
  };

/* operator code */
  enum nanoclj_opcodes {
#define _OP_DEF(A,B,C,OP) OP,
#include "nanoclj_opdf.h"
    OP_MAXDEFINED
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
  
  int is_proc(nanoclj_val_t p);
  int is_foreign(nanoclj_val_t p);
  const char *syntaxname(nanoclj_val_t p);
  int is_closure(nanoclj_val_t p);
#if 0
  int is_macro(nanoclj_val_t p);
#endif
  nanoclj_val_t closure_code(nanoclj_val_t p);
  nanoclj_val_t closure_env(nanoclj_val_t p);

  int is_delay(nanoclj_val_t p);
  int is_environment(nanoclj_val_t p);
#endif

#ifdef __cplusplus
}
#endif
#endif
