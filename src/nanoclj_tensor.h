#ifndef _NANOCLJ_TENSOR_H_
#define _NANOCLJ_TENSOR_H_

#include "nanoclj_prim.h"
#include "nanoclj_types.h"

static inline size_t tensor_get_cell_size(nanoclj_tensor_type_t t) {
  switch (t) {
  case nanoclj_i8: return sizeof(uint8_t);
  case nanoclj_i32: return sizeof(uint32_t);
  case nanoclj_f32: return sizeof(float);
  case nanoclj_f64: return sizeof(double);
  }
  return 0;
}

static inline void tensor_free(nanoclj_tensor_t * tensor) {
  if (tensor && tensor->refcnt != 0 && --(tensor->refcnt) == 0) {
    free(tensor->data);
    free(tensor);
  }
}

/* Assumes tensor is 1-dimensional and peeks the last element */
static inline nanoclj_val_t tensor_peek(nanoclj_tensor_t * tensor) {
  if (tensor->ne[0] > 0) {
    return ((nanoclj_val_t *)tensor->data)[tensor->ne[0] - 1];
  } else {
    return mk_nil();
  }
}

/* Assumes tensor is 1-dimensional and pops the last element */
static inline void tensor_pop(nanoclj_tensor_t * tensor) {
  if (tensor->ne[0] > 0) tensor->ne[0]--;
}

/* Assuments tenso is 1-dimensional and adds an element */
static inline void tensor_mutate_push(nanoclj_tensor_t * tensor, nanoclj_val_t val) {
  if (tensor->ne[0] * tensor->nb[0] >= tensor->nb[1]) {
    tensor->nb[1] = 2 * (sizeof(nanoclj_val_t) + tensor->nb[1]);
    tensor->data = realloc(tensor->data, tensor->nb[1]);
  }
  ((nanoclj_val_t *)tensor->data)[tensor->ne[0]++] = val;
}

static inline void tensor_clear(nanoclj_tensor_t * tensor) {
  tensor->ne[0] = tensor->ne[1] = tensor->ne[2] = 0;
}

static inline int64_t tensor_n_elem(nanoclj_tensor_t * tensor) {
  int64_t n = 1;
  switch (tensor->n_dims) {
  case 3: n *= tensor->ne[2]; return true;
  case 2: n *= tensor->ne[1]; return true;
  case 1: n *= tensor->ne[0]; return true;
  }
  return false;
}

static inline size_t tensor_size(nanoclj_tensor_t * tensor) {
  return tensor->nb[tensor->n_dims + 1];
}

/* Returns true if the size of any of the tensor dimensions is zero */
static inline bool tensor_is_empty(nanoclj_tensor_t * tensor) {
  switch (tensor->n_dims) {
  case 3: if (tensor->ne[2] == 0) return true;
  case 2: if (tensor->ne[1] == 0) return true;
  case 1: if (tensor->ne[0] == 0) return true;
  }
  return false;
}

static inline void tensor_set_f64(nanoclj_tensor_t * tensor, int i, double v) {
  ((double *)tensor->data)[i] = v;
}

static inline double tensor_get_f64(nanoclj_tensor_t * tensor, int i) {
  return ((double *)tensor->data)[i];
}

static inline nanoclj_val_t tensor_get(nanoclj_tensor_t * tensor, int i) {
  nanoclj_val_t v;
  v.as_double = tensor_get_f64(tensor, i);
  return v;
}

static inline uint8_t tensor_get_i8(nanoclj_tensor_t * tensor, int i) {
  return ((uint8_t *)tensor->data)[i];
}

static inline void tensor_set(nanoclj_tensor_t * tensor, int i, nanoclj_val_t v) {
  tensor_set_f64(tensor, i, v.as_double);
}

/* Creates a 1D tensor with padding (reserve space)*/
static inline nanoclj_tensor_t * mk_tensor_1d_padded(nanoclj_tensor_type_t t, int64_t len, int64_t padding) {
  size_t type_size = tensor_get_cell_size(t);
  if (!type_size) return NULL;
  
  size_t size = (len + padding) * type_size;
  void * data = malloc(size);
  
  if (data) {
    nanoclj_tensor_t * s = malloc(sizeof(nanoclj_tensor_t));
    if (s) {
      s->data = data;
      s->n_dims = 1;
      s->type = t;
      s->ne[0] = len;
      s->nb[0] = type_size;
      s->nb[1] = size;
      s->refcnt = 0;
      s->meta = NULL;
      return s;
    } else {
      free(data);
    }
  }
  return NULL;
}

/* Creates a 1D tensor */
static inline nanoclj_tensor_t * mk_tensor_1d(nanoclj_tensor_type_t t, int64_t size) {
  return mk_tensor_1d_padded(t, size, 0);
}

