#ifndef _NANOCLJ_FUNCTIONS_H_
#define _NANOCLJ_FUNCTIONS_H_

#include <stdint.h>
#include <ctype.h>
#include <shapefil.h>
#include <libxml/tree.h>
#include <libxml/parser.h>

#include "linenoise.h"
#include "nanoclj_utils.h"
#include "nanoclj_graph.h"

#ifdef WIN32

#include <direct.h>
#include <processthreadsapi.h>

#define getcwd _getcwd
#define environ _environ
#define STBIW_WINDOWS_UTF8

#else

#include <glob.h>
#include <unistd.h>

#if !HAVE_ARC4RANDOM
#include <sys/random.h>
#endif

#endif

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_IMAGE_RESIZE_STATIC
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"

#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

/* Thread */

static nanoclj_val_t Thread_sleep(nanoclj_t * sc, nanoclj_cell_t * args) {
  long long ms = to_long(first(sc, args));
  if (ms > 0) {
    nanoclj_sleep(ms);
  }
  return mk_nil();
}

/* System */

static nanoclj_val_t System_exit(nanoclj_t * sc, nanoclj_cell_t * args) {
  exit(to_int(first(sc, args)));
}

static nanoclj_val_t System_currentTimeMillis(nanoclj_t * sc, nanoclj_cell_t * args) {
  return mk_long(sc, system_time() / 1000);
}

static nanoclj_val_t System_nanoTime(nanoclj_t * sc, nanoclj_cell_t * args) {
  return mk_long(sc, 1000 * system_time());
}

static nanoclj_val_t System_gc(nanoclj_t * sc, nanoclj_cell_t * args) {
  gc(sc, NULL, NULL, NULL);
  return (nanoclj_val_t)kTRUE;
}

static nanoclj_val_t System_getenv(nanoclj_t * sc, nanoclj_cell_t * args) {
  if (args) {
    char * name = alloc_c_str(to_strview(first(sc, args)));
    const char * v = getenv(name);
    free(name);
    return v ? mk_string(sc, v) : mk_nil();
  } else {
    extern char **environ;
    nanoclj_cell_t * m = mk_hashmap(sc);
    
    for (size_t i = 0; environ[i]; i++) {
      char * p = environ[i], * eq = strchr(p, '=');
      nanoclj_val_t key, val;
      if (eq) {
	key = mk_string_from_sv(sc, (strview_t){ p, eq - p });
	val = mk_string(sc, eq + 1);
      } else {
	key = mk_string(sc, p);
	val = mk_nil();
      }
      m = assoc(sc, m, key, val);
    }

    return mk_pointer(m);
  }
}

static nanoclj_val_t System_getProperty(nanoclj_t * sc, nanoclj_cell_t * args) {
  if (args) {
    return find(sc, sc->context->properties, first(sc, args), mk_nil());
  } else {
    return mk_pointer(sc->context->properties);
  }
}

static nanoclj_val_t System_setProperty(nanoclj_t * sc, nanoclj_cell_t * args) {
  nanoclj_val_t key = first(sc, args);
  nanoclj_val_t val = second(sc, args);

  sc->context->properties = assoc(sc, sc->context->properties, key, val);
  return mk_nil();
}

static inline nanoclj_val_t System_glob(nanoclj_t * sc, nanoclj_cell_t * args) {
#ifndef WIN32
  char * tmp = alloc_c_str(to_strview(first(sc, args)));
  glob_t gstruct;
  int r = glob(tmp, GLOB_ERR, NULL, &gstruct);
  nanoclj_cell_t * x = NULL;
  if (r == 0) {
    for (char ** found = gstruct.gl_pathv; *found; found++) {
      nanoclj_cell_t * str = get_string_object(sc, T_FILE, *found, strlen(*found), 0);
      x = cons(sc, mk_pointer(str), x);
    }
  }
  free(tmp);
  globfree(&gstruct);
  
  if (r == 0 || r == GLOB_NOMATCH) {
    return mk_pointer(x);
  }
#endif
  return mk_nil();
}

#ifdef WIN32
static inline long long filetime_to_msec(FILETIME ft) {
  return ((((long long)(ft.dwHighDateTime)) << 32) | ((long long)ft.dwLowDateTime)) / 1000000;
}
#endif

/* Returns the system timing information (idle, kernel, user) */
static nanoclj_val_t System_getSystemTimes(nanoclj_t * sc, nanoclj_cell_t * args) {
  nanoclj_cell_t * r = NULL;
#ifdef WIN32
  FILETIME idleTime, kernelTime, userTime;
  GetSystemTimes(&idleTime, &kernelTime, &userTime);
  long long idle = filetime_to_msec(idleTime);
  r = mk_vector(sc, 3);
  set_indexed_value(r, 0, mk_long(sc, idle));
  set_indexed_value(r, 1, mk_long(sc, filetime_to_msec(kernelTime) - idle));
  set_indexed_value(r, 2, mk_long(sc, filetime_to_msec(userTime)));
#else
  strview_t sv = to_strview(slurp(sc, T_READER, cons(sc, mk_string(sc, "/proc/stat"), NULL)));
  if (sc->pending_exception) return mk_nil();
  char * b = alloc_c_str(sv);
  const char * p;
  if (strncmp(b, "cpu ", 4) == 0) p = b;
  else p = strstr(b, "\ncpu ");
  if (p) {
    long long user, nice, system, idle;
    if (sscanf(p, "cpu  %lld %lld %lld %lld", &user, &nice, &system, &idle) == 4) {
      r = mk_vector(sc, 3);
      set_indexed_value(r, 0, mk_long(sc, idle));
      set_indexed_value(r, 1, mk_long(sc, system));
      set_indexed_value(r, 2, mk_long(sc, user));
    }
  }
  free(b);
#endif
  return mk_pointer(r);
}

/* Math */

static nanoclj_val_t Math_abs(nanoclj_t * sc, nanoclj_cell_t * args) {
  return mk_double(fabs(to_double(first(sc, args))));
}

static nanoclj_val_t Math_sin(nanoclj_t * sc, nanoclj_cell_t * args) {
  return mk_double(sin(to_double(first(sc, args))));
}

static nanoclj_val_t Math_cos(nanoclj_t * sc, nanoclj_cell_t * args) {
  return mk_double(cos(to_double(first(sc, args))));
}

static nanoclj_val_t Math_exp(nanoclj_t * sc, nanoclj_cell_t * args) {
  return mk_double(exp(to_double(first(sc, args))));
}

static nanoclj_val_t Math_log(nanoclj_t * sc, nanoclj_cell_t * args) {
  return mk_double(log(to_double(first(sc, args))));
}

static nanoclj_val_t Math_log10(nanoclj_t * sc, nanoclj_cell_t * args) {
  return mk_double(log10(to_double(first(sc, args))));
}

static nanoclj_val_t Math_tan(nanoclj_t * sc, nanoclj_cell_t * args) {
  return mk_double(tan(to_double(first(sc, args))));
}

static nanoclj_val_t Math_asin(nanoclj_t * sc, nanoclj_cell_t * args) {
  return mk_double(asin(to_double(first(sc, args))));
}

