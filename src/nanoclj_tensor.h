#ifndef _NANOCLJ_TENSOR_H_
#define _NANOCLJ_TENSOR_H_

#include "nanoclj_prim.h"
#include "nanoclj_char.h"
#include "nanoclj_types.h"

static inline size_t tensor_get_cell_size(nanoclj_tensor_type_t t) {
  switch (t) {
  case nanoclj_boolean: return sizeof(uint8_t);
  case nanoclj_i8: return sizeof(uint8_t);
  case nanoclj_i16: return sizeof(uint16_t);
  case nanoclj_i32: return sizeof(uint32_t);
  case nanoclj_f32: return sizeof(float);
  case nanoclj_f64: return sizeof(double);
  case nanoclj_val: return sizeof(nanoclj_val_t);
  }
  return 0;
}

static inline void tensor_free(nanoclj_tensor_t * tensor) {
  if (tensor && (tensor->refcnt == 0 || --(tensor->refcnt) == 0)) {
    free(tensor->sparse_indices);
    free(tensor->data);
    free(tensor);
  }
}

/* Assumes tensor is 1-dimensional and peeks the last element */
static inline nanoclj_val_t tensor_peek(const nanoclj_tensor_t * tensor) {
  if (tensor->ne[0] > 0) {
    return ((const nanoclj_val_t *)tensor->data)[tensor->ne[0] - 1];
  } else {
    return mk_nil();
  }
}

/* Assumes tensor is 1-dimensional and replaces the last element */
static inline void tensor_replace(nanoclj_tensor_t * tensor, nanoclj_val_t v) {
  if (tensor->ne[0] > 0) {
    ((nanoclj_val_t *)tensor->data)[tensor->ne[0] - 1] = v;
  }
}

/* Assumes tensor is 1-dimensional and pops the last element */
static inline void tensor_mutate_pop(nanoclj_tensor_t * tensor) {
  if (tensor->ne[0] > 0) tensor->ne[0]--;
}

/* Assuments tensor is 1-dimensional and adds an element */
static inline void tensor_mutate_push(nanoclj_tensor_t * tensor, nanoclj_val_t val) {
  if (tensor->ne[0] * tensor->nb[0] >= tensor->nb[1]) {
    tensor->nb[1] = 2 * (tensor->ne[0] + 1) * tensor->nb[0];
    tensor->data = realloc(tensor->data, tensor->nb[1]);
  }
  ((nanoclj_val_t *)tensor->data)[tensor->ne[0]++] = val;
}

static inline void tensor_mutate_clear(nanoclj_tensor_t * tensor) {
  tensor->ne[0] = tensor->ne[1] = tensor->ne[2] = 0;
}

static inline int64_t tensor_n_elem(const nanoclj_tensor_t * tensor) {
  int64_t n = 1;
  switch (tensor->n_dims) {
  case 3: n *= tensor->ne[2]; return true;
  case 2: n *= tensor->ne[1]; return true;
  case 1: n *= tensor->ne[0]; return true;
  }
  return false;
}

static inline size_t tensor_size(const nanoclj_tensor_t * tensor) {
  return tensor->nb[tensor->n_dims + 1];
}

/* Returns true if the size of any of the tensor dimensions is zero */
static inline bool tensor_is_empty(const nanoclj_tensor_t * tensor) {
  switch (tensor->n_dims) {
  case 3: if (tensor->ne[2] == 0) return true;
  case 2: if (tensor->ne[1] == 0) return true;
  case 1: if (tensor->ne[0] == 0) return true;
  }
  return false;
}

/* Returns true if the tensor is an identity object (e.g. 1) */
static inline bool tensor_is_identity(const nanoclj_tensor_t * tensor) {
  if ((tensor->n_dims == 1 && tensor->ne[0] == 1) ||
      (tensor->n_dims == 2 && tensor->ne[0] == 1 && tensor->ne[1] == 1) ||
      (tensor->n_dims == 3 && tensor->ne[0] == 1 && tensor->ne[1] == 1 && tensor->ne[2] == 1)) {
    /* Simple one-element object */
    switch (tensor->type) {
    case nanoclj_boolean:
    case nanoclj_i8: return *(uint8_t*)tensor->data == 1;
    case nanoclj_i16: return *(uint16_t*)tensor->data == 1;
    case nanoclj_i32: return *(uint32_t*)tensor->data == 1;
    case nanoclj_f32: return *(float*)tensor->data == 1.0f;
    case nanoclj_f64:
    case nanoclj_val: return *(double*)tensor->data == 1.0;
    }
  } else {
    /* TODO */
  }
  return false;
}

static inline void tensor_mutate_set_i8(nanoclj_tensor_t * tensor, int64_t i, uint8_t v) {
  ((uint8_t *)tensor->data)[i] = v;
}

static inline void tensor_mutate_set_i16(nanoclj_tensor_t * tensor, int64_t i, uint16_t v) {
  ((uint16_t *)tensor->data)[i] = v;
}

static inline void tensor_mutate_set_i32(nanoclj_tensor_t * tensor, int64_t i, uint32_t v) {
  ((uint32_t *)tensor->data)[i] = v;
}

static inline void tensor_mutate_set_f32(nanoclj_tensor_t * tensor, int64_t i, double v) {
  ((float *)tensor->data)[i] = v;
}

