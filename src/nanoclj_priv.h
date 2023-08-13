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
	void (*func) (const char *, size_t, void*);
      } callback;
    } rep;
  } port;

  typedef struct clj_image {
    unsigned int width, height, channels;
    unsigned char * data;
  } clj_image;

/* cell structure */
  struct cell {
    unsigned short _flag;
    clj_value _metadata;
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
	clj_value * _data;
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
        clj_value _car;
        clj_value _cdr;
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
    clj_value *cell_seg;
    int last_cell_seg;

/* We use 5 registers. */
    clj_value args;               /* register for arguments of function */
    clj_value envir;              /* stack register for current environment */
    clj_value code;               /* register for current code */
    clj_value dump;               /* stack register for next evaluation */
#ifdef USE_RECUR_REGISTER    
    clj_value recur;		  /* recursion point */
#endif

    clj_value arg0;
    clj_value arg1;
    clj_value arg2;
    clj_value arg_rest;
    
    int interactive_repl;       /* are we in an interactive REPL? */

    struct cell _sink;
    clj_value sink;               /* when mem. alloc. fails */
    struct cell _EMPTY;
    clj_value EMPTY;              /* special cell representing empty list */
    struct cell * oblist;         /* pointer to symbol table */
    clj_value global_env;         /* pointer to global environment */
    clj_value target_env;
    clj_value root_env;		/* pointer to the initial root env */
    clj_value c_nest;             /* stack for nested calls from C */

/* global pointers to special symbols */
    clj_value LAMBDA;             /* pointer to syntax lambda */
    clj_value DO;                 /* pointer to syntax do */
    clj_value QUOTE;              /* pointer to syntax quote */
    clj_value ARG1;		  /* first arg in literal functions */
    clj_value ARG2;		  /* second arg in literal functions */
    clj_value ARG3;		  /* third arg in literal functions */
    clj_value ARG_REST;	  	  /* rest args in literal functions */
    
    clj_value DEREF;		  /* pointer to symbol deref */
    clj_value VAR;		  /* pointer to symbol var */
    clj_value QQUOTE;             /* pointer to symbol quasiquote */
    clj_value UNQUOTE;            /* pointer to symbol unquote */
    clj_value UNQUOTESP;          /* pointer to symbol unquote-splicing */
    clj_value SLASH_HOOK;         /* *slash-hook* */
    clj_value ERROR_HOOK;         /* *error-hook* */
    clj_value TAG_HOOK;           /* *default-data-reader-fn* */
    clj_value COMPILE_HOOK;       /* *compile-hook* */
    clj_value PRINT_HOOK;         /* *print-hook* */
    clj_value IN;		  /* *in* */
    clj_value OUT;		  /* *out* */
    clj_value ERR;		  /* *err* */
    clj_value VAL1;               /* *1 */
    clj_value NS;		  /* *ns* */
    clj_value RECUR;		  /* recur */
    clj_value AMP;		  /* & */
    clj_value UNDERSCORE;         /* _ */
    clj_value DOC;		  /* :doc */

    clj_value GREEN;		  /* :green */
    clj_value RED;		  /* :red */
    clj_value MAGENTA;            /* :magenta */
    clj_value BLUE;		  /* :blue */
    clj_value YELLOW;		  /* :yellow */
    clj_value BOLD;		  /* :bold */

    clj_value SORTED_SET;	  /* sorted-set */
    clj_value ARRAY_MAP;	  /* array-map */
    clj_value REGEX;		  /* regex */
    clj_value EMPTYSTR;		  /* "" */
    
    struct cell * free_cell;      /* pointer to top of free cells */
    long fcells;                  /* # of free cells */
    
    clj_value save_inport;
    clj_value loadport;

    port load_stack[MAXFIL];    /* Stack of open files for port -1 (LOADing) */
    int nesting_stack[MAXFIL];
    int file_i;
    int nesting;

    bool no_memory;             /* Whether mem. alloc. has failed */

    char *strbuff;
    size_t strbuff_size;

    char *errbuff;
    size_t errbuff_size;

    FILE *tmpfp;
    int tok;
    int print_flag;
    clj_value value;
    int op;

    void *ext_data;             /* For the benefit of foreign functions */
    clj_value (*object_invoke_callback) (nanoclj_t *, void *, clj_value);

    long gensym_cnt;

    struct nanoclj_interface *vptr;
    void *dump_base;            /* pointer to base of allocated dump stack */
    int dump_size;              /* number of frames allocated for dump stack */
  };

/* operator code */
  enum nanoclj_opcodes {
#define _OP_DEF(A,B,C,OP) OP,
#include "nanoclj_opdf.h"
    OP_MAXDEFINED
  };

#if 0
  int is_string(clj_value p);
  const char *string_value(clj_value p);
  int is_number(clj_value p);
  int64_t ivalue(clj_value p);
  double rvalue(clj_value p);
  int is_integer(clj_value p);
  int is_real(clj_value p);
  int is_character(clj_value p);
  int is_vector(clj_value p);
  int is_port(clj_value p);
  int is_pair(clj_value p);
  clj_value pair_car(clj_value p);
  clj_value pair_cdr(clj_value p);
  clj_value set_car(clj_value p, clj_value q);
  clj_value set_cdr(clj_value p, clj_value q);

  int is_symbol(clj_value p);
  const char *symname(clj_value p);

  int is_syntax(clj_value p);
  
  int is_proc(clj_value p);
  int is_foreign(clj_value p);
  const char *syntaxname(clj_value p);
  int is_closure(clj_value p);
#if 0
  int is_macro(clj_value p);
#endif
  clj_value closure_code(clj_value p);
  clj_value closure_env(clj_value p);

  int is_promise(clj_value p);
  int is_environment(clj_value p);
#if 0
  int is_immutable(clj_value p);
  void setimmutable(clj_value p);
#endif
#endif

#ifdef __cplusplus
}
#endif
#endif