static nanoclj_val_t Math_acos(nanoclj_t * sc, nanoclj_cell_t * args) {
  return mk_double(acos(to_double(first(sc, args))));
}

static nanoclj_val_t Math_atan(nanoclj_t * sc, nanoclj_cell_t * args) {
  nanoclj_val_t x = first(sc, args);
  args = next(sc, args);
  if (args) {
    nanoclj_val_t y = first(sc, args);
    return mk_double(atan2(to_double(x), to_double(y)));
  } else {
    return mk_double(atan(to_double(x)));
  }
}

static nanoclj_val_t Math_sqrt(nanoclj_t * sc, nanoclj_cell_t * args) {
  return mk_double(sqrt(to_double(first(sc, args))));
}

static nanoclj_val_t Math_cbrt(nanoclj_t * sc, nanoclj_cell_t * args) {
  return mk_double(cbrt(to_double(first(sc, args))));
}

static nanoclj_val_t Math_pow(nanoclj_t * sc, nanoclj_cell_t * args) {
  return mk_double(pow(to_double(first(sc, args)), to_double(second(sc, args))));
}

static nanoclj_val_t Math_floor(nanoclj_t * sc, nanoclj_cell_t * args) {
  return mk_double(floor(to_double(first(sc, args))));
}

static nanoclj_val_t Math_ceil(nanoclj_t * sc, nanoclj_cell_t * args) {
  return mk_double(ceil(to_double(first(sc, args))));
}

static nanoclj_val_t Math_round(nanoclj_t * sc, nanoclj_cell_t * args) {
  return mk_double(round(to_double(first(sc, args))));
}

static nanoclj_val_t Math_random(nanoclj_t * sc, nanoclj_cell_t * args) {
#if HAVE_ARC4RANDOM
  return mk_double((double)arc4random() / UINT32_MAX);
#else
  return mk_double((double)random() / INT_MAX);
#endif
}

/* Double */

static nanoclj_val_t Double_doubleToLongBits(nanoclj_t * sc, nanoclj_cell_t * args) {
  double d = to_double(first(sc, args));
  return mk_long(sc, mk_double(isnan(d) ? NAN : d).as_long);
}

static nanoclj_val_t Double_doubleToRawLongBits(nanoclj_t * sc, nanoclj_cell_t * args) {
  nanoclj_val_t v = mk_double(to_double(first(sc, args)));
  return mk_long(sc, v.as_long);
}

/* SecureRandom */

static nanoclj_val_t SecureRandom_nextBytes(nanoclj_t * sc, nanoclj_cell_t * args) {
  nanoclj_val_t ba0 = second(sc, args);
  if (type(ba0) != T_TENSOR) {
    return mk_nil();
  }
  nanoclj_cell_t * c = decode_pointer(ba0);
  nanoclj_tensor_t * tensor = get_tensor(c);
  if (tensor->type != nanoclj_i8) {
    return mk_nil();
  }
  uint8_t * ptr = get_ptr(c);
  size_t n = get_size(c);
#if HAVE_ARC4RANDOM
  arc4random_buf(ptr, n);
#else
  getrandom(ptr, n, 0);
#endif

  return mk_nil();
}

/* clojure.math.numeric-tower */

#define SQRT_INT64_MAX (INT64_C(0xB504F333))

static inline bool ipow_overflow(long long base, int_fast8_t exp, long long * res) {
  if (base == 1) {
    *res = 1;
    return false;
  }
  if (!exp) {
    *res = 1;
    return false;
  }
  if (!base) {
    *res = 0;
    return false;
  }

  long long result = 1;
  if (exp & 1) result *= base;
  exp >>= 1;
  while (exp) {
    if (base > SQRT_INT64_MAX) return true;
    base *= base;
    if (exp & 1) result *= base;
    exp >>= 1;
  }

  *res = result;
  return false;
}

static nanoclj_val_t numeric_tower_expt(nanoclj_t * sc, nanoclj_cell_t * args) {
  nanoclj_val_t x = first(sc, args);
  nanoclj_val_t y = second(sc, args);

  if (is_nil(x) || is_nil(y)) {
    return nanoclj_throw(sc, sc->NullPointerException);
  }
  int tx = type(x), ty = type(y);

  if (tx == T_DOUBLE || ty == T_DOUBLE || ty == T_RATIO) {
    /* default */
  } else {
    long exp = to_long(y);
    if (exp == 0) {
      return mk_int(1);
    } else if (tx == T_RATIO) {
      uint64_t exp2 = llabs(exp);
      nanoclj_cell_t * c = decode_pointer(x);
      int sign = _sign(c) == 1 || (exp & 1) == 0 ? 1 : -1;
      nanoclj_tensor_t * a = tensor_bigint_pow(c->_ratio.numerator, exp2);
      nanoclj_tensor_t * b = tensor_bigint_pow(c->_ratio.denominator, exp2);
      if (exp > 0) {
	return mk_pointer(mk_ratio_from_tensor(sc, sign, a, b));
      } else {
	return mk_pointer(mk_ratio_from_tensor(sc, sign, b, a));
      }
    } else if (tx == T_LONG && ty == T_LONG) {
      long long base = to_long(x);
      long long res;
      if (base == 0 || base == 1) {
	return mk_long(sc, base);
      } else if (exp >= 0 && exp < 256 && !ipow_overflow(base, exp, &res)) {
	return mk_long(sc, res);
      } else if (exp > -256 && exp < 0 && !ipow_overflow(base, -exp, &res)) {
	return mk_pointer(mk_ratio_long(sc, 1, res));
      }
    }
    
    nanoclj_bigint_t base = to_bigint(x);
    nanoclj_tensor_t * t = tensor_bigint_pow(base.tensor, exp);
    if (!base.tensor->refcnt) tensor_free(base.tensor);
    return mk_pointer(mk_bigint_from_tensor(sc, base.sign > 0 || !(exp & 1) ? 1 : -1, t));
  }

  return mk_double(pow(to_double(x), to_double(y)));
}

static nanoclj_val_t numeric_tower_gcd(nanoclj_t * sc, nanoclj_cell_t * args) {
  nanoclj_val_t x = first(sc, args);
  nanoclj_val_t y = second(sc, args);

  if (is_nil(x) || is_nil(y)) {
    return nanoclj_throw(sc, sc->NullPointerException);
  }
  int tx = type(x), ty = type(y);

  if (tx == T_DOUBLE || ty == T_DOUBLE) {
    /* TODO */
  } else if (tx == T_RATIO || ty == T_RATIO) {
    /* TODO */
  } else if (tx == T_BIGINT || ty == T_BIGINT) {
    nanoclj_bigint_t a = to_bigint(x), b = to_bigint(y);
    nanoclj_tensor_t * g = tensor_bigint_gcd(a.tensor, b.tensor);
    if (!a.tensor->refcnt) tensor_free(a.tensor);
    if (!b.tensor->refcnt) tensor_free(b.tensor);
    return mk_pointer(mk_bigint_from_tensor(sc, a.sign == b.sign ? a.sign : 1, g));
  } else {
    return mk_long(sc, gcd_int64(to_long(x), to_long(y)));
  }

  return mk_int(1);
}

