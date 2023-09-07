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
  case T_VECTOR:
  case T_ARRAYMAP:
  case T_SORTED_SET:{
    if (_is_small(p)) {
      size_t s = _sosize_unchecked(p);
      nanoclj_val_t * data = _smalldata_unchecked(p);
      if (s > 0 && is_cell(data[0]) && !is_nil(data[0])) mark(decode_pointer(data[0]));
      if (s > 1 && is_cell(data[1]) && !is_nil(data[1])) mark(decode_pointer(data[1]));
      if (s > 2 && is_cell(data[2]) && !is_nil(data[2])) mark(decode_pointer(data[2]));
    } else {
      size_t offset = _offset_unchecked(p);
      size_t num = _size_unchecked(p);
      nanoclj_val_t * data = _vec_store_unchecked(p)->data;
      for (size_t i = 0; i < num; i++) {
	/* Vector cells will be treated like ordinary cells */
	nanoclj_val_t v = data[offset + i];
	if (is_cell(v) && !is_nil(v)) mark(decode_pointer(v));
      }
    }
  }
    break;
  }

  nanoclj_cell_t * metadata = _metadata_unchecked(p);
  if (metadata) {
    mark(metadata);
  }
  
  if (_is_gc_atom(p))
    goto E6;
  /* E4: down car */
  q0 = _car(p);
  if (!is_primitive(q0) && !is_nil(q0) && !is_mark(q0)) {
    _set_gc_atom(p);                 /* a note that we have moved car */
    _car(p) = mk_pointer(t);
    t = p;
    p = decode_pointer(q0);
    goto E2;
  }
E5:q0 = _cdr(p);                 /* down cdr */
  if (!is_primitive(q0) && !is_nil(q0) && !is_mark(q0)) {
    _cdr(p) = mk_pointer(t);
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
    _car(q) = mk_pointer(p);
    p = q;
    goto E5;
  } else {
    t = decode_pointer(_cdr(q));
    _cdr(q) = mk_pointer(p);
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
    mark_value(frame->args);
    mark(frame->envir);
    mark_value(frame->code);
  }
}

/* garbage collection. parameter a, b is marked. */
static void gc(nanoclj_t * sc, nanoclj_cell_t * a, nanoclj_cell_t * b, nanoclj_cell_t * c) {
#if GC_VERBOSE
  putstr(sc, "gc...", get_err_port(sc));
#endif

  /* mark system globals */
  if (sc->oblist) mark(sc->oblist);
  if (sc->root_env) mark(sc->root_env);
  if (sc->global_env) mark(sc->global_env);
 
  /* mark current registers */
  mark_value(sc->args);
  if (sc->envir) mark(sc->envir);
  mark_value(sc->code);
#ifdef USE_RECUR_REGISTER
  mark_value(sc->recur);
#endif
  dump_stack_mark(sc);

  mark_value(sc->value);
  mark_value(sc->save_inport);
  mark_value(sc->loadport);

  mark_value(sc->active_element);
  mark_value(sc->active_element_target);
  mark_value(sc->EMPTYVEC);
    
  /* Mark recent objects the interpreter doesn't know about yet. */
  mark_value(car(sc->sink));

  /* Mark leaf types */
  mark(sc->INTEGER);
  mark(sc->REAL);
  mark(sc->BOOLEAN);
  mark(sc->CHARACTER);
  mark(sc->PROC);
  mark(sc->KEYWORD);
  mark(sc->SYMBOL);

  /* mark variables a, b, c */
  if (a) mark(a);
  if (b) mark(b);
  if (c) mark(c);

  /* garbage collect */
  clrmark(sc->EMPTY);
  sc->fcells = 0;
  nanoclj_cell_t * free_cell = decode_pointer(sc->EMPTY);
	
  /* free-list is kept sorted by address so as to maintain consecutive
     ranges, if possible, for use with vectors. Here we scan the cells
     (which are also kept sorted by address) downwards to build the
     free-list in sorted order.
   */

  long total_cells = 0;
  
  for (int_fast32_t i = sc->last_cell_seg; i >= 0; i--) {
    nanoclj_cell_t * min_p = decode_pointer(sc->cell_seg[i]);
    nanoclj_cell_t * p = min_p + CELL_SEGSIZE;
      
    while (--p >= min_p) {
      total_cells++;
      
      if (_is_mark(p)) {
	_clrmark(p);
      } else {
        /* reclaim cell */
        if (_cell_type(p) != 0 || _cell_flags(p) != 0) {
	  finalize_cell(sc, p);
          _cell_type(p) = 0;
	  _cell_flags(p) = 0;
	  _metadata_unchecked(p) = NULL;
          _car(p) = sc->EMPTY;
        }
        ++sc->fcells;
	_cdr(p) = mk_pointer(free_cell);
        free_cell = p;
      }
    }
  }
    
  sc->free_cell = free_cell;

#if GC_VERBOSE
  char msg[80];
  sprintf(msg, "done: %ld / %ld cells were recovered.\n", sc->fcells, total_cells);
  putstr(sc, msg, get_err_port(sc));
#endif
}

#endif
