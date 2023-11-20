#ifndef _NANOCLJ_TENSOR_H_
#define _NANOCLJ_TENSOR_H_

#include "nanoclj_prim.h"
#include "nanoclj_char.h"
#include "nanoclj_types.h"

static inline size_t tensor_get_cell_size(nanoclj_tensor_type_t t) {
  switch (t) {
  case nanoclj_i8: return sizeof(uint8_t);
  case nanoclj_i16: return sizeof(uint16_t);
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
static inline nanoclj_val_t tensor_peek(const nanoclj_tensor_t * tensor) {
  if (tensor->ne[0] > 0) {
    return ((const nanoclj_val_t *)tensor->data)[tensor->ne[0] - 1];
  } else {
    return mk_nil();
  }
}

/* Assumes tensor is 1-dimensional and pops the last element */
static inline void tensor_mutate_pop(nanoclj_tensor_t * tensor) {
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

static inline void tensor_set_f64(nanoclj_tensor_t * tensor, int i, double v) {
  ((double *)tensor->data)[i] = v;
}

static inline double tensor_get_f64(const nanoclj_tensor_t * tensor, int i) {
  return ((double *)tensor->data)[i];
}

static inline double tensor_get_f32_2d(const nanoclj_tensor_t * tensor, int i, int j) {
  return *(float *)((uint8_t *)tensor->data + i * tensor->nb[0] + j * tensor->nb[1]);
}

static inline nanoclj_val_t tensor_get(const nanoclj_tensor_t * tensor, int i) {
  nanoclj_val_t v;
  v.as_double = tensor_get_f64(tensor, i);
  return v;
}

static inline uint8_t tensor_get_i8(const nanoclj_tensor_t * tensor, int i) {
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

/* Semimutable push */
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

static inline void tensor_mutate_append_vec(nanoclj_tensor_t * t, float * vec) {
  if (t->ne[1] * t->nb[1] >= t->nb[2]) {
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
  if (head < tensor->ne[0] || head + n > tensor->nb[1]) {
    nanoclj_tensor_t * old_tensor = tensor;
    tensor = mk_tensor_1d_padded(nanoclj_i8, head, head + n);
    memcpy(tensor->data, old_tensor->data, head);
  }
  memcpy(tensor->data + head, &buffer[0], n);
  tensor->ne[0] += n;
  return tensor;
}

static inline imageview_t tensor_to_imageview(const nanoclj_tensor_t * tensor) {
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

  return (imageview_t){ (const uint8_t*)tensor->data, tensor->ne[1], tensor->ne[2], tensor->nb[2], f };
}

static inline bool tensor_eq(const nanoclj_tensor_t * a, const nanoclj_tensor_t * b) {
  int n = a->n_dims;
  if (n != b->n_dims) return false;
  switch (n) {
  case 3: if (a->nb[3] != b->nb[3] || a->ne[2] != b->ne[2]) return false;
  case 2: if (a->nb[2] != b->nb[2] || a->ne[1] != b->ne[1]) return false;
  case 1: if (a->nb[1] != b->nb[1] || a->ne[0] != b->ne[0] ||
	      a->nb[0] != b->nb[0]) return false;
  }

  if (n == 1 && a->nb[0] == 4) {
    int64_t ne = a->ne[0];
    const uint32_t * ad = a->data, * bd = b->data;
    for (int64_t i = 0; i < ne; i++) {
      if (ad[i] != bd[i]) return false;
    }
  } else {
    /* TODO */
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

static inline void tensor_mutate_lshift(nanoclj_tensor_t * a, uint_fast8_t n) {
  if (!n || tensor_is_empty(a)) return;
  uint32_t * limbs = a->data;
  uint32_t overflow = limbs[a->ne[0] - 1] >> (32 - n);
  if (overflow) {
    tensor_mutate_append_i32(a, overflow);
  }
  for (int64_t i = a->ne[0] - 1; i > 0; i--) {
    limbs[i] = (limbs[i] << n) | (limbs[i - 1] >> (32 - n));
  }
  limbs[0] <<= n;
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
  for (int64_t i = 0; i < b->ne[0]; ++i) {
    limbs_a[i] |= limbs_b[i];
  }
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

static inline nanoclj_tensor_t * tensor_bigint_dup(const nanoclj_tensor_t * tensor) {
  nanoclj_tensor_t * r = mk_tensor_1d(nanoclj_i32, tensor->ne[0]);
  memcpy(r->data, tensor->data, tensor->nb[1]);
  return r;
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
  nanoclj_tensor_t * c = tensor_bigint_dup(a);
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
  uint32_t * limbs = tensor->data;
  uint64_t x = 0;
  for (int64_t i = 0; i < tensor->ne[0]; i++) {
    x += (uint64_t)v * limbs[i];
    limbs[i] = x;
    x >>= 32;
  }
  if (x) tensor_mutate_append_i32(tensor, x);
}

static inline uint32_t tensor_bigint_mutate_divmod(nanoclj_tensor_t * tensor, uint32_t divisor) {
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
  const uint32_t * limbs = tensor->data;
  for (int64_t i = tensor->ne[0] - 1; i >= 0; i--) {
    v *= 0xffffffff;
    v += limbs[i];
  }
  return v;
}

static inline nanoclj_tensor_t * tensor_bigint_inc(const nanoclj_tensor_t * tensor) {
  nanoclj_tensor_t * r = tensor_bigint_dup(tensor);
  tensor_bigint_mutate_add_int(r, 1);
  return r;
}

static inline nanoclj_tensor_t * tensor_bigint_dec(const nanoclj_tensor_t * tensor) {
  nanoclj_tensor_t * r = tensor_bigint_dup(tensor);
  uint32_t * d = r->data;
  for (int64_t i = 0; i < r->ne[0]; i++) {
    uint32_t tmp = d[i], res = tmp - 1;
    d[i] = res;
    if (!(res > tmp)) { /* finish if there is no underflow */
      tensor_mutate_trim(r);
      return r;
    }
  }
  tensor_mutate_clear(r);
  return r;
}

static inline nanoclj_tensor_t * tensor_bigint_div(nanoclj_tensor_t * a, nanoclj_tensor_t * b) {
  return NULL;
}

static inline nanoclj_tensor_t * tensor_bigint_pow(const nanoclj_tensor_t * base0, uint64_t exp) {
  if (!exp) {
    return mk_tensor_bigint(1);
  } else if (tensor_is_empty(base0)) {
    return mk_tensor_bigint(0);
  } else {
    nanoclj_tensor_t * result = mk_tensor_bigint(1);
    nanoclj_tensor_t * base = tensor_bigint_dup(base0);
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
    return result;
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

static inline size_t tensor_bigint_to_string(const nanoclj_tensor_t * tensor0, char ** buff) {
  nanoclj_tensor_t * tensor = tensor_bigint_dup(tensor0);
  size_t n = ((tensor->ne[0] * 32) >> 1) + 5;
  char * tmp = malloc(n);
  char *p = tmp + n;
  while (!tensor_is_empty(tensor)) {
    /* divide the bigint by 1e9 and put the remainder in r */
    uint64_t r = tensor_bigint_mutate_divmod(tensor, 1000000000);
    /* print the remainder */
    for (int_fast8_t i = 0; i < 9 && (!tensor_is_empty(tensor) || r > 0); i++, r /= 10) {
      *--p = '0' + (r % 10);
    }
  }
  size_t real_n = tmp + n - p;
  *buff = malloc(real_n + 1);
  memcpy(*buff, p, real_n);
  (*buff)[real_n] = 0;
  free(tmp);
  tensor_free(tensor);
  return real_n;
}

#endif