static inline nanoclj_val_t browse_url(nanoclj_t * sc, nanoclj_cell_t * args) {
#ifdef WIN32
  char * url = alloc_c_str(to_strview(first(sc, args)));
  ShellExecute(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
  free(url);
  return (nanoclj_val_t)kTRUE;
#else
  pid_t r = fork();
  if (r == 0) {
    char * url = alloc_c_str(to_strview(first(sc, args)));
    const char * cmd = "xdg-open";
    execlp(cmd, cmd, url, NULL);
    exit(1);
  } else {
    return r < 0 ? (nanoclj_val_t)kFALSE : (nanoclj_val_t)kTRUE;
  }
#endif
}

static inline nanoclj_val_t Image_load(nanoclj_t * sc, nanoclj_cell_t * args) {
  nanoclj_val_t src = first(sc, args);
  strview_t sv = to_strview(slurp(sc, T_INPUT_STREAM, args));
  if (sc->pending_exception) return mk_nil();
  
  int w, h, channels;
  uint8_t * data = stbi_load_from_memory((const uint8_t *)sv.ptr, sv.size, &w, &h, &channels, 0);
  if (!data) { /* FIXME: stbi_failure_reason() is not thread-safe */
    strview_t src_sv = to_strview(src);
    return nanoclj_throw(sc, mk_runtime_exception(sc, mk_string_fmt(sc, "%s [%.*s]", stbi_failure_reason(), (int)src_sv.size, src_sv.ptr)));
  }

  nanoclj_cell_t * meta = mk_hashmap(sc);
  meta = assoc(sc, meta, sc->WIDTH, mk_int(w));
  meta = assoc(sc, meta, sc->HEIGHT, mk_int(h));
  if (is_string(src) || is_file(src) || is_url(src)) {
    meta = assoc(sc, meta, sc->FILE_KW, src);
  }
  
  nanoclj_internal_format_t f;
  switch (channels) {
  case 1: f = nanoclj_r8; break;
  case 3: f = nanoclj_rgb8; break;
  case 4: f = nanoclj_rgba8; break;
  default:
    stbi_image_free(data);
    return mk_nil();
  }
  nanoclj_cell_t * image = mk_image(sc, w, h, f, meta);
  memcpy(image->_image.tensor->data, data, w * h * get_format_bpp(f));
  stbi_image_free(data);
  return mk_pointer(image);
}

/* Resizes an image. Only support simple formats r8, rgb8 or rgba8. */
static inline nanoclj_val_t Image_resize(nanoclj_t * sc, nanoclj_cell_t * args) {
  imageview_t iv = to_imageview(first(sc, args));
  nanoclj_val_t target_w0 = second(sc, args), target_h0 = third(sc, args);
  if (!iv.ptr || !is_number(target_w0) || !is_number(target_h0)) {
    return nanoclj_throw(sc, mk_runtime_exception(sc, mk_string(sc, "Invalid type")));
  }

  int target_w = to_int(target_w0), target_h = to_int(target_h0);
  int channels = get_format_channels(iv.format);

  uint32_t stride = iv.stride;
  uint8_t * tmp = NULL;
  nanoclj_internal_format_t f = iv.format;
  if (iv.format == nanoclj_bgr8_32) {
    f = nanoclj_rgb8;
    tmp = convert_imageview(iv, f);
    stride = 3 * iv.width;
  }

  nanoclj_cell_t * target_image = mk_image(sc, target_w, target_h, f, NULL);
  nanoclj_tensor_t * img = target_image->_image.tensor;
  uint8_t * target_ptr = img->data;

  stbir_resize_uint8_generic(tmp ? tmp : iv.ptr, iv.width, iv.height, stride, target_ptr, target_w, target_h, 0, channels, 0, STBIR_FLAG_ALPHA_PREMULTIPLIED, STBIR_EDGE_CLAMP, STBIR_FILTER_DEFAULT, STBIR_COLORSPACE_LINEAR, NULL);

  free(tmp);

  return mk_pointer(target_image);
}

static inline nanoclj_val_t Image_transpose(nanoclj_t * sc, nanoclj_cell_t * args) {
  imageview_t iv = to_imageview(first(sc, args));
  if (!iv.ptr) {
    return nanoclj_throw(sc, mk_runtime_exception(sc, mk_string(sc, "Not an Image")));
  }
  int w = iv.width, h = iv.height, stride = iv.stride;
  nanoclj_cell_t * new_image = mk_image(sc, h, w, iv.format, NULL);
  uint8_t * data = new_image->_image.tensor->data;
  size_t bpp = get_format_bpp(iv.format);
  
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      for (int c = 0; c < bpp; c++) {
	data[bpp * (x * h + y) + c] = iv.ptr[y * stride + x * bpp + c];
      }
    }
  }
  
  return mk_pointer(new_image);
}

static inline nanoclj_val_t Image_save(nanoclj_t * sc, nanoclj_cell_t * args) {
  imageview_t iv = to_imageview(first(sc, args));
  nanoclj_val_t filename0 = second(sc, args);
  if (!iv.ptr) {
    return nanoclj_throw(sc, mk_runtime_exception(sc, mk_string(sc, "Not an Image")));
  } else if (!is_string(filename0)) {
    return nanoclj_throw(sc, mk_runtime_exception(sc, mk_string(sc, "Not a string")));
  }
  char * filename = alloc_c_str(to_strview(filename0));
  char * ext = strrchr(filename, '.');
  if (!ext) {
    free(filename);
    return nanoclj_throw(sc, mk_runtime_exception(sc, mk_string(sc, "Could not determine file format")));
  } else {
    uint8_t * tmp = NULL;
    int w = iv.width, h = iv.height, channels = get_format_channels(iv.format);
    switch (iv.format) {
    case nanoclj_bgra8:
      tmp = convert_imageview(iv, nanoclj_rgba8);
      break;
    case nanoclj_bgr8_32:
      tmp = convert_imageview(iv, nanoclj_rgb8);
      break;
    default:
      break;
    }

    int success = 0;
    if (strcmp(ext, ".png") == 0) {
      success = stbi_write_png(filename, w, h, channels, tmp ? tmp : iv.ptr, iv.stride);
    } else if (strcmp(ext, ".bmp") == 0) {
      success = stbi_write_bmp(filename, w, h, channels, tmp ? tmp : iv.ptr);
    } else if (strcmp(ext, ".tga") == 0) {
      success = stbi_write_tga(filename, w, h, channels, tmp ? tmp : iv.ptr);
    } else if (strcmp(ext, ".jpg") == 0) {
      success = stbi_write_jpg(filename, w, h, channels, tmp ? tmp : iv.ptr, 95);
    } else {
      free(tmp);
      free(filename);
      return nanoclj_throw(sc, mk_runtime_exception(sc, mk_string(sc, "Unsupported file format")));
    }

    free(tmp);
    free(filename);

    if (!success) {
      return nanoclj_throw(sc, mk_runtime_exception(sc, mk_string(sc, "Error writing file")));
    }
  }

  return (nanoclj_val_t)kTRUE;
}