static inline void tensor_mutate_set_f64(nanoclj_tensor_t * tensor, int64_t i, double v) {
  ((double *)tensor->data)[i] = v;
}

static inline void tensor_mutate_set_f64_2d(nanoclj_tensor_t * tensor, int64_t i, int64_t j, double v) {
  *(double *)(tensor->data + i * tensor->nb[0] + j * tensor->nb[1]) = v;
}

static inline void tensor_mutate_set(nanoclj_tensor_t * tensor, int64_t i, nanoclj_val_t v) {
  tensor_mutate_set_f64(tensor, i, v.as_double);
}

static inline void tensor_mutate_set_2d(nanoclj_tensor_t * tensor, int64_t i, int64_t j, nanoclj_val_t v) {
  tensor_mutate_set_f64_2d(tensor, i, j, v.as_double);
}

static inline uint8_t tensor_get_i8(const nanoclj_tensor_t * tensor, int64_t i) {
  return ((uint8_t *)tensor->data)[i];
}

static inline uint16_t tensor_get_i16(const nanoclj_tensor_t * tensor, int64_t i) {
  return ((uint16_t *)tensor->data)[i];
}

static inline uint32_t tensor_get_i32(const nanoclj_tensor_t * tensor, int64_t i) {
  return ((uint32_t *)tensor->data)[i];
}

static inline double tensor_get_f32(const nanoclj_tensor_t * tensor, int64_t i) {
  return ((float *)tensor->data)[i];
}

static inline double tensor_get_f64(const nanoclj_tensor_t * tensor, int64_t i) {
  return ((double *)tensor->data)[i];
}

static inline double tensor_get_i8_2d(const nanoclj_tensor_t * tensor, int64_t i, int64_t j) {
  return *(uint8_t *)(tensor->data + i * tensor->nb[0] + j * tensor->nb[1]);
}

static inline double tensor_get_i16_2d(const nanoclj_tensor_t * tensor, int64_t i, int64_t j) {
  return *(uint16_t *)(tensor->data + i * tensor->nb[0] + j * tensor->nb[1]);
}

static inline double tensor_get_i32_2d(const nanoclj_tensor_t * tensor, int64_t i, int64_t j) {
  return *(uint32_t *)(tensor->data + i * tensor->nb[0] + j * tensor->nb[1]);
}

static inline double tensor_get_f32_2d(const nanoclj_tensor_t * tensor, int64_t i, int64_t j) {
  return *(float *)(tensor->data + i * tensor->nb[0] + j * tensor->nb[1]);
}

static inline double tensor_get_f64_2d(const nanoclj_tensor_t * tensor, int64_t i, int64_t j) {
  return *(double *)(tensor->data + i * tensor->nb[0] + j * tensor->nb[1]);
}

static inline nanoclj_val_t tensor_get(const nanoclj_tensor_t * tensor, int64_t i) {
  switch (tensor->type) {
  case nanoclj_boolean: return mk_boolean(tensor_get_i8(tensor, i));
  case nanoclj_i8: return mk_byte(tensor_get_i8(tensor, i));
  case nanoclj_i16: return mk_short(tensor_get_i16(tensor, i));
  case nanoclj_i32: return mk_int(tensor_get_i32(tensor, i));
  case nanoclj_f32: return mk_double(tensor_get_f32(tensor, i));
  case nanoclj_f64: return mk_double(tensor_get_f64(tensor, i));
  case nanoclj_val:
    { /* Can't use mk_double() here since it would break nan-packing */
      nanoclj_val_t v;
      v.as_double = tensor_get_f64(tensor, i);
      return v;
    }
  }
  return mk_nil();
}

static inline nanoclj_val_t tensor_get_2d(const nanoclj_tensor_t * tensor, int64_t i, int64_t j) {
  switch (tensor->type) {
  case nanoclj_boolean:
  case nanoclj_i8: return mk_byte(tensor_get_i8_2d(tensor, i, j));
  case nanoclj_i16: return mk_short(tensor_get_i16_2d(tensor, i, j));
  case nanoclj_i32: return mk_int(tensor_get_i32_2d(tensor, i, j));
  case nanoclj_f32: return mk_double(tensor_get_f32_2d(tensor, i, j));
  case nanoclj_f64: return mk_double(tensor_get_f32_2d(tensor, i, j));
  case nanoclj_val:
    { /* Can't use mk_double() here since it would break nan-packing */
      nanoclj_val_t v;
      v.as_double = tensor_get_f64_2d(tensor, i, j);
      return v;
    }
  }
  return mk_nil();
}

/* Creates a 1D tensor with padding (reserve space)*/
static inline nanoclj_tensor_t * mk_tensor_1d_padded(nanoclj_tensor_type_t t, int64_t len, int64_t padding) {
  size_t type_size = tensor_get_cell_size(t);
  if (!type_size) return NULL;
  
  size_t size = (len + padding) * type_size;
  void * data = malloc(size);
  
  if (data) {
    nanoclj_tensor_t * tensor = malloc(sizeof(nanoclj_tensor_t));
    if (tensor) {
      tensor->data = data;
      tensor->sparse_indices = NULL;
      tensor->n_dims = 1;
      tensor->type = t;
      tensor->ne[0] = len;
      tensor->nb[0] = type_size;
      tensor->nb[1] = size;
      tensor->refcnt = 0;
      return tensor;
    } else {
      free(data);
    }
  }
  return NULL;
}

