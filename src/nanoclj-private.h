#ifndef _NANOCLJ_PRIVATE_H
#define _NANOCLJ_PRIVATE_H

#include "nanoclj.h"

#define NANOCLJ_SMALL_VEC_SIZE 3
#define NANOCLJ_SMALL_STR_SIZE 24
#define NANOCLJ_SMALL_UINT_VEC_SIZE 6
#define NANOCLJ_ARRAYMAP_LIMIT 8

#define STRBUFFSIZE 256

#include <zlib.h>
#include <stdatomic.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "nanoclj_types.h"
#include "nanoclj_threads.h"
  
  typedef enum {
    port_free = 0,
    port_file,
    port_string,
    port_callback,
    port_canvas,
    port_z
  } nanoclj_port_type_t;
  
  /* operator code */
  enum nanoclj_opcode {
#define _OP_DEF(A,B,OP) OP,
#include "nanoclj-op.h"
    OP_MAXDEFINED
  };
  
  typedef struct {
    nanoclj_display_mode_t mode;
    nanoclj_color_t fg;
    nanoclj_color_t bg;
  } nanoclj_term_state_t;
  
  typedef union {
    struct {
      FILE *file;
      char *filename;
      atomic_int *rc;
      nanoclj_display_mode_t mode;
      nanoclj_color_t fg;
      nanoclj_color_t bg;
      int32_t num_states;
      nanoclj_term_state_t states[256];
      int32_t backchars[2];
    } stdio;
    struct {
      size_t read_pos;
      nanoclj_tensor_t * data;
    } string;
    struct {
      void (*text) (const char *, size_t, void*);
      void (*color) (double r, double g, double b, void*);
      void (*restore) (void*);
      void (*image) (imageview_t, void*);
    } callback;
    struct {
      z_stream strm;
    } z;
    struct {
      void * impl;
      nanoclj_tensor_t * data;
    } canvas;
  } nanoclj_port_rep_t;