static inline int * mk_kernel(nanoclj_t * sc, float radius, int * size) {
  int r = (int)ceil(radius);

  if (r <= 0) {
    *size = 0;
    return NULL;
  }
  
  int rows = 2 * r + 1;
  float sigma = radius / 3;
  float sigma22 = 2.0f * sigma * sigma;
  float sigmaPi2 = 2.0f * (float)M_PI * sigma;
  float sqrtSigmaPi2 = sqrt(sigmaPi2);

  int * kernel = malloc(rows * sizeof(int));

  int row = -r;
  float first_value = exp(-row*row/sigma22) / sqrtSigmaPi2;
  int i = 0;
  kernel[i++] = 1;

  for ( ; i < rows; row++) {
    kernel[i++] = (int)(exp(-row * row / sigma22) / sqrtSigmaPi2 / first_value);
  }
  *size = rows;
  return kernel;
}

/* Horizontal gaussian blur */
nanoclj_val_t Image_horizontalGaussianBlur(nanoclj_t * sc, nanoclj_cell_t * args) {
  imageview_t iv = to_imageview(first(sc, args));
  nanoclj_val_t radius = second(sc, args);
  if (!iv.ptr) {
    return nanoclj_throw(sc, mk_runtime_exception(sc, mk_string(sc, "Not an Image")));
  }
  if (!is_number(radius)) {
    return nanoclj_throw(sc, mk_runtime_exception(sc, mk_string(sc, "Not a number")));
  }
  int w = iv.width, h = iv.height, stride = iv.stride;
  size_t bpp = get_format_bpp(iv.format);
  int kernel_size;
  int * kernel = mk_kernel(sc, to_double(radius), &kernel_size);

  int total = 0;
  for (int i = 0; i < kernel_size; i++) total += kernel[i];
  
  nanoclj_cell_t * r = mk_image(sc, w, h, iv.format, NULL);
  uint8_t * output_data = r->_image.tensor->data;
    
  switch (iv.format) {
  case nanoclj_rgba8:
  case nanoclj_bgra8:
    for (int row = 0; row < h; row++) {
      for (int col = 0; col < w; col++) {
	int c0 = 0, c1 = 0, c2 = 0, c3 = 0;
	for (int i = 0; i < kernel_size; i++) {
	  const uint8_t * ptr = iv.ptr + row * stride + clamp(col + i - kernel_size / 2, 0, w - 1) * 4;
	  c0 += *ptr++ * kernel[i];
	  c1 += *ptr++ * kernel[i];
	  c2 += *ptr++ * kernel[i];
	  c3 += *ptr++ * kernel[i];
	}
	uint8_t * ptr = output_data + (row * w + col) * 4;
	*ptr++ = (uint8_t)(c0 / total);
	*ptr++ = (uint8_t)(c1 / total);
	*ptr++ = (uint8_t)(c2 / total);
	*ptr++ = (uint8_t)(c3 / total);
      }
    }
    break;
  case nanoclj_rgb8:
  case nanoclj_bgr8_32:
    for (int row = 0; row < h; row++) {
      for (int col = 0; col < w; col++) {
	int c0 = 0, c1 = 0, c2 = 0;
	for (int i = 0; i < kernel_size; i++) {
	  const uint8_t * ptr = iv.ptr + row * stride + clamp(col + i - kernel_size / 2, 0, w - 1) * bpp;
	  c0 += *ptr++ * kernel[i];
	  c1 += *ptr++ * kernel[i];
	  c2 += *ptr++ * kernel[i];
	}
	uint8_t * ptr = output_data + (row * w + col) * bpp;
	*ptr++ = (uint8_t)(c0 / total);
	*ptr++ = (uint8_t)(c1 / total);
	*ptr++ = (uint8_t)(c2 / total);
      }
    }
    break;
  case nanoclj_r8:
    for (int row = 0; row < h; row++) {
      for (int col = 0; col < w; col++) {
	int c0 = 0;
	for (int i = 0; i < kernel_size; i++) {
	  const uint8_t * ptr = iv.ptr + row * stride + clamp(col + i - kernel_size / 2, 0, w - 1);
	  c0 += *ptr * kernel[i];
	}
	uint8_t * ptr = output_data + row * w + col;
	*ptr = (uint8_t)(c0 / total);
      }
    }
    break;
  default:
    r = NULL;
  }
  free(kernel);
  return mk_pointer(r);
}

static inline nanoclj_val_t Audio_load(nanoclj_t * sc, nanoclj_cell_t * args) {
  nanoclj_val_t src = first(sc, args);
  strview_t sv = to_strview(slurp(sc, T_INPUT_STREAM, args));
  if (sc->pending_exception) return mk_nil();
  
  drwav wav;
  if (!drwav_init_memory(&wav, sv.ptr, sv.size, NULL)) {
    return nanoclj_throw(sc, mk_runtime_exception(sc, mk_string(sc, "Failed to load Audio")));
  }

  nanoclj_cell_t * audio = mk_audio(sc, wav.totalPCMFrameCount, wav.channels, wav.sampleRate);
  nanoclj_tensor_t * t = audio->_audio.tensor;
  float * data = t->data;

  /* Read interleaved frames */
  size_t samples_decoded = drwav_read_pcm_frames_f32(&wav, wav.totalPCMFrameCount, data);

  /* TODO: do something if partial read */
  
  drwav_uninit(&wav);
  return mk_pointer(audio);
}

static inline nanoclj_val_t Audio_lowpass(nanoclj_t * sc, nanoclj_cell_t * args) {
  return mk_nil();
}