static inline nanoclj_tensor_t * mk_tensor_2d_padded(nanoclj_tensor_type_t t, int64_t d0, int64_t d1, int64_t padding) {
  size_t type_size = tensor_get_cell_size(t);
  if (!type_size) return NULL;

  size_t size = d0 * (d1 + padding) * type_size;
  void * data = malloc(size);
  if (data) {
    nanoclj_tensor_t * tensor = malloc(sizeof(nanoclj_tensor_t));
    if (!tensor) {
      free(data);
    } else {
      tensor->type = t;
      tensor->data = data;
      tensor->sparse_indices = NULL;
      tensor->n_dims = 2;
      tensor->ne[0] = d0;
      tensor->ne[1] = d1;
      tensor->nb[0] = type_size;
      tensor->nb[1] = d0 * type_size;
      tensor->nb[2] = size;
      tensor->refcnt = 0;
      return tensor;
    }
  }
  return NULL;
}

/* Creates a 1D tensor */
static inline nanoclj_tensor_t * mk_tensor_1d(nanoclj_tensor_type_t t, int64_t size) {
  return mk_tensor_1d_padded(t, size, 0);
}

static inline nanoclj_tensor_t * mk_tensor_2d(nanoclj_tensor_type_t t, int64_t d0, int64_t d1) {
  return mk_tensor_2d_padded(t, d0, d1, 0);
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
      tensor->sparse_indices = NULL;
      tensor->n_dims = 3;
      tensor->ne[0] = d0;
      tensor->ne[1] = d1;
      tensor->ne[2] = d2;
      tensor->nb[0] = type_size;
      tensor->nb[1] = stride0;
      tensor->nb[2] = stride1;
      tensor->nb[3] = d2 * stride1;
      tensor->refcnt = 0;
      return tensor;
    }
  }
  return NULL;
}

static inline nanoclj_tensor_t * tensor_dup(const nanoclj_tensor_t * tensor) {
  if (tensor->n_dims == 2) {
    size_t is = tensor->nb[tensor->n_dims] / tensor->nb[tensor->n_dims - 1];
    nanoclj_tensor_t * t = mk_tensor_2d_padded(tensor->type, tensor->ne[0], tensor->ne[1], is - tensor->ne[1]);
    if (tensor->sparse_indices) {
      t->sparse_indices = malloc(is * sizeof(int64_t));
      for (size_t i = 0; i < is; i++) t->sparse_indices[i] = tensor->sparse_indices[i];
    }
    memcpy(t->data, tensor->data, tensor->nb[2]);
    return t;
  } else if (tensor->n_dims == 1) {
    nanoclj_tensor_t * t = mk_tensor_1d(tensor->type, tensor->ne[0]);
    memcpy(t->data, tensor->data, tensor->ne[0] * tensor->nb[0]);
    return t;
  }
  return NULL;
}

static inline nanoclj_tensor_t * tensor_resize(nanoclj_tensor_t * tensor, int64_t from_size, int64_t size) {
  switch (tensor->n_dims) {
  case 1:
    if (from_size != tensor->ne[0] || size * tensor->nb[0] > tensor->nb[1]) {
      nanoclj_tensor_t * old_tensor = tensor;
      tensor = mk_tensor_1d_padded(old_tensor->type, old_tensor->ne[0], size);
      if (!tensor) return NULL;
      memcpy(tensor->data, old_tensor->data, old_tensor->ne[0] * old_tensor->nb[0]);
    }
    tensor->ne[0] = size;
    break;
  case 2:
    if (from_size != tensor->ne[1] || size * tensor->nb[1] > tensor->nb[2]) {
      nanoclj_tensor_t * old_tensor = tensor;
      tensor = mk_tensor_2d_padded(old_tensor->type, old_tensor->ne[0], old_tensor->ne[1], size);
      if (!tensor) return NULL;
      memcpy(tensor->data, old_tensor->data, old_tensor->ne[1] * tensor->nb[1]);
    }
    tensor->ne[1] = size;
    break;
  }
  return tensor;
}

/* Semimutable push */
static inline nanoclj_tensor_t * tensor_push(nanoclj_tensor_t * tensor, size_t head, nanoclj_val_t val) {
  tensor = tensor_resize(tensor, head, head + 1);
  tensor_mutate_set(tensor, head, val);
  return tensor;
}

/* Semimutable push */
static inline nanoclj_tensor_t * tensor_push_i8(nanoclj_tensor_t * tensor, size_t head, uint8_t val) {
  tensor = tensor_resize(tensor, head, head + 1);
  tensor_mutate_set_i8(tensor, head, val);
  return tensor;
}

/* Semimutable push */
static inline nanoclj_tensor_t * tensor_push_i16(nanoclj_tensor_t * tensor, size_t head, uint16_t val) {
  tensor = tensor_resize(tensor, head, head + 1);
  tensor_mutate_set_i16(tensor, head, val);
  return tensor;
}

/* Semimutable push */
static inline nanoclj_tensor_t * tensor_push_i32(nanoclj_tensor_t * tensor, size_t head, uint32_t val) {
  tensor = tensor_resize(tensor, head, head + 1);
  tensor_mutate_set_i32(tensor, head, val);
  return tensor;
}

