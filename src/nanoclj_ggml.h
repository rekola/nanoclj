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

static inline nanoclj_val_t mk_tensor(nanoclj_t * sc, void * tensor) {
  nanoclj_cell_t * c = get_cell_x(sc, T_TENSOR, T_GC_ATOM, NULL, NULL, NULL);
  _tensor_unchecked(c) = tensor;
  return mk_pointer(c);
}

static inline nanoclj_val_t mk_tensor_1f(nanoclj_t * sc, size_t size) {
  return mk_tensor(sc, ggml_new_tensor_1d((struct ggml_context *)sc->tensor_ctx, GGML_TYPE_F32, size));  
}

static inline nanoclj_val_t mk_tensor_2f(nanoclj_t * sc, size_t width, size_t height) {
  return mk_tensor(sc, ggml_new_tensor_2d((struct ggml_context *)sc->tensor_ctx, GGML_TYPE_F32, width, height));
}

static inline nanoclj_val_t mk_tensor_3f(nanoclj_t * sc, size_t width, size_t height, size_t depth) {
  return mk_tensor(sc, ggml_new_tensor_3d((struct ggml_context *)sc->tensor_ctx, GGML_TYPE_F32, width, height, depth));
}

static inline float tensor_get1f(void * tensor, size_t x) {
  return ggml_get_f32_1d((struct ggml_tensor *)tensor, x);
}

static inline void tensor_set1f(void * tensor, int x, float v) {
  ggml_set_f32_1d((struct ggml_tensor *)tensor, x, v);
}

static inline float tensor_get2f(void * tensor, size_t x, size_t y) {
  struct ggml_tensor * a = tensor;
  return *(float *)((char *)a->data + y * a->nb[1] + x * a->nb[0]);
}

static inline void tensor_set2f(void * tensor, size_t x, size_t y, float v) {
  struct ggml_tensor * a = tensor;
  *(float *)((char *)a->data + y * a->nb[1] + x * a->nb[0]) = v;
}

static inline int tensor_get_n_dims(void * tensor) {
  struct ggml_tensor * a = tensor;
  return a->n_dims;
}

static inline size_t tensor_get_dim(void * tensor, int dim) {
  struct ggml_tensor * a = tensor;
  return a->ne[dim];
}

static inline nanoclj_val_t tensor_add(nanoclj_t * sc, void * a, void * b) {
  return mk_tensor(sc, ggml_add((struct ggml_context *)sc->tensor_ctx,
				(struct ggml_tensor *)a, (struct ggml_tensor *)b)); 
}

#endif
