#ifndef _NANOCLJ_GGML_H_
#define _NANOCLJ_GGML_H_

#include "ggml.h"

static inline void * mk_tensor_context() {
  struct ggml_init_params params = {
    .mem_size   = 16*1024*1024,
    .mem_buffer = NULL,
  };

  struct ggml_context * ctx = ggml_init(params);
  return ctx;
}

static inline nanoclj_val_t mk_tensor_1f(nanoclj_t * sc, size_t size) {
  struct ggml_context * ctx = sc->tensor_ctx;
  
  struct ggml_tensor * x = ggml_new_tensor_1d(ctx, GGML_TYPE_F32, size);

  nanoclj_cell_t * c = get_cell_x(sc, T_TENSOR, T_GC_ATOM, NULL, NULL, NULL);
  _tensor_unchecked(c) = x;

  return mk_pointer(c);
}

static inline nanoclj_val_t mk_tensor_2f(nanoclj_t * sc, size_t width, size_t height) {
  struct ggml_context * ctx = sc->tensor_ctx;
  
  struct ggml_tensor * x = ggml_new_tensor_2d(ctx, GGML_TYPE_F32, width, height);

  nanoclj_cell_t * c = get_cell_x(sc, T_TENSOR, T_GC_ATOM, NULL, NULL, NULL);
  _tensor_unchecked(c) = x;

  return mk_pointer(c);
}

static inline nanoclj_val_t mk_tensor_3f(nanoclj_t * sc, size_t width, size_t height, size_t depth) {
  struct ggml_context * ctx = sc->tensor_ctx;
  
  struct ggml_tensor * x = ggml_new_tensor_3d(ctx, GGML_TYPE_F32, width, height, depth);

  nanoclj_cell_t * c = get_cell_x(sc, T_TENSOR, T_GC_ATOM, NULL, NULL, NULL);
  _tensor_unchecked(c) = x;

  return mk_pointer(c);
}

static inline float tensor_get1f(void * tensor, int x) {
  struct ggml_tensor * a = tensor;
  return *(float *)((char *)a->data + x * a->nb[0]);
}

static inline float tensor_get2f(void * tensor, int x, int y) {
  struct ggml_tensor * a = tensor;
  return *(float *)((char *)a->data + y * a->nb[1] + x * a->nb[0]);
}

static inline int tensor_get_n_dims(void * tensor) {
  struct ggml_tensor * a = tensor;
  return a->n_dims;
}

static inline size_t tensor_get_dim(void * tensor, int dim) {
  struct ggml_tensor * a = tensor;
  return a->ne[dim];
}

#endif