static inline nanoclj_val_t Geo_load(nanoclj_t * sc, nanoclj_cell_t * args) {
  char * filename = alloc_c_str(to_strview(first(sc, args)));

  SHPHandle shp = SHPOpen(filename, "rb");
  if (!shp) return nanoclj_throw(sc, mk_runtime_exception(sc, mk_string(sc, "Failed to open file")));

  DBFHandle dbf = DBFOpen(filename, "rb");
  if (!dbf) return nanoclj_throw(sc, mk_runtime_exception(sc, mk_string(sc, "Failed to open file")));

  size_t field_count = DBFGetFieldCount(dbf);

  double minb[4], maxb[4];
  int shape_count, default_shape_type;
  SHPGetInfo(shp, &shape_count, &default_shape_type, minb, maxb);

  nanoclj_cell_t * r = NULL;

  nanoclj_val_t type_key = def_keyword(sc, "type");
  nanoclj_val_t geom_key = def_keyword(sc, "geometry");
  nanoclj_val_t prop_key = def_keyword(sc, "properties");
  nanoclj_val_t coord_key = def_keyword(sc, "coordinates");
  nanoclj_val_t bbox_key = def_keyword(sc, "bbox");
  nanoclj_val_t id_key = def_keyword(sc, "id");

  nanoclj_val_t point_id = mk_string(sc, "Point");
  nanoclj_val_t linestring_id = mk_string(sc, "LineString");
  nanoclj_val_t multilinestring_id = mk_string(sc, "MultiLineString");
  nanoclj_val_t multipolygon_id = mk_string(sc, "MultiPolygon");

  retain_value(sc, point_id);
  retain_value(sc, linestring_id);
  retain_value(sc, multilinestring_id);
  retain_value(sc, multipolygon_id);

  for (int i = 0; i < shape_count; i++) {
    SHPObject * o = SHPReadObject(shp, i);

    bool has_z = o->nSHPType != SHPT_POINT && o->nSHPType != SHPT_MULTIPOINT &&
      o->nSHPType != SHPT_ARC && o->nSHPType != SHPT_POLYGON;

    nanoclj_val_t type_id = mk_nil();
    nanoclj_cell_t * coord = NULL;
    
    switch (o->nSHPType) {
    case SHPT_POINT:
    case SHPT_POINTZ:
    case SHPT_POINTM:
    case SHPT_MULTIPOINT:
    case SHPT_MULTIPOINTZ:
    case SHPT_MULTIPOINTM:
      type_id = point_id;
      if (has_z) {
	coord = decode_pointer(mk_vector_3d(sc, o->padfX[0], o->padfY[0], o->padfZ[0]));
      } else {
	coord = decode_pointer(mk_vector_2d(sc, o->padfX[0], o->padfY[0]));
      }
      retain(sc, coord);
      /* TODO: Implement multipoint */    
      break;
      
    case SHPT_ARC: // = polyline
    case SHPT_ARCZ:
    case SHPT_ARCM:
      if (o->nParts == 1) {
	type_id = linestring_id;
	coord = mk_vector(sc, o->nVertices);
	fill_vector(coord, mk_nil());
	retain(sc, coord);
	
	for (int j = 0; j < o->nVertices; j++) {
	  double x = o->padfX[j], y = o->padfY[j], z = o->padfZ[j];
	  if (has_z) {
	    set_indexed_value(coord, j, mk_vector_3d(sc, x, y, z));
	  } else {
	    set_indexed_value(coord, j, mk_vector_2d(sc, x, y));
	  }
	}	
      } else {
	type_id = multilinestring_id;
	coord = mk_vector(sc, o->nParts);
	fill_vector(coord, mk_nil());
	retain(sc, coord);
	for (int j = 0; j < o->nParts; j++) {
	  int start = o->panPartStart[j];
	  int end = j + 1 < o->nParts ? o->panPartStart[j + 1] : o->nVertices;
	  nanoclj_cell_t * part = mk_vector(sc, end - start);
	  fill_vector(part, mk_nil());

	  for (int k = 0; k < end - start; k++) {
	    double x = o->padfX[start + k], y = o->padfY[start + k], z = o->padfZ[start + k];
	    if (has_z) {
	      set_indexed_value(part, k, mk_vector_3d(sc, x, y, z));
	    } else {
	      set_indexed_value(part, k, mk_vector_2d(sc, x, y));
	    }
	  }
	  set_indexed_value(coord, j, mk_pointer(part));
	}
      }
      break;
      
    case SHPT_POLYGON:
    case SHPT_POLYGONZ:
    case SHPT_POLYGONM:
      type_id = multipolygon_id;
      coord = mk_vector(sc, o->nParts);
      fill_vector(coord, mk_nil());
      retain(sc, coord);

      for (int j = 0; j < o->nParts; j++) {
	int start = o->panPartStart[j];
	int end = j + 1 < o->nParts ? o->panPartStart[j + 1] : o->nVertices;
	nanoclj_cell_t * part = mk_vector(sc, end - start);
	fill_vector(part, mk_nil());
	retain(sc, part);
	for (int k = 0; k < end - start; k++) {
	  double x = o->padfX[start + k], y = o->padfY[start + k], z = o->padfZ[start + k];
	  if (has_z) {
	    set_indexed_value(part, k, mk_vector_3d(sc, x, y, z));
	  } else {
	    set_indexed_value(part, k, mk_vector_2d(sc, x, y));
	  }
	}
	set_indexed_value(coord, j, mk_pointer(part));
      }
      break;
    }

    if (is_nil(type_id)) continue;

    nanoclj_cell_t * geom = mk_hashmap(sc);
    retain(sc, geom);
    
    geom = assoc(sc, geom, type_key, type_id);
    geom = assoc(sc, geom, coord_key, mk_pointer(coord));

    nanoclj_cell_t * prop = mk_hashmap(sc);
    retain(sc, prop);

    for (size_t j = 0; j < field_count; j++) {
      if (DBFIsAttributeNULL(dbf, i, j)) continue;

      char name[255];
      int type = DBFGetFieldInfo(dbf, j, name, 0, 0);
      nanoclj_val_t name_v = mk_string(sc, name);

      switch (type) {
      case FTString:
	prop = assoc(sc, prop, name_v, mk_string(sc, DBFReadStringAttribute(dbf, i, j)));
	if (strcmp(name, "NAME_EN") == 0) {
	  fprintf(stderr, "id = %d: %s\n", o->nShapeId, DBFReadStringAttribute(dbf, i, j));
	}
	break;
      case FTInteger:
	prop = assoc(sc, prop, name_v, mk_int(DBFReadIntegerAttribute(dbf, i, j)));
	break;
      case FTDouble:
	prop = assoc(sc, prop, name_v, mk_double(DBFReadDoubleAttribute(dbf, i, j)));
	break;
      case FTLogical:
	prop = assoc(sc, prop, name_v, mk_boolean(DBFReadLogicalAttribute(dbf, i, j)));
	break;
      }
    }

    nanoclj_val_t bbox;
    if (has_z) {
      bbox = mk_vector_6d(sc, o->dfXMin, o->dfYMin, o->dfZMin, o->dfXMax, o->dfYMax, o->dfZMax);
    } else {
      bbox = mk_vector_4d(sc, o->dfXMin, o->dfYMin, o->dfXMax, o->dfYMax);
    }
    retain_value(sc, bbox);
    
    nanoclj_cell_t * feat = mk_hashmap(sc);
    retain(sc, feat);
    
    feat = assoc(sc, feat, type_key, mk_string(sc, "Feature"));
    feat = assoc(sc, feat, geom_key, mk_pointer(geom));
    feat = assoc(sc, feat, prop_key, mk_pointer(prop));
    feat = assoc(sc, feat, bbox_key, bbox);
    feat = assoc(sc, feat, id_key, mk_long(sc, o->nShapeId));

    r = cons(sc, mk_pointer(feat), r);

    SHPDestroyObject(o);
  }
  
  SHPClose(shp);
  DBFClose(dbf);

  return mk_pointer(r);
}

static inline nanoclj_val_t Graph_updateLayout(nanoclj_t * sc, nanoclj_cell_t * args) {
  nanoclj_cell_t * g = decode_pointer(first(sc, args));
  float gravity = 0.075f, friction = 0.9f, charge = -35.0f;
  float alpha = 0.1f;
  
  for (int i = 0; i < 1000; i++) {
    relax_links(g, alpha);
    apply_gravity(g, alpha, gravity);
    apply_repulsion(g, alpha, charge);
    apply_drag(g, friction);
  }
  return mk_nil();
}

static void XMLCDECL silent_error_handler(void *ctx, const char *msg, ...) {
  
}
  
