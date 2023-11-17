#ifndef _NANOCLJ_GC_H_
#define _NANOCLJ_GC_H_

/* ========== garbage collector ========== */

/*--
 *  We use algorithm E (Knuth, The Art of Computer Programming Vol.1,
 *  sec. 2.3.5), the Schorr-Deutsch-Waite link-inversion algorithm,
 *  for marking.
 */
static inline void mark(nanoclj_cell_t * p) {
  nanoclj_cell_t * t = NULL;
  nanoclj_cell_t * q;
  nanoclj_val_t q0;

E2:_setmark(p);
  switch (_type(p)) {
  case T_CLASS:
  case T_ENVIRONMENT:
  case T_LIST:
  case T_CLOSURE:
  case T_MACRO:
  case T_LAZYSEQ:
  case T_DELAY:{
    nanoclj_cell_t * meta = _cons_metadata(p);
    if (meta) mark(meta);
    break;
  }
  case T_FOREIGN_FUNCTION:{
    nanoclj_cell_t * meta = _ff_metadata(p);
    if (meta)  mark(meta);
    break;
  }
  case T_VECTOR:
  case T_ARRAYMAP:
  case T_SORTED_SET:
  case T_QUEUE:
  case T_MAPENTRY:
  case T_VAR:
  case T_RATIO:{
    if (_is_small(p)) {
      nanoclj_val_t md = _so_vector_metadata(p);
      if (is_cell(md) && !is_nil(md)) mark(decode_pointer(md));
     
      size_t s = _sosize_unchecked(p);
      nanoclj_val_t * data = _smalldata_unchecked(p);
      if (s > 0 && is_cell(data[0]) && !is_nil(data[0])) mark(decode_pointer(data[0]));
      if (s > 1 && is_cell(data[1]) && !is_nil(data[1])) mark(decode_pointer(data[1]));
    } else {
      size_t offset = _offset_unchecked(p);
      size_t num = _size_unchecked(p);
      nanoclj_val_t * data = _tensor_unchecked(p)->data;
      for (size_t i = 0; i < num; i++) {
	/* Vector cells will be treated like ordinary cells */
	nanoclj_val_t v = data[offset + i];
	if (is_cell(v) && !is_nil(v)) mark(decode_pointer(v));
      }
    }
  }
    break;

  case T_GRAPH:
    {
      size_t num = _num_nodes_unchecked(p);
      nanoclj_graph_array_t * g = _graph_unchecked(p);
      for (size_t i = 0; i < num; i++) {
	nanoclj_val_t v = g->nodes[i].data;
	if (is_cell(v) && !is_nil(v)) mark(decode_pointer(v));
      }
    }
    break;
  }
  
  if (_is_gc_atom(p))
    goto E6;
  
  /* E4: down car */
  q0 = _car(p);
  if (!is_primitive(q0) && !is_nil(q0) && !is_mark(q0)) {
    _set_gc_atom(p);                 /* a note that we have moved car */
    _set_car(p, mk_pointer(t));
    t = p;
    p = decode_pointer(q0);
    goto E2;
  }
E5:q0 = _cdr(p);                 /* down cdr */
  if (!is_primitive(q0) && !is_nil(q0) && !is_mark(q0)) {
    _set_cdr(p, mk_pointer(t));
    t = p;
    p = decode_pointer(q0);
    goto E2;
  }
E6:                            /* up.  Undo the link switching from steps E4 and E5. */
  if (!t) {
    return;
  }

  q = t;
  if (_is_gc_atom(q)) {
    _clr_gc_atom(q);
    t = decode_pointer(_car(q));
    _set_car(q, mk_pointer(p));
    p = q;
    goto E5;
  } else {
    t = decode_pointer(_cdr(q));
    _set_cdr(q, mk_pointer(p));
    p = q;
    goto E6;
  }
}

static inline void mark_value(nanoclj_val_t v) {
  if (is_cell(v) && !is_nil(v)) mark(decode_pointer(v));
}

static inline void dump_stack_mark(nanoclj_t * sc) {
  size_t nframes = sc->dump;
  for (size_t i = 0; i < nframes; i++) {
    dump_stack_frame_t * frame = sc->dump_base + i;
    if (frame->args) mark(frame->args);
    mark(frame->envir);
    mark_value(frame->code);
  }
}

static void mark_thread(nanoclj_t * sc) {
  if (sc->global_env) mark(sc->global_env);
  
  /* mark current registers */
  if (sc->args) mark(sc->args);
  if (sc->envir) mark(sc->envir);
  mark_value(sc->code);
#ifdef USE_RECUR_REGISTER
  mark_value(sc->recur);
#endif
  dump_stack_mark(sc);
  
  mark_value(sc->value);
  mark_value(sc->save_inport);

  for (size_t i = 0; i < sc->load_stack->ne[0]; i++) {
    mark_value(tensor_get(sc->load_stack, i));
  }
  
  /* Exceptions */
  if (sc->pending_exception) mark(sc->pending_exception);

  mark(sc->EMPTYVEC);
  
  /* Mark recent objects the interpreter doesn't know about yet. */
  mark_value(_car(&(sc->sink)));
}

/* garbage collection. parameter a, b is marked. */
static void gc(nanoclj_t * sc, nanoclj_cell_t * a, nanoclj_cell_t * b, nanoclj_cell_t * c) {
  nanoclj_shared_context_t * ctx = sc->context;

#if GC_VERBOSE
  putstr(sc, "gc...", get_err_port(sc));
#endif
  
  /* mark system globals */
  if (sc->oblist) mark(sc->oblist);
  if (sc->root_env) mark(sc->root_env);
  
  if (ctx->properties) mark(ctx->properties);

  mark(sc->OutOfMemoryError);
  mark(sc->NullPointerException);

  /* Mark types */  
  for (size_t i = 0; i < sc->types->ne[0]; i++) {
    mark_value(tensor_get(sc->types, i));
  }
  
  mark_thread(sc);
    
  /* mark variables a, b, c */
  if (a) mark(a);
  if (b) mark(b);
  if (c) mark(c);

  /* garbage collect */
  clrmark(sc->EMPTY);
  ctx->fcells = 0;
  nanoclj_cell_t * free_cell = decode_pointer(sc->EMPTY);
	
  /* free-list is kept sorted by address so as to maintain consecutive
     ranges, if possible, for use with vectors. Here we scan the cells
     (which are also kept sorted by address) downwards to build the
     free-list in sorted order.
   */
  
  for (int_fast32_t i = ctx->last_cell_seg; i >= 0; i--) {
    nanoclj_cell_t * min_p = decode_pointer(ctx->cell_seg[i]);
    nanoclj_cell_t * p = min_p + CELL_SEGSIZE;
      
    while (--p >= min_p) {      
      if (_is_mark(p)) {
	_clrmark(p);
      } else {
        /* reclaim cell */
        if (p->type != 0 || p->flags != 0) {
	  finalize_cell(sc, p);
          p->type = 0;
	  p->flags = 0;
	  _cons_metadata(p) = NULL;
          _set_car(p, sc->EMPTY);
        }
        ++ctx->fcells;
	_set_cdr(p, mk_pointer(free_cell));
        free_cell = p;
      }
    }
  }
    
  ctx->free_cell = free_cell != &(sc->_EMPTY) ? free_cell : NULL;
  
#if GC_VERBOSE
  char msg[80];
  sprintf(msg,80,"done: %ld cells were recovered.\n", ctx->fcells);
  putstr(sc, msg, get_err_port(sc));
#endif
}

#endif