/* Semimutable push */
static inline nanoclj_tensor_t * tensor_push_f32(nanoclj_tensor_t * tensor, size_t head, float val) {
  tensor = tensor_resize(tensor, head, head + 1);
  tensor_mutate_set_f32(tensor, head, val);
  return tensor;
}

/* Semimutable push */
static inline nanoclj_tensor_t * tensor_push_f64(nanoclj_tensor_t * tensor, size_t head, float val) {
  tensor = tensor_resize(tensor, head, head + 1);
  tensor_mutate_set_f64(tensor, head, val);
  return tensor;
}

/* Semimutable push */
static inline nanoclj_tensor_t * tensor_push_vec(nanoclj_tensor_t * tensor, int64_t head, nanoclj_val_t * vec) {
  tensor = tensor_resize(tensor, head, head + 1);
  memcpy(tensor->data + head * tensor->nb[1], vec, tensor->nb[1]);
  return tensor;
}

static inline void tensor_mutate_append_i32(nanoclj_tensor_t * s, int32_t v) {
  if ((s->ne[0] + 1) * sizeof(int32_t) > s->nb[1]) {
    s->nb[1] = 2 * (s->ne[0] + 1) * sizeof(int32_t);
    s->data = realloc(s->data, s->nb[1]);
  }
  *(int32_t*)(s->data + s->ne[0] * s->nb[0]) = v;
  s->ne[0]++;
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

/* Assumes tensor is 2-dimensional and adds a vector.
 * vec must have the correct size */
static inline void tensor_mutate_append_vec(nanoclj_tensor_t * t, void * vec) {
  if ((t->ne[1] + 1) * t->nb[1] > t->nb[2]) {
    t->nb[2] = 2 * (t->ne[1] + 1) * t->nb[1];
    t->data = realloc(t->data, t->nb[2]);
  }
  memcpy(t->data + t->ne[1] * t->nb[1], vec, t->nb[1]);
  t->ne[1]++;
}

/* Appends codepoint to the end of an 1D tensor */
static inline size_t tensor_mutate_append_codepoint(nanoclj_tensor_t * t, int32_t c) {
  uint8_t buffer[4];
  return tensor_mutate_append_bytes(t, &buffer[0], utf8proc_encode_char(c, &buffer[0]));
}

/* Semimutable append */
static inline nanoclj_tensor_t * tensor_append_codepoint(nanoclj_tensor_t * tensor, size_t head, int32_t c) {
  uint8_t buffer[4];
  size_t n = utf8proc_encode_char(c, &buffer[0]);
  tensor = tensor_resize(tensor, head, head + n);
  memcpy(tensor->data + head, &buffer[0], n);
  return tensor;
}

static inline nanoclj_internal_format_t tensor_image_get_internal_format(const nanoclj_tensor_t * tensor) {
  if (tensor->ne[0] == 4) {
    return nanoclj_rgba8;
  } else if (tensor->ne[0] == 3) {
    if (tensor->nb[1] == 4) {
      return nanoclj_bgr8_32;
    } else {
      return nanoclj_rgb8;
    }
  } else {
    return nanoclj_r8;
  }
}

static inline imageview_t tensor_to_imageview(const nanoclj_tensor_t * tensor) {
  nanoclj_internal_format_t f = tensor_image_get_internal_format(tensor);
  return (imageview_t){ (const uint8_t*)tensor->data, tensor->ne[1], tensor->ne[2], tensor->nb[2], f };
}

static inline bool tensor_eq(const nanoclj_tensor_t * a, const nanoclj_tensor_t * b) {
  int n = a->n_dims;
  if (n != b->n_dims) return false;
  switch (n) {
  case 3: if (a->ne[2] != b->ne[2]) return false;
  case 2: if (a->ne[1] != b->ne[1]) return false;
  case 1: if (a->ne[0] != b->ne[0]) return false;
  }

  if (n > 0) {
    if (a->nb[0] != b->nb[0]) return false;
    if (n == 1 && a->nb[0] == 4) {
      int64_t ne = a->ne[0];
      const uint32_t * ad = a->data, * bd = b->data;
      for (int64_t i = 0; i < ne; i++) {
	if (ad[i] != bd[i]) return false;
      }
    } else {
      /* TODO */
    }
  }
  return true;
}

static inline int tensor_cmp(const nanoclj_tensor_t * a, const nanoclj_tensor_t * b) {
  int64_t n = a->ne[0];
  if (n < b->ne[0]) {
    return -1;
  } else if (n > b->ne[0]) {
    return +1;
  } else {
    const uint32_t * a_limbs = a->data, * b_limbs = b->data;
    while (n > 0) {
      n--;
      if (a_limbs[n] > b_limbs[n]) return 1;
      else if (a_limbs[n] < b_limbs[n]) return -1;
    }
    return 0;
  }
}

static inline void tensor_mutate_trim(nanoclj_tensor_t * tensor) {
  uint32_t * limbs = tensor->data;
  while (tensor->ne[0] > 0) {
    if (limbs[tensor->ne[0] - 1] > 0) {
      break;
    }
    tensor->ne[0]--;
  }
}

static inline nanoclj_tensor_t * tensor_mutate_lshift(nanoclj_tensor_t * a, uint_fast8_t n) {
  if (!tensor_is_empty(a)) {
    if (n >= 32) {
      int n2 = n / 32;
      for (int i = 0; i < n2; i++) tensor_mutate_append_i32(a, 0);
      uint32_t * limbs = a->data;
      for (int i = a->ne[0] - 1; i >= n2; i--) {
	limbs[i] = limbs[i - n2];
      }
      for (int i = 0; i < n2; i++) {
	limbs[i] = 0;
      }
      n -= n2 * 32;
    }
    if (n != 0) {
      uint32_t * limbs = a->data;
      uint32_t overflow = limbs[a->ne[0] - 1] >> (32 - n);
      for (int64_t i = a->ne[0] - 1; i > 0; i--) {
	limbs[i] = (limbs[i] << n) | (limbs[i - 1] >> (32 - n));
      }
      limbs[0] <<= n;
      if (overflow) {
	tensor_mutate_append_i32(a, overflow);
      }
    }
  }
  return a;
}

static inline void tensor_mutate_rshift(nanoclj_tensor_t * a, uint_fast8_t n) {
  if (!n || tensor_is_empty(a)) return;
  uint32_t * limbs = a->data;
  for (int64_t i = 0; i < a->ne[0] - 1; i++) {
    limbs[i] = (limbs[i] >> n) | (limbs[i + 1] << (32 - n));
  }
  limbs[a->ne[0] - 1] >>= n;
  tensor_mutate_trim(a);
}

static inline void tensor_mutate_or(nanoclj_tensor_t * a, const nanoclj_tensor_t * b) {
  while (a->ne[0] < b->ne[0]) {
    tensor_mutate_append_i32(a, 0);
  }
  uint32_t * limbs_a = a->data;
  const uint32_t * limbs_b = b->data;
  for (int64_t i = 0; i < b->ne[0]; i++) {
    limbs_a[i] |= limbs_b[i];
  }
}

static inline void tensor_mutate_and(nanoclj_tensor_t * a, const nanoclj_tensor_t * b) {
  if (a->ne[0] > b->ne[0]) a->ne[0] = b->ne[0];
  uint32_t * limbs_a = a->data;
  const uint32_t * limbs_b = b->data;
  for (int64_t i = 0; i < a->ne[0]; i++) {
    limbs_a[i] &= limbs_b[i];
  }
}

static inline void tensor_mutate_sort(nanoclj_tensor_t * tensor, int (*compar)(const void *a, const void *b)) {
  int n = tensor->n_dims;
  if (n >= 1 && tensor->ne[n - 1] >= 2) qsort(tensor->data, tensor->ne[n - 1], tensor->nb[n - 1], compar);
}

static inline bool tensor_is_sparse(const nanoclj_tensor_t * tensor) {
  return tensor->sparse_indices != NULL;
}

/* BigInts */

static inline nanoclj_tensor_t * mk_tensor_bigint(uint64_t v) {
  nanoclj_tensor_t * tensor;
  if (!v) {
    tensor = mk_tensor_1d(nanoclj_i32, 0);
  } else if (v < UINT32_MAX) {
    tensor = mk_tensor_1d(nanoclj_i32, 1);
    *(uint32_t*)tensor->data = v;
  } else {
    tensor = mk_tensor_1d(nanoclj_i32, 2);
    ((uint32_t*)tensor->data)[0] = v & 0xffffffff;
    ((uint32_t*)tensor->data)[1] = v >> 32;
  }
  return tensor;
}

static inline void tensor_bigint_assign(nanoclj_tensor_t * tensor, uint64_t v) {
  tensor_mutate_clear(tensor);
  if (v) {
    tensor_mutate_append_i32(tensor, v & 0xffffffff);
    if (v >= UINT32_MAX) tensor_mutate_append_i32(tensor, v >> 32);
  }
}

static inline nanoclj_tensor_t * mk_tensor_bigint_abs(int64_t v) {
  if (v == LLONG_MIN) {
    return tensor_mutate_lshift(mk_tensor_bigint(1), 63);
  } else {
    return mk_tensor_bigint(llabs(v));
  }
}

static inline void tensor_bigint_mutate_sub(nanoclj_tensor_t * a, const nanoclj_tensor_t * b) {
  int64_t n_a = a->ne[0], n_b = b->ne[0];
  if (n_a < n_b) {
    tensor_mutate_clear(a); /* underflow */
    return;
  }
  uint32_t * limbs_a = a->data;
  const uint32_t * limbs_b = b->data;
  int64_t i;
  uint_fast32_t borrow = 0;
  for (i = 0; i < n_b; i++) {
    uint64_t x = limbs_a[i] + (UINT64_C(1) << 32) - limbs_b[i] - borrow;
    limbs_a[i] = x;
    borrow = x < (UINT64_C(1) << 32);
  }
  for (; i < n_a; i++) {
    uint64_t x = limbs_a[i] + (UINT64_C(1) << 32) - borrow;
    limbs_a[i] = x;
    borrow = x < (UINT64_C(1) << 32);
  }
  if (borrow) {
    tensor_mutate_clear(a); /* underflow */
  } else {
    tensor_mutate_trim(a);
  }
}

static inline nanoclj_tensor_t * tensor_bigint_sub(const nanoclj_tensor_t * a, const nanoclj_tensor_t * b) {
  nanoclj_tensor_t * c = tensor_dup(a);
  tensor_bigint_mutate_sub(c, b);
  return c;
}

static inline nanoclj_tensor_t * tensor_bigint_add(const nanoclj_tensor_t * a, const nanoclj_tensor_t * b) {
  int64_t n_a = a->ne[0], n_b = b->ne[0];
  int64_t i, n_max = n_a > n_b ? n_a : n_b;
  nanoclj_tensor_t * tensor = mk_tensor_1d(nanoclj_i32, n_max + 1);
  const uint32_t * limbs_a = a->data, * limbs_b = b->data;
  uint32_t * limbs_c = tensor->data;
  uint64_t x = 0;
  for (i = 0; i < n_max; i++) {
    if (i < n_a) x += limbs_a[i];
    if (i < n_b) x += limbs_b[i];
    limbs_c[i] = x;
    x >>= 32;
  }
  if (x) limbs_c[i++] = x;
  tensor->ne[0] = i;
  return tensor;
}

static inline void tensor_bigint_mutate_add_int(nanoclj_tensor_t * tensor, uint32_t v) {
  if (v) {
    uint64_t x = v;
    uint32_t * limbs = tensor->data;
    for (int i = 0; i < tensor->ne[0]; i++) {
      x += limbs[i];
      limbs[i] = x;
      x >>= 32;
      if (!x) return;
    }
    tensor_mutate_append_i32(tensor, x);
  }
}

static inline bool tensor_bigint_mutate_sub_int(nanoclj_tensor_t * tensor, uint32_t v) {
  if (!v) return true;
  uint32_t * d = tensor->data;
  for (int64_t i = 0; i < tensor->ne[0]; i++) {
    uint32_t tmp = d[i], res = tmp - v;
    d[i] = res;
    if (!(res > tmp)) { /* finish if there is no underflow */
      tensor_mutate_trim(tensor);
      return true;
    }
    v = 1;
  }
  tensor_mutate_clear(tensor);
  return false;
}

static inline nanoclj_tensor_t * tensor_bigint_mul(const nanoclj_tensor_t * a, const nanoclj_tensor_t * b) {
  int64_t n_a = a->ne[0], n_b = b->ne[0];
  nanoclj_tensor_t * tensor = mk_tensor_1d(nanoclj_i32, n_a + n_b);
  tensor_mutate_fill_i8(tensor, 0);
  const uint32_t * limbs_a = a->data, * limbs_b = b->data;
  uint32_t * limbs_c = tensor->data;

  for (int64_t i = 0; i < n_a; i++) {
    uint64_t t = 0;
    for (int64_t j = 0; j < n_b; j++) {
      t += limbs_c[i + j] + (uint64_t)limbs_a[i] * limbs_b[j];
      limbs_c[i + j] = t;
      t >>= 32;
    }
    if (t) limbs_c[i + n_b] = t;
  }
  tensor_mutate_trim(tensor);
  return tensor;
}

static inline void tensor_bigint_mutate_mul_int(nanoclj_tensor_t * tensor, uint32_t v) {
  if (!tensor_is_empty(tensor)) {
    if (v == 0) {
      tensor_mutate_clear(tensor);
    } else {
      uint32_t * limbs = tensor->data;
      uint64_t x = 0;
      for (int64_t i = 0; i < tensor->ne[0]; i++) {
	x += (uint64_t)v * limbs[i];
	limbs[i] = x;
	x >>= 32;
      }
      if (x) tensor_mutate_append_i32(tensor, x);
    }
  }
}

static inline uint32_t tensor_bigint_mutate_idivmod(nanoclj_tensor_t * tensor, uint32_t divisor) {
  uint32_t * limbs = tensor->data;
  uint64_t x = 0;
  for (int64_t i = tensor->ne[0] - 1; i >= 0; i--) {
    x = x << 32 | limbs[i];
    limbs[i] = x / divisor;
    x = x % divisor;
  }
  tensor_mutate_trim(tensor);
  return x;
}

static inline double tensor_bigint_to_double(const nanoclj_tensor_t * tensor) {
  double v = 0;
  if (!tensor_is_empty(tensor)) {
    const uint32_t * limbs = tensor->data;
    for (int64_t i = tensor->ne[0] - 1; i >= 0; i--) {
      v *= 0x100000000;
      v += limbs[i];
    }
  }
  return v;
}

static inline void tensor_bigint_divmod(const nanoclj_tensor_t * a, const nanoclj_tensor_t * b, nanoclj_tensor_t ** q, nanoclj_tensor_t ** rem) {
  if (b->ne[0] == 1) {
    nanoclj_tensor_t * c = tensor_dup(a);
    uint32_t r = tensor_bigint_mutate_idivmod(c, *(uint32_t*)b->data);
    if (q) *q = c;
    else tensor_free(c);
    if (rem) *rem = mk_tensor_bigint(r);
  } else {
    nanoclj_tensor_t * c = mk_tensor_bigint(0);
    nanoclj_tensor_t * current = mk_tensor_bigint(1);
    nanoclj_tensor_t * denom = tensor_dup(b);
    nanoclj_tensor_t * tmp = tensor_dup(a);
    
    while (tensor_cmp(denom, a) != 1) {
      tensor_mutate_lshift(current, 1);
      tensor_mutate_lshift(denom, 1);
    }
    tensor_mutate_rshift(denom, 1);
    tensor_mutate_rshift(current, 1);
    while (!tensor_is_empty(current)) {
      if (tensor_cmp(tmp, denom) != -1) {
	tensor_bigint_mutate_sub(tmp, denom);
	tensor_mutate_or(c, current);
      }
      tensor_mutate_rshift(current, 1);
      tensor_mutate_rshift(denom, 1);
    }

    if (rem) {
      *rem = tensor_dup(a);
      nanoclj_tensor_t * tmp = tensor_bigint_mul(b, c);
      *rem = tensor_bigint_sub(*rem, tmp);
      tensor_free(tmp);
    }
    tensor_free(current);
    tensor_free(denom);
    tensor_free(tmp);

    if (q) *q = c;
    else tensor_free(c);
  }
}

static inline void tensor_bigint_mutate_div(nanoclj_tensor_t * a, const nanoclj_tensor_t * b) {
  if (b->ne[0] == 1) {
    tensor_bigint_mutate_idivmod(a, *(uint32_t*)b->data);
  } else {
    nanoclj_tensor_t * current = mk_tensor_bigint(1);
    nanoclj_tensor_t * denom = tensor_dup(b);
    nanoclj_tensor_t * tmp = tensor_dup(a);
    tensor_bigint_assign(a, 0);
    
    while (tensor_cmp(denom, tmp) != 1) {
      tensor_mutate_lshift(current, 1);
      tensor_mutate_lshift(denom, 1);
    }
    tensor_mutate_rshift(denom, 1);
    tensor_mutate_rshift(current, 1);
    while (!tensor_is_empty(current)) {
      if (tensor_cmp(tmp, denom) != -1) {
	tensor_bigint_mutate_sub(tmp, denom);
	tensor_mutate_or(a, current);
      }
      tensor_mutate_rshift(current, 1);
      tensor_mutate_rshift(denom, 1);
    }

    tensor_free(current);
    tensor_free(denom);
    tensor_free(tmp);
  }
}

static inline nanoclj_tensor_t * tensor_bigint_pow(const nanoclj_tensor_t * base0, uint64_t exp) {
  if (!exp) {
    return mk_tensor_bigint(1);
  } else if (tensor_is_empty(base0)) {
    return mk_tensor_bigint(0);
  } else {
    nanoclj_tensor_t * result = mk_tensor_bigint(1);
    nanoclj_tensor_t * base = tensor_dup(base0);
    if (exp & 1) {
      nanoclj_tensor_t * tmp = tensor_bigint_mul(result, base);
      tensor_free(result);
      result = tmp;
    }
    exp >>= 1;
    while (exp) {
      nanoclj_tensor_t * tmp = tensor_bigint_mul(base, base);
      tensor_free(base);
      base = tmp;
      if (exp & 1) {
	tmp = tensor_bigint_mul(result, base);
	tensor_free(result);
	result = tmp;
      }
      exp >>= 1;
    }
    tensor_free(base);
    return result;
  }
}

static inline nanoclj_tensor_t * tensor_bigint_gcd(const nanoclj_tensor_t * a0, const nanoclj_tensor_t * b0) {
  if (tensor_is_empty(b0)) {
    return tensor_dup(a0);
  } else {
    nanoclj_tensor_t * rem;
    tensor_bigint_divmod(a0, b0, NULL, &rem);
    nanoclj_tensor_t * a = tensor_dup(b0);
    nanoclj_tensor_t * b = rem;
    while (!tensor_is_empty(b)) {
      tensor_bigint_divmod(a, b, NULL, &rem);
      tensor_free(a);
      a = b;
      b = rem;
    }
    tensor_free(b);
    return a;
  }
}

static inline nanoclj_tensor_t * mk_tensor_bigint_from_string(const char * s, size_t len, int radix) {
  nanoclj_tensor_t * tensor = mk_tensor_bigint(0);
  for (size_t i = 0; i < len; i++) {
    int d = digit(s[i], radix);
    if (d == -1) {
      tensor_free(tensor);
      return NULL;
    }
    tensor_bigint_mutate_mul_int(tensor, radix);
    tensor_bigint_mutate_add_int(tensor, d);
  }
  return tensor;
}

/* Hashes */

static inline nanoclj_tensor_t * mk_tensor_hash(int64_t dims, size_t payload_size, size_t initial_size) {
  nanoclj_tensor_t * t;
  if (dims == 1) {
    t = mk_tensor_1d_padded(nanoclj_val, 0, initial_size);
  } else {
    t = mk_tensor_2d_padded(nanoclj_val, payload_size, 0, initial_size);
  }
  t->sparse_indices = malloc(initial_size * sizeof(int64_t));
  for (int64_t i = 0; i < initial_size; i++) t->sparse_indices[i] = -1;
  return t;
}

static inline int64_t tensor_hash_get_bucket_count(const nanoclj_tensor_t * tensor) {
  return tensor->nb[tensor->n_dims] / tensor->nb[tensor->n_dims - 1];
}

static inline size_t tensor_hash_get_bucket(uint32_t hash, uint32_t num_buckets, bool is_ordered) {
  if (is_ordered) return (((uint64_t)hash * num_buckets) >> 32);
  else return hash % num_buckets;
}

static inline void _tensor_hash_mutate_set(nanoclj_tensor_t * tensor, uint32_t hash, int64_t val_index, nanoclj_val_t key, nanoclj_val_t val, nanoclj_val_t meta, bool is_ordered) {
  int64_t num_buckets = tensor_hash_get_bucket_count(tensor);
  int64_t * sparse_indices = tensor->sparse_indices;
  int64_t offset = tensor_hash_get_bucket(hash, num_buckets, is_ordered);
  while ( 1 ) {
    if (sparse_indices[offset] == -1) {
      sparse_indices[offset] = val_index;
      if (tensor->n_dims == 2) {
	tensor_mutate_set_2d(tensor, 0, offset, key);
	tensor_mutate_set_2d(tensor, 1, offset, val);
	if (tensor->ne[0] >= 3) tensor_mutate_set_2d(tensor, 2, offset, meta);
      } else {
	tensor_mutate_set(tensor, offset, key);
      }
      break;
    } else {
      if (++offset == num_buckets) {
	if (is_ordered) {
	  /* TODO: handle failure */
	  break;
	} else offset = 0;
      }
    }
  }
  tensor->ne[tensor->n_dims - 1]++;
}

static inline nanoclj_tensor_t * tensor_hash_mutate_set(nanoclj_tensor_t * tensor, uint32_t hash, int64_t val_index, nanoclj_val_t key, nanoclj_val_t val, nanoclj_val_t meta, bool is_ordered, void * context, uint32_t (*hashfun)(nanoclj_val_t, void *)) {
  bool rebuild = false, overload = false;
  if (val_index * 10 / tensor_hash_get_bucket_count(tensor) >= 7) { /* load factor more than 70% */
    rebuild = overload = true;
  } else if (val_index < tensor->ne[tensor->n_dims - 1]) {
    rebuild = true;
  }
  if (rebuild) {
    nanoclj_tensor_t * old_tensor = tensor;
    int64_t old_num_buckets = tensor_hash_get_bucket_count(old_tensor);
    int64_t num_buckets = overload ? 2 * (old_num_buckets + 1) : old_num_buckets;
    tensor = mk_tensor_hash(old_tensor->n_dims, old_tensor->ne[0], num_buckets);
    if (!tensor) return NULL;
    int64_t * old_sparse_indices = old_tensor->sparse_indices;
    for (int64_t offset = 0; offset < old_num_buckets; offset++) {
      int64_t i = old_sparse_indices[offset];
      if (i >= 0 && i < val_index) {
	nanoclj_val_t old_key, old_val, old_meta = mk_nil();
	if (old_tensor->n_dims == 2) {
	  old_key = tensor_get_2d(old_tensor, 0, offset);
	  old_val = tensor_get_2d(old_tensor, 1, offset);
	  if (old_tensor->ne[0] >= 3) old_meta = tensor_get_2d(old_tensor, 2, offset);
	} else {
	  old_key = tensor_get(old_tensor, offset);
	  old_val = mk_nil();
	}
	uint32_t hash = hashfun(old_key, context);
	_tensor_hash_mutate_set(tensor, hash, tensor->ne[tensor->n_dims - 1], old_key, old_val, old_meta, is_ordered);
      }
    }
    if (!old_tensor->refcnt) tensor_free(old_tensor);
  }

  _tensor_hash_mutate_set(tensor, hash, val_index, key, val, meta, is_ordered);
  return tensor;
}

static inline int64_t tensor_hash_first_offset(const nanoclj_tensor_t * tensor, int64_t offset, int64_t limit) {
  int64_t num_buckets = tensor_hash_get_bucket_count(tensor);
  int64_t * sparse_indices = tensor->sparse_indices;
  while ( offset < num_buckets ) {
    int64_t i = sparse_indices[offset];
    if (i >= 0 && i < limit) return offset;
    offset++;
  }
  return offset;
}

static inline int64_t tensor_hash_rest(const nanoclj_tensor_t * tensor, int64_t offset, int64_t limit) {
  int64_t num_buckets = tensor_hash_get_bucket_count(tensor);
  int64_t * sparse_indices = tensor->sparse_indices;
  while ( offset < num_buckets && sparse_indices[offset] == -1 ) {
    offset++;
  }
  if (offset < num_buckets) {
    offset++;
    while ( offset < num_buckets ) {
      int64_t i = sparse_indices[offset];
      if (i >= 0 && i < limit) break;
      offset++;
    }
  }
  return offset;
}

static inline size_t tensor_hash_count(const nanoclj_tensor_t * tensor, int64_t offset, int64_t limit) {
  int64_t num_buckets = tensor_hash_get_bucket_count(tensor);
  int64_t * sparse_indices = tensor->sparse_indices;
  size_t count = 0;
  for (; offset < num_buckets; offset++) {
    int64_t i = sparse_indices[offset];
    if (i >= 0 && i < limit) count++;
  }
  return count;
}

static inline bool tensor_is_valid_offset(const nanoclj_tensor_t * tensor, uint64_t offset) {
  return offset * tensor->nb[tensor->n_dims - 1] < tensor->nb[tensor->n_dims];
}

static inline bool tensor_hash_is_unassigned(const nanoclj_tensor_t * tensor, size_t offset) {
  return tensor->sparse_indices[offset] == -1;
}

static inline nanoclj_val_t tensor_hash_get(const nanoclj_tensor_t * tensor, size_t offset, int64_t limit) {
  int64_t val_index = tensor->sparse_indices[offset];
  if (val_index >= 0 && val_index < limit) return *(nanoclj_val_t *)(tensor->data + offset * tensor->nb[tensor->n_dims - 1]);
  else return mk_nil();
}

#endif