static inline nanoclj_val_t Graph_load(nanoclj_t * sc, nanoclj_cell_t * args) {
  nanoclj_val_t src = first(sc, args);
  strview_t sv = to_strview(slurp(sc, T_READER, args));
  if (sc->pending_exception) return mk_nil();
  
  char * fn = NULL;
  if (is_file(src) || is_string(src)) {
    fn = alloc_c_str(to_strview(src));
  }
  xmlSetGenericErrorFunc(NULL, silent_error_handler);
  xmlDoc * doc = xmlReadMemory(sv.ptr, sv.size, fn, NULL, 0);
  if (doc == NULL) {
    nanoclj_cell_t * e = mk_runtime_exception(sc, mk_string_fmt(sc, "Could not parse GraphML file [%s]", fn ? fn : ""));
    free(fn);
    return nanoclj_throw(sc, e);
  }
  free(fn);

  xmlNode * root = xmlDocGetRootElement(doc);
  xmlNode * node = NULL, * graph_node = NULL;
  for (node = root->children; node; node = node->next) {
    if (node->type == XML_ELEMENT_NODE && strcmp((const char *)node->name, "graph") == 0) {
      graph_node = node;
    } else if (node->type == XML_ELEMENT_NODE && strcmp((const char *)node->name, "key") == 0) {
      /* TODO: Handle key */
    }
  }
  if (!graph_node) return mk_nil();

  nanoclj_graph_array_t * g = mk_graph_array(sc);
  
  for (node = graph_node->children; node; node = node->next) {
    if (node->type == XML_ELEMENT_NODE && strcmp((const char *)node->name, "edge") == 0) {
      nanoclj_val_t source = mk_nil(), target = mk_nil();
      xmlAttr* property = node->properties;
      for (; property; property = property->next) {
	xmlChar * value = xmlNodeListGetString(doc, property->children, 1);
	if (strcmp((const char *)property->name, "source") == 0) {
	  source = mk_string(sc, (const char *)value);
	} else if (strcmp((const char *)property->name, "target") == 0) {
	  target = mk_string(sc, (const char *)value);
	}
	xmlFree(value);
      }
      if (!is_nil(source) && !is_nil(target)) {
	size_t si = find_node_index(sc, g, source), ti = find_node_index(sc, g, target);
	strview_t ssv = to_strview(source), tsv = to_strview(target);
	if (si != NPOS && ti != NPOS) {
	  graph_array_append_edge(sc, g, si, ti);
	}
      }
    } else if (node->type == XML_ELEMENT_NODE && strcmp((const char *)node->name, "node") == 0) {
      nanoclj_val_t id = mk_nil();
      xmlAttr* property = node->properties;
      for (; property; property = property->next) {
	if (strcmp((const char *)property->name, "id") != 0) continue;
	xmlChar * value = xmlNodeListGetString(doc, property->children, 1);
	id = mk_string(sc, (const char *)value);
	xmlFree(value);
      }
      nanoclj_cell_t * attributes = mk_hashmap(sc);
      xmlNode * child = node->children;
      for (; child; child = child->next) {
	if (child->type != XML_ELEMENT_NODE || strcmp((const char *)child->name, "data") != 0) continue;
	xmlAttr * child_property = child->properties;
	nanoclj_val_t key = mk_nil(), value = mk_nil();
	for (; child_property; child_property = child_property->next) {
	  if (strcmp((const char*)child_property->name, "key") == 0) {
	    xmlChar * value = xmlNodeListGetString(doc, child_property->children, 1);
	    key = mk_string(sc, (const char *)value);
	    xmlFree(value);
	  }
	}
	xmlNode * content = child->children;
	if (content && content->type == XML_TEXT_NODE) {
	  value = mk_string(sc, (const char *)content->content);
	}
	if (!is_nil(key)) {
	  attributes = assoc(sc, attributes, key, value);
	}
      }
      if (!is_nil(id)) {
	graph_array_append_node(sc, g, mk_mapentry(sc, id, mk_pointer(attributes)));
      }
    }
  }
  xmlFreeDoc(doc);
  return mk_pointer(mk_graph(sc, T_GRAPH, 0, g->num_nodes, g));
}

static inline nanoclj_val_t create_xml_node(nanoclj_t * sc, xmlNode * input) {
  nanoclj_cell_t * output = 0;
  nanoclj_val_t tag = mk_nil();
  if (input->type == XML_ELEMENT_NODE) {
    nanoclj_cell_t * content = 0, * attrs = 0;
    nanoclj_val_t tag = def_keyword(sc, (const char *)input->name);
    xmlNode * cur_node = input->children;
    for (; cur_node; cur_node = cur_node->next) {
      nanoclj_val_t child = create_xml_node(sc, cur_node);
      if (!is_nil(child)) {
	if (!content) content = mk_vector(sc, 0);
	content = conjoin(sc, content, child);
	retain(sc, content);
      }
    }

    xmlAttr* attribute = input->properties;
    for (; attribute; attribute = attribute->next) {
      xmlChar * value = xmlNodeListGetString(input->doc, attribute->children, 1);
      if (!attrs) {
	attrs = mk_hashmap(sc);
	retain(sc, attrs);
      }
      attrs = assoc(sc, attrs, def_keyword(sc, (const char *)attribute->name), mk_string(sc, (const char *)value));
      retain(sc, attrs);
      xmlFree(value);
    }
    
    output = mk_hashmap(sc);
    retain(sc, output);

    output = assoc(sc, output, def_keyword(sc, "tag"), tag);
    if (content) {
      output = assoc(sc, output, def_keyword(sc, "content"), mk_pointer(content));
    }
    if (attrs) {
      output = assoc(sc, output, def_keyword(sc, "attrs"), mk_pointer(attrs));
    }
  } else if (input->type == XML_TEXT_NODE) {
    const char * text = (const char *)input->content;
    size_t size = strlen(text);
    for (size_t i = 0; i < size; i++) {
      if (!isspace(text[i])) {
	output = get_string_object(sc, T_STRING, text, size, 0);
	retain(sc, output);
	break;
      }
    }
  }
  return mk_pointer(output);
}

static inline nanoclj_val_t clojure_xml_parse(nanoclj_t * sc, nanoclj_cell_t * args) {
  nanoclj_val_t src = first(sc, args);
  strview_t sv = to_strview(slurp(sc, T_READER, args));
  if (sc->pending_exception) return mk_nil();
  
  char * fn = NULL;
  if (is_file(src) || is_string(src)) {
    fn = alloc_c_str(to_strview(src));
  }
  xmlSetGenericErrorFunc(NULL, silent_error_handler);
  xmlDoc * doc = xmlReadMemory(sv.ptr, sv.size, fn, NULL, 0);
  if (doc == NULL) {
    nanoclj_cell_t * e = mk_runtime_exception(sc, mk_string_fmt(sc, "Could not parse xml file [%s]", fn ? fn : ""));
    free(fn);
    return nanoclj_throw(sc, e);
  }
  free(fn);
  nanoclj_val_t xml = create_xml_node(sc, xmlDocGetRootElement(doc));
  xmlFreeDoc(doc);
  return xml;
}