/* cell structure */
  struct nanoclj_cell_t {
    uint32_t hasheq;
    uint16_t type;
    uint16_t flags;
    union {
      union {
	uint8_t bytes[NANOCLJ_SMALL_STR_SIZE];
	nanoclj_val_t vals[NANOCLJ_SMALL_VEC_SIZE];
	long long longs[NANOCLJ_SMALL_VEC_SIZE];
	uint32_t uints[NANOCLJ_SMALL_UINT_VEC_SIZE];
      } _small_tensor;
      struct {
	nanoclj_tensor_t * numerator;
	nanoclj_tensor_t * denominator;
      } _ratio;
      struct {
	nanoclj_tensor_t * tensor;
	int32_t offset, size;
	nanoclj_cell_t * meta;
      } _collection;
      struct {
	nanoclj_tensor_t * tensor;
	struct nanoclj_cell_t * meta;
      } _image;
      struct {
	nanoclj_tensor_t * tensor;
	uint32_t sample_rate;
	uint32_t frame_offset, frame_count;
	uint8_t channel_offset, channel_count;
      } _audio;
      struct {
	nanoclj_port_rep_t * rep;
	int nesting;
	uint8_t type, flags;
	int32_t line, column;
      } _port;
      struct {
	nanoclj_graph_array_t * rep;
	uint32_t num_nodes, num_edges;
	uint32_t node_offset, edge_offset;
      } _graph;
      struct {
	int min_arity, max_arity;
	foreign_func ptr;
	struct nanoclj_cell_t * meta;
      } _ff;
      struct {
        nanoclj_val_t car;
	nanoclj_cell_t * cdr;
	union {
	  struct nanoclj_cell_t * meta;
	  nanoclj_val_t value;
	};
      } _cons;
    };
  };

  /* this structure holds all the interpreter's registers */
  typedef struct dump_stack_frame_t {
    enum nanoclj_opcode op;
    nanoclj_cell_t * args;
    nanoclj_cell_t * envir;
    nanoclj_val_t code;
  } dump_stack_frame_t;

  typedef struct {
    nanoclj_mutex_t mutex;
    /* arrays for segments */
    nanoclj_cell_t ** alloc_seg;
    nanoclj_val_t * cell_seg;
    int n_seg_reserved;
    int last_cell_seg;
    nanoclj_cell_t * free_cell;      /* pointer to top of free cells */
    long fcells;                  /* # of free cells */
    size_t gensym_cnt, gentypeid_cnt;
    nanoclj_cell_t * properties;
  } nanoclj_shared_context_t;

  struct nanoclj_s {

/* We use 4 registers. */
    nanoclj_cell_t * args;               /* register for arguments of function */
    nanoclj_cell_t * envir;              /* stack register for current environment */
    nanoclj_val_t code;               /* register for current code */
    size_t dump;               /* stack register for next evaluation */

    nanoclj_shared_context_t * context;
    
    nanoclj_cell_t * pending_exception;		/* pending exception */

    nanoclj_cell_t * Object;
    nanoclj_cell_t * Audio;
    nanoclj_cell_t * Image;
    nanoclj_cell_t * Graph;
    nanoclj_cell_t * Double;
    nanoclj_cell_t * SecureRandom;
    nanoclj_cell_t * OutOfMemoryError;
    nanoclj_cell_t * NullPointerException;
    nanoclj_cell_t * Throwable;
    nanoclj_cell_t * IOException;
    nanoclj_cell_t * UnknownHostException;
    nanoclj_cell_t * FileNotFoundException;
    nanoclj_cell_t * AccessDeniedException;
    nanoclj_cell_t * CharacterCodingException;
    nanoclj_cell_t * MalformedURLException;
    
    nanoclj_cell_t sink;	      /* when mem. alloc. fails */
    nanoclj_tensor_t * oblist;         /* pointer to symbol table */
    nanoclj_tensor_t * namespaces;
    nanoclj_cell_t * core_ns;		/* pointer to the core ns */
    nanoclj_tensor_t * types;

/* global pointers to special symbols */
    nanoclj_val_t LAMBDA;             /* pointer to syntax lambda */
    nanoclj_val_t DO;                 /* pointer to syntax do */
    nanoclj_val_t QUOTE;              /* pointer to syntax quote */
    nanoclj_val_t DEREF;		  /* pointer to symbol deref */
    nanoclj_val_t VAR;		  /* pointer to symbol var */
    nanoclj_val_t QQUOTE;             /* pointer to symbol quasiquote */
    nanoclj_val_t UNQUOTE;            /* pointer to symbol unquote */
    nanoclj_val_t UNQUOTESP;          /* pointer to symbol unquote-splicing */

    nanoclj_val_t ARG1;		  /* first arg in literal functions */
    nanoclj_val_t ARG2;		  /* second arg in literal functions */
    nanoclj_val_t ARG3;		  /* third arg in literal functions */
    nanoclj_val_t ARG_REST;	  	  /* rest args in literal functions */
    
    nanoclj_val_t TAG_HOOK;           /* *default-data-reader-fn* */
    nanoclj_val_t COMPILE_HOOK;       /* *compile-hook* */
    nanoclj_val_t AUTOCOMPLETE_HOOK; /* *autocomplete-hook* */
    nanoclj_val_t FLUSH_ON_NEWLINE; /* *flush-on-newline* */
    nanoclj_val_t IN_SYM;	  /* *in* */
    nanoclj_val_t OUT_SYM;	  /* *out* */
    nanoclj_val_t ERR;		  /* *err* */
    nanoclj_val_t NS_SYM;	  /* *ns* */
    nanoclj_val_t CURRENT_FILE;   /* *file* */
    nanoclj_val_t ENV;	  	  /* *env* */
    nanoclj_val_t CELL_SIZE;      /* *cell-size* */
    nanoclj_val_t WINDOW_SIZE;    /* *window-size* */
    nanoclj_val_t WINDOW_SCALE_F;   /* *window-scale-factor* */
    nanoclj_val_t MOUSE_POS;      /* *mouse-pos* */
    nanoclj_val_t THEME;	  /* *theme* */
    
    nanoclj_val_t RECUR;		  /* recur */
    nanoclj_val_t AMP;		  /* & */
    nanoclj_val_t UNDERSCORE;         /* _ */
    nanoclj_val_t AS;		  /* :as */
    nanoclj_val_t DOC;		  /* :doc */
    nanoclj_val_t WIDTH;	  /* :width */
    nanoclj_val_t HEIGHT;	  /* :height */
    nanoclj_val_t CHANNELS;	  /* :channels */
    nanoclj_val_t GRAPHICS;	  /* :graphics */
    nanoclj_val_t WATCHES;	  /* :watches */
    nanoclj_val_t NAME;		  /* :name */
    nanoclj_val_t TYPE;		  /* :type */
    nanoclj_val_t LINE;		  /* :line */
    nanoclj_val_t COLUMN;	  /* :column */
    nanoclj_val_t FILE_KW;	  /* :file */
    nanoclj_val_t NS;		  /* :ns */
    nanoclj_val_t INLINE;	  /* :inline */
    nanoclj_val_t BLOCK;	  /* :block */
    nanoclj_val_t GRAY;		  /* :gray */
    nanoclj_val_t RGB;		  /* :rgb */
    nanoclj_val_t RGBA;		  /* :rgba */
    nanoclj_val_t ITALIC;         /* :italic */
    nanoclj_val_t BOLD;           /* :bold */
    nanoclj_val_t PDF;	          /* :pdf */
    nanoclj_val_t POSITION;	  /* :position */
    nanoclj_val_t EDGES;	  /* :edges */
    nanoclj_val_t SOURCE;         /* :source */
    nanoclj_val_t TARGET;         /* :target */
    nanoclj_val_t DATA;		  /* :data */
    nanoclj_val_t HAIR;	          /* :hair */
    nanoclj_val_t INT;	 	  /* :int */
    nanoclj_val_t FLOAT;	  /* :float */
    nanoclj_val_t DOUBLE;	  /* :double */
    nanoclj_val_t BYTE;	          /* :byte */
    nanoclj_val_t SHORT;          /* :short */
    nanoclj_val_t BOOLEAN;        /* :boolean */
    nanoclj_val_t RELOAD;         /* :reload */
    nanoclj_val_t IMPORT;	  /* :import */
    nanoclj_val_t PRIVATE;	  /* :private */
    nanoclj_val_t GEN_CLASS;	  /* :gen-class */
    
    nanoclj_val_t DOT;		  /* . */
    nanoclj_val_t CATCH;
    nanoclj_val_t FINALLY;
    nanoclj_cell_t * EMPTYVEC, * EMPTYSET, * EMPTYMAP;
    
    nanoclj_val_t save_inport;
    nanoclj_tensor_t * load_stack;
    
    char strbuff[STRBUFFSIZE];
    nanoclj_tensor_t * rdbuff;

    int tok;
    nanoclj_val_t value;
    enum nanoclj_opcode op;

    void *ext_data;             /* For the benefit of foreign functions */

    struct nanoclj_interface *vptr;
    dump_stack_frame_t * dump_base;            /* pointer to base of allocated dump stack */
    size_t dump_size;              /* number of frames allocated for dump stack */
    
    nanoclj_graphics_t term_graphics;
    nanoclj_colortype_t term_colors;
    int window_lines, window_columns;
    double window_scale_factor;
    nanoclj_color_t fg_color, bg_color;

    size_t tests_passed, tests_failed;
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
  nanoclj_val_t closure_code(nanoclj_val_t p);
  nanoclj_val_t closure_env(nanoclj_val_t p);

#endif

#ifdef __cplusplus
}
#endif
#endif

