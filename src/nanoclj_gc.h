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
  case T_FOREIGN_FUNCTION:{
    nanoclj_cell_t * meta = _ff_metadata(p);
    if (meta) mark(meta);
    break;
  }
  case T_IMAGE:
    {
      nanoclj_cell_t * meta = p->_image.meta;
      if (meta) mark(meta);
    }
    break;
  case T_VAR:
  case T_VECTOR:
  case T_ARRAYMAP:
  case T_HASHSET:
  case T_HASHMAP:
  case T_SORTED_HASHSET:
  case T_SORTED_HASHMAP:
  case T_QUEUE:
  case T_MAPENTRY:
  case T_VARMAP:
    if (_is_small(p)) {
      size_t s = _sodim0_unchecked(p) * _sodim1_unchecked(p);
      nanoclj_val_t * data = _smalldata_unchecked(p);
      for (int64_t i = 0; i < s; i++) {
	nanoclj_val_t v = data[i];
	if (is_cell(v)) mark(decode_pointer(v));
      }
    } else {
      nanoclj_tensor_t * tensor = p->_collection.tensor;
      size_t num = _size_unchecked(p);
      if (tensor_is_sparse(tensor)) {
	int64_t num_buckets = tensor_hash_get_bucket_count(tensor);
	for (int64_t offset = 0; offset < num_buckets; offset++) {
	  int64_t idx = tensor->sparse_indices[offset];
	  if (idx >= 0 && idx < num) {
	    if (tensor->n_dims == 1) {
	      nanoclj_val_t v = tensor_get(tensor, offset);
	      if (is_cell(v)) mark(decode_pointer(v));
	    } else {
	      for (int64_t i = 0; i < tensor->ne[0]; i++) {
		nanoclj_val_t v = tensor_get_2d(tensor, i, offset);
		if (is_cell(v)) mark(decode_pointer(v));
	      }
	    }
	  }
	}
      } else {
	nanoclj_val_t * data = (nanoclj_val_t *)tensor->data;
	size_t offset = _offset_unchecked(p);
	for (size_t i = 0; i < num; i++) {
	  if (tensor->n_dims == 1) {
	    nanoclj_val_t v = data[offset + i];
	    if (is_cell(v)) mark(decode_pointer(v));
	  } else {
	    for (int64_t j = 0; j < tensor->ne[0]; j++) {
	      nanoclj_val_t v = tensor_get_2d(tensor, j, i);
	      if (is_cell(v)) mark(decode_pointer(v));
	    }
	  }
	}
      }

      if (p->_collection.meta) mark(p->_collection.meta);
    }
    break;

  case T_GRAPH:
    {
      size_t num = _num_nodes_unchecked(p);
      nanoclj_graph_array_t * g = _graph_unchecked(p);
      for (size_t i = 0; i < num; i++) {
	nanoclj_val_t v = g->nodes[i].data;
	if (is_cell(v)) mark(decode_pointer(v));
      }
    }
    break;
  }
  
  if (_is_gc_atom(p))
    goto E6;

  /* mark cons cell meta here (not an atom) */
  if (_type(p) == T_LISTMAP) {
    nanoclj_val_t value = p->_cons.value;
    if (is_cell(value)) mark(decode_pointer(value));
  } else {
    nanoclj_cell_t * meta = _cons_metadata(p);
    if (meta) mark(meta);
  }
  
  /* E4: down car */
  q0 = _car(p);
  if (is_cell(q0) && !is_mark(q0)) {
    _set_gc_atom(p);                 /* a note that we have moved car */
    _set_car(p, mk_pointer(t));
    t = p;
    p = decode_pointer(q0);
    goto E2;
  }
 E5:q = _cdr(p);                 /* down cdr */
  if (q && !_is_mark(q)) {
    _set_cdr(p, t);
    t = p;
    p = q;
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
    t = _cdr(q);
    _set_cdr(q, p);
    p = q;
    goto E6;
  }
}

static inline void mark_value(nanoclj_val_t v) {
  if (is_cell(v)) mark(decode_pointer(v));
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
  if (sc->current_ns) mark(sc->current_ns);
  
  /* mark current registers */
  if (sc->args) mark(sc->args);
  if (sc->envir) mark(sc->envir);
  mark_value(sc->code);

  dump_stack_mark(sc);
  
  mark_value(sc->value);
  mark_value(sc->save_inport);

  if (sc->load_stack) {
    for (size_t i = 0; i < sc->load_stack->ne[0]; i++) {
      mark_value(tensor_get(sc->load_stack, i));
    }
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
  if (sc->root_ns) mark(sc->root_ns);
  
  if (ctx->properties) mark(ctx->properties);

  mark(sc->OutOfMemoryError);
  mark(sc->NullPointerException);

  /* Mark types */
  if (sc->types) {
    for (int64_t i = 0; i < sc->types->ne[0]; i++) {
      mark_value(tensor_get(sc->types, i));
    }
  }

  mark_thread(sc);
  
  /* mark variables a, b, c */
  if (a) mark(a);
  if (b) mark(b);
  if (c) mark(c);

  /* garbage collect */
  ctx->fcells = 0;
  nanoclj_cell_t * free_cell = NULL;
  
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
          _set_car(p, mk_emptylist());
        }
        ++ctx->fcells;
	_set_cdr(p, free_cell);
        free_cell = p;
      }
    }
  }

  ctx->free_cell = free_cell;
  
#if GC_VERBOSE
  char msg[80];
  sprintf(msg,80,"done: %ld cells were recovered.\n", ctx->fcells);
  putstr(sc, msg, get_err_port(sc));
#endif
}

#endif