static inline nanoclj_val_t clojure_data_csv_read_csv(nanoclj_t * sc, nanoclj_cell_t * args) {
  nanoclj_val_t f = first(sc, args);
  if (is_nil(f)) {
    return nanoclj_throw(sc, sc->NullPointerException);
  }
  nanoclj_cell_t * rdr = decode_pointer(f);
  if (_type(rdr) != T_READER) {
    return nanoclj_throw(sc, mk_runtime_exception(sc, mk_string(sc, "Argument is not a reader")));
  }

  bool is_quoted = false;
  nanoclj_tensor_t * vec = NULL, * value = NULL;
  int32_t delimiter = ',';
  
  while ( 1 ) {
    int32_t c = inchar(sc, rdr);
    if (c == -1) break;
    if (c == '\r') continue;
    if (!is_quoted) {
      if (!vec) vec = mk_tensor_1d(nanoclj_val, 0);
      if (c == '\n') {
	break;
      } else {
	if (!value) value = mk_tensor_1d(nanoclj_i8, 0);
	if (c == '"') {
	  is_quoted = true;
	} else if (c == delimiter) {
	  tensor_mutate_push(vec, mk_string_with_tensor(sc, value));
	  value = mk_tensor_1d(nanoclj_i8, 0);
	} else {
	  tensor_mutate_append_codepoint(value, c);
	}
      }
    } else if (c == '"') {
      is_quoted = false;
    } else {
      tensor_mutate_append_codepoint(value, c);
    }
  }

  if (vec) {
    if (value) tensor_mutate_push(vec, mk_string_with_tensor(sc, value));
    nanoclj_val_t next_row = mk_foreign_func(sc, clojure_data_csv_read_csv, 1, 1, NULL);
    nanoclj_cell_t * code = cons(sc, mk_pointer(cons(sc, next_row, cons(sc, mk_pointer(rdr), NULL))), NULL);
    nanoclj_cell_t * lazy_seq = get_cell(sc, T_LAZYSEQ, 0, mk_pointer(cons(sc, mk_emptylist(), code)), sc->envir, NULL);
    return mk_pointer(cons(sc, mk_vector_with_tensor(sc, vec), lazy_seq));
  } else {
    return mk_emptylist();
  }
}

#if NANOCLJ_USE_LINENOISE
static nanoclj_t * linenoise_sc = NULL;

static inline void completion(const char *input, linenoiseCompletions *lc) {
  nanoclj_val_t ns = resolve_value(linenoise_sc, linenoise_sc->envir, def_symbol(linenoise_sc, "clojure.repl"));
  if (is_cell(ns)) {
    nanoclj_cell_t * var = get_var_in_ns(linenoise_sc, decode_pointer(ns), linenoise_sc->AUTOCOMPLETE_HOOK, false);
    if (var) {
      nanoclj_val_t f = get_indexed_value(var, 1);
      nanoclj_val_t args = mk_pointer(cons(linenoise_sc, mk_string(linenoise_sc, input), NULL));
      nanoclj_val_t r0 = nanoclj_call(linenoise_sc, f, args);
      if (linenoise_sc->pending_exception) {
	linenoiseTerminate();
      } else if (is_cell(r0)) {
	for (nanoclj_cell_t * r = decode_pointer(r0); r; r = next(linenoise_sc, r)) {
	  strview_t sv = to_strview(first(linenoise_sc, r));
	  linenoiseAddCompletion(lc, sv.ptr, sv.size);
	}
      }
    }
  }
}

static inline char *complete_parens(const char * input) {
  char nest[1024];
  int si = 0;
  for (int i = 0, n = strlen(input); i < n; i++) {
    if (input[i] == '\\') {
      i++;
    } else if (input[i] == '"') {
      if (si != 0 && nest[si - 1] == '"') {
	si--;
      } else {
	nest[si++] = '"';
      }
    } else if (si == 0 || nest[si - 1] != '"') {
      if (input[i] == '(') {
	nest[si++] = ')';
      } else if (input[i] == '[') {
	nest[si++] = ']';
      } else if (input[i] == '{') {
	nest[si++] = '}';
      } else if (input[i] == ')' || input[i] == ']' || input[i] == '}') {
	if (si > 0 && nest[si - 1] == input[i]) {
	  si--;
	} else {
	  return NULL;
	}
      }
    }
  }
  if (si == 0) {
    return NULL;
  }
  char * h = malloc(si + 1);
  int hi = 0;
  for (int i = si - 1; i >= 0; i--) {
    h[hi++] = nest[i];
  }
  h[hi] = 0;

  return h;
}

static char *hints_callback(const char *input, int *color, int *bold) {
  char * h = complete_parens(input);
  if (!h) return NULL;
  
  *color = 35;
  *bold = 0;
  return h;
}

static bool * errorcheck_callback(const char * buf, size_t len) {
  bool * errorvec = malloc(len * sizeof(bool));
  
  memset(errorvec, 0, len * sizeof(bool));
  /* find single backslashes */
  for (int i = 0; i < len; i++) {
    if (buf[i] == '\\') {
      if (i + 1 == len || isspace(buf[i + 1])) {
	errorvec[i] = true;
      }
      i++;
    }
  }
  /* find unmatched groupings */
  char nesting[1024];
  int ni = 0;
  for (int i = 0; i < len; i++) {
    char c = buf[i];
    if (c == '\\') {
      i++;
    } else if (c == '"') {
      if (!ni || nesting[ni-1] != '"') {
	nesting[ni++] = '"';
      } else {
	ni--;
      }
    } else if (!ni || nesting[ni-1] != '"') {
      if (c == ')' || c == ']' || c == '}') {
	if (ni && nesting[ni-1] == c) ni--;
	else errorvec[i] = true;
      } else if (ni < 1024) {
	switch (c) {
	case '(': nesting[ni++] = ')'; break;
	case '[': nesting[ni++] = ']'; break;
	case '{': nesting[ni++] = '}'; break;
	}
      }
    }
  }
  return errorvec;
}

static inline void mouse_motion_callback(int x, int y) {
  double f = linenoise_sc->window_scale_factor;
  nanoclj_val_t p = mk_vector_2d(linenoise_sc, x / f, y / f);
  intern(linenoise_sc, linenoise_sc->core_ns, linenoise_sc->MOUSE_POS, p);
  if (linenoise_sc->pending_exception) {
    linenoiseTerminate();
  }
}

static inline void windowsize_callback() {
  update_window_info(linenoise_sc, decode_pointer(get_out_port(linenoise_sc)));
  if (linenoise_sc->pending_exception) {
    linenoiseTerminate();
  }
}

static inline void init_linenoise(nanoclj_t * sc) {
  linenoise_sc = sc;
  linenoiseSetMultiLine(0);
  linenoiseSetClearOutput(1);
  linenoiseSetupSigWinchHandler();
  linenoiseSetCompletionCallback(completion);
  linenoiseSetHintsCallback(hints_callback);
  linenoiseSetFreeCallback(free);
#if 0
  linenoiseSetMouseMotionCallback(mouse_motion_callback);
#endif
  linenoiseSetErrorCheckCallback(errorcheck_callback);
  linenoiseSetWindowSizeCallback(windowsize_callback);
  linenoiseHistorySetMaxLen(10000);
}