static inline nanoclj_tensor_t * mk_tensor_2d(nanoclj_tensor_type_t t, int64_t d0, int64_t d1) {
  size_t type_size = tensor_get_cell_size(t);
  if (!type_size) return NULL;

  void * data = malloc(d0 * d1 * type_size);
  if (data) {
    nanoclj_tensor_t * tensor = malloc(sizeof(nanoclj_tensor_t));
    if (!tensor) {
      free(data);
    } else {
      tensor->type = t;
      tensor->data = data;
      tensor->n_dims = 2;
      tensor->ne[0] = d0;
      tensor->ne[1] = d1;
      tensor->nb[0] = type_size;
      tensor->nb[1] = d0 * type_size;
      tensor->nb[2] = d0 * d1 * type_size;
      tensor->meta = NULL;
      return tensor;
    }
  }
  return NULL;
}

static inline nanoclj_tensor_t * mk_tensor_3d(nanoclj_tensor_type_t t, int64_t d0, size_t stride0,
					      int64_t d1, size_t stride1, int64_t d2) {
  size_t type_size = tensor_get_cell_size(t);
  if (!type_size) return NULL;

  void * data = malloc(d2 * stride1);
  if (data) {
    nanoclj_tensor_t * tensor = malloc(sizeof(nanoclj_tensor_t));
    if (!tensor) {
      free(data);
    } else {
      tensor->type = t;
      tensor->data = data;
      tensor->n_dims = 3;
      tensor->ne[0] = d0;
      tensor->ne[1] = d1;
      tensor->ne[2] = d2;
      tensor->nb[0] = type_size;
      tensor->nb[1] = stride0;
      tensor->nb[2] = stride1;
      tensor->nb[3] = d2 * stride1;
      tensor->meta = NULL;
      return tensor;
    }
  }
  return NULL;
}

static inline nanoclj_tensor_t * tensor_push(nanoclj_tensor_t * tensor, size_t head, nanoclj_val_t val) {
  if (head < tensor->ne[0] || (head + 1) * sizeof(nanoclj_val_t) > tensor->nb[1]) {
    nanoclj_tensor_t * old_tensor = tensor;
    tensor = mk_tensor_1d_padded(nanoclj_f64, head, head + 1);
    if (!tensor) return NULL;
    memcpy(tensor->data, old_tensor->data, head * sizeof(nanoclj_val_t));
  }
  ((nanoclj_val_t *)tensor->data)[head] = val;
  tensor->ne[0]++;
  return tensor;
}

static inline size_t tensor_mutate_append_bytes(nanoclj_tensor_t * s, const uint8_t * ptr, size_t n) {
  if (s->ne[0] + n > s->nb[1]) {
    s->nb[1] = 2 * (s->ne[0] + n);
    s->data = realloc(s->data, s->nb[1]);
  }
  memcpy(s->data + s->ne[0], ptr, n);
  s->ne[0] += n;
  return n;
}

static inline void tensor_mutate_fill_i8(nanoclj_tensor_t * tensor, uint8_t v) {
  memset(tensor->data, v, tensor->nb[tensor->n_dims]);
}

/* Appends codepoint to the end of an 1D tensor */
static inline size_t tensor_mutate_append_codepoint(nanoclj_tensor_t * t, int32_t c) {
  uint8_t buffer[4];
  return tensor_mutate_append_bytes(t, &buffer[0], utf8proc_encode_char(c, &buffer[0]));
}

static inline nanoclj_tensor_t * tensor_append_codepoint(nanoclj_tensor_t * tensor, size_t head, int32_t c) {
  uint8_t buffer[4];
  size_t n = utf8proc_encode_char(c, &buffer[0]);
  if (head < tensor->ne[0] || head + n > tensor->nb[1]) {
    nanoclj_tensor_t * old_tensor = tensor;
    tensor = mk_tensor_1d_padded(nanoclj_i8, head, head + n);
    memcpy(tensor->data, old_tensor->data, head);
  }
  memcpy(tensor->data + head, &buffer[0], n);
  tensor->ne[0] += n;
  return tensor;
}

static inline imageview_t tensor_to_imageview(nanoclj_tensor_t * tensor) {
  nanoclj_internal_format_t f;
  if (tensor->ne[0] == 4) {
    f = nanoclj_rgba8;
  } else if (tensor->ne[0] == 3) {
    if (tensor->nb[1] == 4) {
      f = nanoclj_bgr8_32;
    } else {
      f = nanoclj_rgb8;
    }
  } else {
    f = nanoclj_r8;
  }

  return (imageview_t){ (uint8_t*)tensor->data, tensor->ne[1], tensor->ne[2], tensor->nb[2], f };
}

#endif