static inline nanoclj_val_t linenoise_readline(nanoclj_t * sc, nanoclj_cell_t * args) {
  strview_t prompt = to_strview(first(sc, args));
  char * line = linenoise(prompt.ptr, prompt.size);
  if (line == NULL) return mk_nil();
  
  nanoclj_val_t r;
  char * missing_parens = complete_parens(line);
  if (!missing_parens) {
    r = mk_string(sc, line);
  } else {
    int l1 = strlen(line), l2 = strlen(missing_parens);
    char * tmp = malloc(l1 + l2 + 1);
    strcpy(tmp, line);
    strcpy(tmp + l1, missing_parens);
    free(missing_parens);
    r = mk_string(sc, tmp);
    free(tmp);
  }
  free(line);

  return r;
}

static inline nanoclj_val_t linenoise_history_load(nanoclj_t * sc, nanoclj_cell_t * args) {
  char * fn = alloc_c_str(to_strview(first(sc, args)));
  linenoiseHistoryLoad(fn);
  free(fn);
  return mk_nil();
}

static inline nanoclj_val_t linenoise_history_append(nanoclj_t * sc, nanoclj_cell_t * args) {
  char * line = alloc_c_str(to_strview(first(sc, args)));
  linenoiseHistoryAdd(line);
  args = next(sc, args);
  if (args) {
    char * fn = alloc_c_str(to_strview(first(sc, args)));
    linenoiseHistorySave(fn, line);
    free(fn);
  }
  free(line);
  return mk_nil();
}

#endif

static inline void register_functions(nanoclj_t * sc) {
  nanoclj_cell_t * Thread = mk_class_with_fn(sc, "java.lang.Thread", gentypeid(sc), sc->Object, __FILE__);
  nanoclj_cell_t * System = mk_class_with_fn(sc, "java.lang.System", gentypeid(sc), sc->Object, __FILE__);
  nanoclj_cell_t * Math = mk_class_with_fn(sc, "java.lang.Math", gentypeid(sc), sc->Object, __FILE__);
  
  nanoclj_cell_t * numeric_tower = def_namespace(sc, "clojure.math.numeric-tower", __FILE__);
  nanoclj_cell_t * browse = def_namespace(sc, "clojure.java.browse", __FILE__);
  
  nanoclj_cell_t * Geo = def_namespace(sc, "Geo", __FILE__);
  nanoclj_cell_t * xml = def_namespace(sc, "clojure.xml", __FILE__);
  nanoclj_cell_t * csv = def_namespace(sc, "clojure.data.csv", __FILE__);

  intern_foreign_func(sc, Thread, "sleep", Thread_sleep, 1, 1);
  
  intern_foreign_func(sc, System, "exit", System_exit, 1, 1);
  intern_foreign_func(sc, System, "currentTimeMillis", System_currentTimeMillis, 0, 0);
  intern_foreign_func(sc, System, "nanoTime", System_nanoTime, 0, 0);
  intern_foreign_func(sc, System, "gc", System_gc, 0, 0);
  intern_foreign_func(sc, System, "getenv", System_getenv, 0, 1);
  intern_foreign_func(sc, System, "getProperty", System_getProperty, 0, 1);
  intern_foreign_func(sc, System, "setProperty", System_setProperty, 2, 2);
  intern_foreign_func(sc, System, "glob", System_glob, 1, 1);
  intern_foreign_func(sc, System, "getSystemTimes", System_getSystemTimes, 0, 0);
  intern_foreign_func(sc, Math, "abs", Math_abs, 1, 1);
  intern_foreign_func(sc, Math, "sin", Math_sin, 1, 1);
  intern_foreign_func(sc, Math, "cos", Math_cos, 1, 1);
  intern_foreign_func(sc, Math, "exp", Math_exp, 1, 1);
  intern_foreign_func(sc, Math, "log", Math_log, 1, 1);
  intern_foreign_func(sc, Math, "log10", Math_log10, 1, 1);
  intern_foreign_func(sc, Math, "tan", Math_tan, 1, 1);
  intern_foreign_func(sc, Math, "asin", Math_asin, 1, 1);
  intern_foreign_func(sc, Math, "acos", Math_acos, 1, 1);
  intern_foreign_func(sc, Math, "atan", Math_atan, 1, 2);
  intern_foreign_func(sc, Math, "sqrt", Math_sqrt, 1, 1);
  intern_foreign_func(sc, Math, "cbrt", Math_cbrt, 1, 1);
  intern_foreign_func(sc, Math, "pow", Math_pow, 2, 2);
  intern_foreign_func(sc, Math, "floor", Math_floor, 1, 1);
  intern_foreign_func(sc, Math, "ceil", Math_ceil, 1, 1);
  intern_foreign_func(sc, Math, "round", Math_round, 1, 1);
  intern_foreign_func(sc, Math, "random", Math_random, 0, 0);
  
  intern(sc, Math, def_symbol(sc, "E"), mk_double(M_E));
  intern(sc, Math, def_symbol(sc, "PI"), mk_double(M_PI));

  intern_foreign_func(sc, sc->Double, "doubleToLongBits", Double_doubleToLongBits, 1, 1);
  intern_foreign_func(sc, sc->Double, "doubleToRawLongBits", Double_doubleToRawLongBits, 1, 1);
  
  intern_foreign_func(sc, sc->SecureRandom, "nextBytes", SecureRandom_nextBytes, 2, 2);

  intern_foreign_func(sc, numeric_tower, "expt", numeric_tower_expt, 2, 2);
  intern_foreign_func(sc, numeric_tower, "gcd", numeric_tower_gcd, 2, 2);

  intern_foreign_func(sc, browse, "browse-url", browse_url, 1, 1);

  intern_foreign_func(sc, sc->Image, "load", Image_load, 1, 1);
  intern_foreign_func(sc, sc->Image, "resize", Image_resize, 3, 3);
  intern_foreign_func(sc, sc->Image, "transpose", Image_transpose, 1, 1);
  intern_foreign_func(sc, sc->Image, "save", Image_save, 2, 2);
  intern_foreign_func(sc, sc->Image, "horizontalGaussianBlur", Image_horizontalGaussianBlur, 2, 2);

  intern_foreign_func(sc, sc->Audio, "load", Audio_load, 1, 1);
  intern_foreign_func(sc, sc->Audio, "lowpass", Audio_lowpass, 1, 1);

  intern_foreign_func(sc, Geo, "load", Geo_load, 1, 1);

  intern_foreign_func(sc, sc->Graph, "updateLayout", Graph_updateLayout, 1, 1);
  intern_foreign_func(sc, sc->Graph, "load", Graph_load, 1, 1);

  intern_foreign_func(sc, xml, "parse", clojure_xml_parse, 1, 1);
  intern_foreign_func(sc, csv, "read-csv", clojure_data_csv_read_csv, 1, 1);

#if NANOCLJ_USE_LINENOISE
  nanoclj_cell_t * linenoise = def_namespace(sc, "linenoise", __FILE__);
  intern_foreign_func(sc, linenoise, "read-line", linenoise_readline, 1, 1);
  intern_foreign_func(sc, linenoise, "history-load", linenoise_history_load, 1, 1);
  intern_foreign_func(sc, linenoise, "history-append", linenoise_history_append, 1, 2);

  init_linenoise(sc);
#endif
}

#endif
