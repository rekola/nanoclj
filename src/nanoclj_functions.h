#ifndef _NANOCLJ_FUNCTIONS_H_
#define _NANOCLJ_FUNCTIONS_H_

#include <stdint.h>
#include <ctype.h>
#include <shapefil.h>
#include <libxml/tree.h>
#include <libxml/parser.h>

#include "linenoise.h"
#include "nanoclj_utils.h"

#ifdef WIN32

#include <direct.h>
#include <processthreadsapi.h>

#define getcwd _getcwd
#define environ _environ
#define STBIW_WINDOWS_UTF8

#else

#include <glob.h>
#include <unistd.h>

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

static nanoclj_val_t Thread_sleep(nanoclj_t * sc, nanoclj_val_t args) {
  long long ms = to_long(first(sc, decode_pointer(args)));
  if (ms > 0) {
    nanoclj_sleep(ms);
  }
  return mk_nil();
}

/* System */

static nanoclj_val_t System_exit(nanoclj_t * sc, nanoclj_val_t args) {
  exit(to_int(first(sc, decode_pointer(args))));
}

static nanoclj_val_t System_currentTimeMillis(nanoclj_t * sc, nanoclj_val_t args) {
  return mk_integer(sc, system_time() / 1000);
}

static nanoclj_val_t System_nanoTime(nanoclj_t * sc, nanoclj_val_t args) {
  return mk_integer(sc, 1000 * system_time());
}

static nanoclj_val_t System_gc(nanoclj_t * sc, nanoclj_val_t args) {
  gc(sc, NULL, NULL, NULL);
  return (nanoclj_val_t)kTRUE;
}

static nanoclj_val_t System_getenv(nanoclj_t * sc, nanoclj_val_t args0) {
  nanoclj_cell_t * args = decode_pointer(args0);
  if (args) {
    char * name = alloc_c_str(sc, to_strview(first(sc, args)));
    const char * v = getenv(name);
    sc->free(name);
    return v ? mk_string(sc, v) : mk_nil();
  } else {
    extern char **environ;
    
    int l = 0;
    for ( ; environ[l]; l++) { }
    nanoclj_cell_t * map = mk_arraymap(sc, l);
    fill_vector(map, mk_nil());

    for (int i = 0; i < l; i++) {
      char * p = environ[i];
      int j = 0;
      for (; p[j] != '=' && p[j] != 0; j++) {
	
      }
      nanoclj_val_t key = mk_string_from_sv(sc, (strview_t){ p, j });
      nanoclj_val_t val = p[j] == '=' ? mk_string(sc, p + j + 1) : mk_nil();
      set_vector_elem(map, i, mk_mapentry(sc, key, val));
    }

    return mk_pointer(map);
  }
}

static nanoclj_val_t System_getProperty(nanoclj_t * sc, nanoclj_val_t args0) {
  nanoclj_cell_t * args = decode_pointer(args0);
  if (args) {
    nanoclj_val_t v;
    if (get_elem(sc, sc->context->properties, first(sc, args), &v)) {
      return v;
    } else {
      return mk_nil();
    }
  } else {
    return mk_pointer(sc->context->properties);
  }
}

static nanoclj_val_t System_setProperty(nanoclj_t * sc, nanoclj_val_t args0) {
  nanoclj_cell_t * args = decode_pointer(args0);
  nanoclj_val_t key = first(sc, args);
  nanoclj_val_t val = second(sc, args);

  /* TODO: Implement */

  return mk_nil();  
}

static inline nanoclj_val_t System_glob(nanoclj_t * sc, nanoclj_val_t args0) {
  nanoclj_cell_t * args = decode_pointer(args0);

#ifndef WIN32
  char * tmp = alloc_c_str(sc, to_strview(first(sc, args)));
  glob_t gstruct;
  int r = glob(tmp, GLOB_ERR, NULL, &gstruct);
  nanoclj_cell_t * x = NULL;
  if (r == 0) {
    for (char ** found = gstruct.gl_pathv; *found; found++) {
      nanoclj_cell_t * str = get_string_object(sc, T_FILE, *found, strlen(*found), 0);
      x = cons(sc, mk_pointer(str), x);
    }
  }
  sc->free(tmp);
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
static nanoclj_val_t System_getSystemTimes(nanoclj_t * sc, nanoclj_val_t args) {
  nanoclj_cell_t * r = NULL;
#ifdef WIN32
  FILETIME idleTime, kernelTime, userTime;
  GetSystemTimes(&idleTime, &kernelTime, &userTime);
  long long idle = filetime_to_msec(idleTime);
  r = mk_vector(sc, 3);
  set_vector_elem(r, 0, mk_integer(sc, idle));
  set_vector_elem(r, 1, mk_integer(sc, filetime_to_msec(kernelTime) - idle));
  set_vector_elem(r, 2, mk_integer(sc, filetime_to_msec(userTime)));
#else
  char * b = alloc_c_str(sc, to_strview(slurp(sc, T_READER, cons(sc, mk_string(sc, "/proc/stat"), NULL))));
  const char * p;
  if (strncmp(b, "cpu ", 4) == 0) p = b;
  else p = strstr(b, "\ncpu ");
  if (p) {
    long long user, nice, system, idle;
    if (sscanf(p, "cpu  %lld %lld %lld %lld", &user, &nice, &system, &idle) == 4) {
      r = mk_vector(sc, 3);
      set_vector_elem(r, 0, mk_integer(sc, idle));
      set_vector_elem(r, 1, mk_integer(sc, system));
      set_vector_elem(r, 2, mk_integer(sc, user));
    }
    sc->free(b);
  }
#endif
  return mk_pointer(r);
}

/* Math */

static nanoclj_val_t Math_sin(nanoclj_t * sc, nanoclj_val_t args) {
  return mk_real(sin(to_double(first(sc, decode_pointer(args)))));
}

static nanoclj_val_t Math_cos(nanoclj_t * sc, nanoclj_val_t args) {
  return mk_real(cos(to_double(first(sc, decode_pointer(args)))));
}

static nanoclj_val_t Math_exp(nanoclj_t * sc, nanoclj_val_t args) {
  return mk_real(exp(to_double(first(sc, decode_pointer(args)))));
}

static nanoclj_val_t Math_log(nanoclj_t * sc, nanoclj_val_t args) {
  return mk_real(log(to_double(first(sc, decode_pointer(args)))));
}

static nanoclj_val_t Math_log10(nanoclj_t * sc, nanoclj_val_t args) {
  return mk_real(log10(to_double(first(sc, decode_pointer(args)))));
}

static nanoclj_val_t Math_tan(nanoclj_t * sc, nanoclj_val_t args) {
  return mk_real(tan(to_double(first(sc, decode_pointer(args)))));
}

static nanoclj_val_t Math_asin(nanoclj_t * sc, nanoclj_val_t args) {
  return mk_real(asin(to_double(first(sc, decode_pointer(args)))));
}

static nanoclj_val_t Math_acos(nanoclj_t * sc, nanoclj_val_t args) {
  return mk_real(acos(to_double(first(sc, decode_pointer(args)))));
}

static nanoclj_val_t Math_atan(nanoclj_t * sc, nanoclj_val_t args0) {
  nanoclj_cell_t * args = decode_pointer(args0);
  nanoclj_val_t x = first(sc, args);
  args = next(sc, args);
  if (args) {
    nanoclj_val_t y = first(sc, args);
    return mk_real(atan2(to_double(x), to_double(y)));
  } else {
    return mk_real(atan(to_double(x)));
  }
}

static nanoclj_val_t Math_sqrt(nanoclj_t * sc, nanoclj_val_t args) {
  return mk_real(sqrt(to_double(first(sc, decode_pointer(args)))));
}

static nanoclj_val_t Math_cbrt(nanoclj_t * sc, nanoclj_val_t args) {
  return mk_real(cbrt(to_double(first(sc, decode_pointer(args)))));
}

static nanoclj_val_t Math_pow(nanoclj_t * sc, nanoclj_val_t args0) {
  nanoclj_cell_t * args = decode_pointer(args0);
  return mk_real(pow(to_double(first(sc, args)), to_double(second(sc, args))));
}

static nanoclj_val_t Math_floor(nanoclj_t * sc, nanoclj_val_t args) {
  return mk_real(floor(to_double(first(sc, decode_pointer(args)))));
}

static nanoclj_val_t Math_ceil(nanoclj_t * sc, nanoclj_val_t args) {
  return mk_real(ceil(to_double(first(sc, decode_pointer(args)))));
}

static nanoclj_val_t Math_round(nanoclj_t * sc, nanoclj_val_t args) {
  return mk_real(round(to_double(first(sc, decode_pointer(args)))));
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

static nanoclj_val_t numeric_tower_expt(nanoclj_t * sc, nanoclj_val_t args0) {
  nanoclj_cell_t * args = decode_pointer(args0);
  nanoclj_val_t x = first(sc, args);
  nanoclj_val_t y = second(sc, args);

  if (is_nil(x) || is_nil(y)) {
    return nanoclj_throw(sc, sc->NullPointerException);
  }
  
  int tx = type(x), ty = type(y);

  if (tx == T_RATIO && ty == T_LONG) {
    long exp = to_long(y);
    if (exp > -256 && exp < 256) {
      long long den;
      long long num = get_ratio(x, &den);
      if (exp < 0) {
	exp = -exp;
	long long tmp = den;
	den = num;
	num = tmp;
      }
      long long num2, den2;
      if (!ipow_overflow(num, exp, &num2) && !ipow_overflow(den, exp, &den2)) {
	if (den2 == 1) return mk_integer(sc, num2);
	else return mk_ratio_long(sc, num2, den2);
      }
    }
  } else if (tx == T_LONG && ty == T_LONG) {
    long long base = to_long(x), exp = to_long(y);
    long long res;
    if (base == 0 || base == 1) {
      return mk_integer(sc, base);
    } else if (exp >= 0 && exp < 256 && !ipow_overflow(base, exp, &res)) {
      return mk_integer(sc, res);
    } else if (exp > -256 && exp < 0 && !ipow_overflow(base, -exp, &res)) {
      return mk_ratio_long(sc, 1, res);
    }
  }

  return mk_real(pow(to_double(x), to_double(y)));
}

static inline nanoclj_val_t browse_url(nanoclj_t * sc, nanoclj_val_t args0) {
  nanoclj_cell_t * args = decode_pointer(args0);
#ifdef WIN32
  char * url = alloc_c_str(sc, to_strview(first(sc, args)));
  ShellExecute(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
  sc->free(url);
  return (nanoclj_val_t)kTRUE;
#else
  pid_t r = fork();
  if (r == 0) {
    char * url = alloc_c_str(sc, to_strview(first(sc, args)));
    const char * cmd = "xdg-open";
    execlp(cmd, cmd, url, NULL);
    exit(1);
  } else {
    return r < 0 ? (nanoclj_val_t)kFALSE : (nanoclj_val_t)kTRUE;
  }
#endif
}

static inline nanoclj_val_t Image_load(nanoclj_t * sc, nanoclj_val_t args0) {
  nanoclj_cell_t * args = decode_pointer(args0);
  nanoclj_val_t src = first(sc, args);
  strview_t sv = to_strview(slurp(sc, T_INPUT_STREAM, args));
  
  int w, h, channels;
  uint8_t * data = stbi_load_from_memory((const uint8_t *)sv.ptr, sv.size, &w, &h, &channels, 0);
  if (!data) { /* FIXME: stbi_failure_reason() is not thread-safe */
    strview_t src_sv = to_strview(src);
    return nanoclj_throw(sc, mk_runtime_exception(sc, mk_string_fmt(sc, "%s [%.*s]", stbi_failure_reason(), (int)src_sv.size, src_sv.ptr)));
  }

  nanoclj_cell_t * meta = mk_arraymap(sc, 3);
  set_vector_elem(meta, 0, mk_mapentry(sc, sc->WIDTH, mk_int(w)));
  set_vector_elem(meta, 1, mk_mapentry(sc, sc->HEIGHT, mk_int(h)));
  set_vector_elem(meta, 2, mk_mapentry(sc, sc->FILE, src));

  nanoclj_internal_format_t f;
  switch (channels) {
  case 1: f = nanoclj_r8; break;
  case 3: f = nanoclj_rgb8; break;
  case 4: f = nanoclj_rgba8; break;
  default:
    stbi_image_free(data);
    return mk_nil();
  }
  nanoclj_val_t image = mk_image(sc, w, h, w, f, meta);
  memcpy(image_unchecked(image)->data, data, w * h * channels);
    
  stbi_image_free(data);
  return image;
}

static inline nanoclj_val_t Image_resize(nanoclj_t * sc, nanoclj_val_t args0) {
  nanoclj_cell_t * args = decode_pointer(args0);
  imageview_t iv = to_imageview(first(sc, args));
  nanoclj_val_t target_w0 = second(sc, args), target_h0 = third(sc, args);
  if (!iv.ptr || !is_number(target_w0) || !is_number(target_h0)) {
    return nanoclj_throw(sc, mk_runtime_exception(sc, mk_string(sc, "Invalid type")));
  }
  
  int target_w = to_int(target_w0), target_h = to_int(target_h0);
  int channels = get_format_channels(iv.format);
  int bpp = get_format_bpp(iv.format);
  size_t target_size = target_w * target_h * bpp;

  nanoclj_val_t target_image = mk_image(sc, target_w, target_h, target_w, iv.format, NULL);

  nanoclj_image_t * img = image_unchecked(target_image);
  uint8_t * target_ptr = img->data;
  
  stbir_resize_uint8_generic(iv.ptr, iv.width, iv.height, 0, target_ptr, target_w, target_h, 0, channels, 0, STBIR_FLAG_ALPHA_PREMULTIPLIED, STBIR_EDGE_CLAMP, STBIR_FILTER_DEFAULT, STBIR_COLORSPACE_LINEAR, NULL);
    
  return target_image;
}

static inline nanoclj_val_t Image_transpose(nanoclj_t * sc, nanoclj_val_t args0) {
  nanoclj_cell_t * args = decode_pointer(args0);
  imageview_t iv = to_imageview(first(sc, args));
  if (!iv.ptr) {
    return nanoclj_throw(sc, mk_runtime_exception(sc, mk_string(sc, "Not an Image")));
  }
  int w = iv.width, h = iv.height;
  nanoclj_val_t new_image = mk_image(sc, h, w, h, iv.format, NULL);
  uint8_t * data = image_unchecked(new_image)->data;
  size_t bpp = get_format_bpp(iv.format);
  
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      for (int c = 0; c < bpp; c++) {
	data[bpp * (x * h + y) + c] = iv.ptr[bpp * (y * w + x) + c];
      }
    }
  }
  
  return new_image;
}

static inline nanoclj_val_t Image_save(nanoclj_t * sc, nanoclj_val_t args0) {
  nanoclj_cell_t * args = decode_pointer(args0);
  imageview_t iv = to_imageview(first(sc, args));
  nanoclj_val_t filename0 = second(sc, args);
  if (!iv.ptr) {
    return nanoclj_throw(sc, mk_runtime_exception(sc, mk_string(sc, "Not an Image")));
  }
  if (!is_string(filename0)) {
    return nanoclj_throw(sc, mk_runtime_exception(sc, mk_string(sc, "Not a string")));
  }
  char * filename = alloc_c_str(sc, to_strview(filename0));

  int w = iv.width, h = iv.height;
  int channels = get_format_channels(iv.format);
  const uint8_t * data = iv.ptr;
  
  char * ext = strrchr(filename, '.');
  if (!ext) {
    sc->free(filename);
    return nanoclj_throw(sc, mk_runtime_exception(sc, mk_string(sc, "Could not determine file format")));
  } else {
    int success = 0;
    if (strcmp(ext, ".png") == 0) {
      success = stbi_write_png(filename, w, h, channels, data, w);
    } else if (strcmp(ext, ".bmp") == 0) {
      success = stbi_write_bmp(filename, w, h, channels, data);
    } else if (strcmp(ext, ".tga") == 0) {
      success = stbi_write_tga(filename, w, h, channels, data);
    } else if (strcmp(ext, ".jpg") == 0) {
      success = stbi_write_jpg(filename, w, h, channels, data, 95);
    } else {
      sc->free(filename);
      return nanoclj_throw(sc, mk_runtime_exception(sc, mk_string(sc, "Unsupported file format")));
    }

    sc->free(filename);

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

  int * kernel = (int*)sc->malloc(rows * sizeof(int));

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
nanoclj_val_t Image_horizontalGaussianBlur(nanoclj_t * sc, nanoclj_val_t args0) {
  nanoclj_cell_t * args = decode_pointer(args0);
  imageview_t iv = to_imageview(first(sc, args));
  nanoclj_val_t radius = second(sc, args);
  if (!iv.ptr) {
    return nanoclj_throw(sc, mk_runtime_exception(sc, mk_string(sc, "Not an Image")));
  }
  if (!is_number(radius)) {
    return nanoclj_throw(sc, mk_runtime_exception(sc, mk_string(sc, "Not a number")));
  }
  int w = iv.width, h = iv.height;
  int channels = get_format_channels(iv.format);
  size_t bpp = get_format_bpp(iv.format);
    
  int kernel_size;
  int * kernel = mk_kernel(sc, to_double(radius), &kernel_size);

  int total = 0;
  for (int i = 0; i < kernel_size; i++) total += kernel[i];
  
  nanoclj_val_t r = mk_image(sc, w, h, w, iv.format, NULL);
  uint8_t * output_data = image_unchecked(r)->data;
    
  switch (channels) {
  case 4:
    for (int row = 0; row < h; row++) {
      for (int col = 0; col < w; col++) {
	int c0 = 0, c1 = 0, c2 = 0, c3 = 0;
	for (int i = 0; i < kernel_size; i++) {
	  const uint8_t * ptr = iv.ptr + (row * w + clamp(col + i - kernel_size / 2, 0, w - 1)) * 4;
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
  case 3:
    for (int row = 0; row < h; row++) {
      for (int col = 0; col < w; col++) {
	int c0 = 0, c1 = 0, c2 = 0;
	for (int i = 0; i < kernel_size; i++) {
	  const uint8_t * ptr = iv.ptr + (row * w + clamp(col + i - kernel_size / 2, 0, w - 1)) * bpp;
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
  case 1:      
    for (int row = 0; row < h; row++) {
      for (int col = 0; col < w; col++) {
	int c0 = 0;
	for (int i = 0; i < kernel_size; i++) {
	  const uint8_t * ptr = iv.ptr + (row * w + clamp(col + i - kernel_size / 2, 0, w - 1));
	  c0 += *ptr * kernel[i];
	}
	uint8_t * ptr = output_data + row * w + col;
	*ptr = (uint8_t)(c0 / total);
      }
    }
  }
  sc->free(kernel);
  return r;
}

static inline nanoclj_val_t Audio_load(nanoclj_t * sc, nanoclj_val_t args0) {
  nanoclj_cell_t * args = decode_pointer(args0);
  nanoclj_val_t src = first(sc, args);
  strview_t sv = to_strview(slurp(sc, T_INPUT_STREAM, args));

  drwav wav;
  if (!drwav_init_memory(&wav, sv.ptr, sv.size, NULL)) {
    return nanoclj_throw(sc, mk_runtime_exception(sc, mk_string(sc, "Failed to load Audio")));
  }

  nanoclj_val_t audio = mk_audio(sc, wav.totalPCMFrameCount, wav.channels, wav.sampleRate);
  nanoclj_audio_t * a = audio_unchecked(audio);
  float * data = a->data;
  
  /* Read interleaved frames */
  size_t samples_decoded = drwav_read_pcm_frames_f32(&wav, wav.totalPCMFrameCount, data);

  /* TODO: do something if partial read */
  
  drwav_uninit(&wav);
  
  return audio;
}

static inline nanoclj_val_t Audio_lowpass(nanoclj_t * sc, nanoclj_val_t args) {
  return mk_nil();
}

static inline nanoclj_val_t Geo_load(nanoclj_t * sc, nanoclj_val_t args0) {
  nanoclj_cell_t * args = decode_pointer(args0);
  char * filename = alloc_c_str(sc, to_strview(first(sc, args)));

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
	    set_vector_elem(coord, j, mk_vector_3d(sc, x, y, z));
	  } else {
	    set_vector_elem(coord, j, mk_vector_2d(sc, x, y));
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
	      set_vector_elem(part, k, mk_vector_3d(sc, x, y, z));
	    } else {
	      set_vector_elem(part, k, mk_vector_2d(sc, x, y));
	    }
	  }
	  set_vector_elem(coord, j, mk_pointer(part));
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
	    set_vector_elem(part, k, mk_vector_3d(sc, x, y, z));
	  } else {
	    set_vector_elem(part, k, mk_vector_2d(sc, x, y));
	  }
	}
	set_vector_elem(coord, j, mk_pointer(part));
      }
      break;
    }

    if (is_nil(type_id)) continue;

    nanoclj_cell_t * geom = mk_arraymap(sc, 2);
    fill_vector(geom, mk_nil());
    retain(sc, geom);
    
    set_vector_elem(geom, 0, mk_mapentry(sc, type_key, type_id));
    set_vector_elem(geom, 1, mk_mapentry(sc, coord_key, mk_pointer(coord)));    

    nanoclj_cell_t * prop = mk_arraymap(sc, 0);
    fill_vector(prop, mk_nil());
    retain(sc, prop);

    for (size_t j = 0; j < field_count; j++) {
      if (DBFIsAttributeNULL(dbf, i, j)) continue;

      char name[255];
      int type = DBFGetFieldInfo(dbf, j, name, 0, 0);
      nanoclj_val_t name_v = mk_string(sc, name);

      switch (type) {
      case FTString:
	prop = conjoin(sc, prop, mk_mapentry(sc, name_v, mk_string(sc, DBFReadStringAttribute(dbf, i, j))));
	if (strcmp(name, "NAME_EN") == 0) {
	  fprintf(stderr, "id = %d: %s\n", o->nShapeId, DBFReadStringAttribute(dbf, i, j));
	}
	break;
      case FTInteger:
	prop = conjoin(sc, prop, mk_mapentry(sc, name_v, mk_int(DBFReadIntegerAttribute(dbf, i, j))));
	break;
      case FTDouble:
	prop = conjoin(sc, prop, mk_mapentry(sc, name_v, mk_real(DBFReadDoubleAttribute(dbf, i, j))));
	break;
      case FTLogical:
	prop = conjoin(sc, prop, mk_mapentry(sc, name_v, mk_boolean(DBFReadLogicalAttribute(dbf, i, j))));
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
    
    nanoclj_cell_t * feat = mk_arraymap(sc, 5);
    fill_vector(feat, mk_nil());
    retain(sc, feat);
    
    set_vector_elem(feat, 0, mk_mapentry(sc, type_key, mk_string(sc, "Feature")));
    set_vector_elem(feat, 1, mk_mapentry(sc, geom_key, mk_pointer(geom)));
    set_vector_elem(feat, 2, mk_mapentry(sc, prop_key, mk_pointer(prop)));
    set_vector_elem(feat, 3, mk_mapentry(sc, bbox_key, bbox));
    set_vector_elem(feat, 4, mk_mapentry(sc, id_key, mk_integer(sc, o->nShapeId)));

    r = cons(sc, mk_pointer(feat), r);

    SHPDestroyObject(o);
  }
  
  SHPClose(shp);
  DBFClose(dbf);

  return mk_pointer(r);
}

static inline nanoclj_val_t Graph_load(nanoclj_t * sc, nanoclj_val_t args0) {
  nanoclj_cell_t * args = decode_pointer(args0);
  nanoclj_val_t src = first(sc, args);
  strview_t sv = to_strview(slurp(sc, T_READER, args));

  xmlDoc * doc = xmlReadMemory(sv.ptr, sv.size, "noname.xml", NULL, 0);
  if (doc == NULL) {
    return nanoclj_throw(sc, mk_runtime_exception(sc, mk_string(sc, "Could not parse file")));    
  }
  xmlNode * root = xmlDocGetRootElement(doc);
  
  
  xmlFreeDoc(doc);
  return mk_nil();
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
      nanoclj_val_t e = mk_mapentry(sc, def_keyword(sc, (const char *)attribute->name), mk_string(sc, (const char *)value));
      if (!attrs) {
	attrs = mk_arraymap(sc, 1);
	set_vector_elem(attrs, 0, e);
      } else {
	attrs = conjoin(sc, attrs, e);
      }
      retain(sc, attrs);
      xmlFree(value); 
    }
    
    output = mk_arraymap(sc, 1 + (content ? 1 : 0) + (attrs ? 1 : 0));
    retain(sc, output);

    int ni = 0;
    set_vector_elem(output, ni++, mk_mapentry(sc, def_keyword(sc, "tag"), tag));
    if (content) {
      set_vector_elem(output, ni++, mk_mapentry(sc, def_keyword(sc, "content"), mk_pointer(content)));
    }
    if (attrs) {
      set_vector_elem(output, ni++, mk_mapentry(sc, def_keyword(sc, "attrs"), mk_pointer(attrs)));
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

static inline nanoclj_val_t clojure_xml_parse(nanoclj_t * sc, nanoclj_val_t args0) {
  nanoclj_cell_t * args = decode_pointer(args0);
  nanoclj_val_t src = first(sc, args);
  strview_t sv = to_strview(slurp(sc, T_READER, args));

  xmlDoc * doc = xmlReadMemory(sv.ptr, sv.size, "noname.xml", NULL, 0);
  if (doc == NULL) {
    return nanoclj_throw(sc, mk_runtime_exception(sc, mk_string(sc, "Could not parse file")));
  }
  nanoclj_val_t xml = create_xml_node(sc, xmlDocGetRootElement(doc));
  xmlFreeDoc(doc);
  return xml;
}

static inline nanoclj_val_t clojure_data_csv_read_csv(nanoclj_t * sc, nanoclj_val_t args) {
  nanoclj_val_t f = first(sc, decode_pointer(args));
  if (is_nil(f)) {
    return nanoclj_throw(sc, sc->NullPointerException);
  }
  nanoclj_cell_t * rdr = decode_pointer(f);
  if (_type(rdr) != T_READER) {
    return nanoclj_throw(sc, mk_runtime_exception(sc, mk_string(sc, "Argument is not a reader")));
  }

  bool is_quoted = false;
  nanoclj_valarray_t * vec = NULL;
  nanoclj_bytearray_t * value = NULL;
  int32_t delimiter = ',';
  
  while ( 1 ) {
    int32_t c = inchar(rdr);
    if (c == -1) break;
    if (c == '\r') continue;
    if (!is_quoted) {
      if (!vec) vec = mk_valarray(sc, 0, 0);
      if (c == '\n') {
	break;
      } else {
	if (!value) value = mk_bytearray(sc, 0, 32);
	if (c == '"') {
	  is_quoted = true;
	} else if (c == delimiter) {
	  valarray_push(sc, vec, mk_string_with_bytearray(sc, value));
	  value = mk_bytearray(sc, 0, 32);
	} else {
	  append_codepoint(sc, value, c);
	}
      }
    } else if (c == '"') {
      is_quoted = false;
    } else {
      append_codepoint(sc, value, c);
    }
  }

  if (vec) {
    if (value) valarray_push(sc, vec, mk_string_with_bytearray(sc, value));
    nanoclj_val_t next_row = mk_foreign_func(sc, clojure_data_csv_read_csv, 1, 1);
    nanoclj_cell_t * code = cons(sc, mk_pointer(cons(sc, next_row, cons(sc, mk_pointer(rdr), NULL))), NULL);
    nanoclj_cell_t * lazy_seq = get_cell(sc, T_LAZYSEQ, 0, mk_pointer(cons(sc, sc->EMPTY, code)), sc->envir, NULL);
    return mk_pointer(cons(sc, mk_vector_with_valarray(sc, vec), lazy_seq));
  } else {
    return sc->EMPTY;
  }
}

#if NANOCLJ_USE_LINENOISE
#define MAX_COMPLETION_SYMBOLS 65535

static nanoclj_t * linenoise_sc = NULL;
static int num_completion_symbols = 0;
static const char * completion_symbols[MAX_COMPLETION_SYMBOLS];
 
static inline void completion(const char *input, linenoiseCompletions *lc) {
  int i, n = strlen(input);
  bool is_in_string = false;
  for (i = 0; i < n; i++) {
    if (input[i] == '"') {
      is_in_string = !is_in_string;
    } else if (input[i] == '\\' && i + 1 < n && input[i + 1] == '"') {
      i++;
    }
  }
  
  if (is_in_string) {
    // TODO: auto-complete urls and paths
  } else {
    if (!num_completion_symbols) {
      const char * expr = "(ns-interns *ns*)";
      nanoclj_val_t sym = nanoclj_eval_string(linenoise_sc, expr, strlen(expr));
      for ( ; sym.as_long != linenoise_sc->EMPTY.as_long && num_completion_symbols < MAX_COMPLETION_SYMBOLS; sym = cdr(sym)) {
	nanoclj_val_t v = car(sym);
	if (is_symbol(v)) {
	  completion_symbols[num_completion_symbols++] = decode_symbol(v)->name.ptr;
	} else if (is_mapentry(v)) {
	  completion_symbols[num_completion_symbols++] = decode_symbol(car(v))->name.ptr;
	}
      }
    }
    
    for (i = n; i > 0 && !(isspace(input[i - 1]) ||
			   input[i - 1] == '(' ||
			   input[i - 1] == '[' ||
			   input[i - 1] == '{'); i--) { }
    char buffer[1024];
    int prefix_len = 0;
    if (i > 0) {
      prefix_len = i;
      strncpy(buffer, input, i);
      buffer[prefix_len] = 0;
      input += i;
      n -= i;
    }
    for (i = 0; i < num_completion_symbols; i++) {
      const char * s = completion_symbols[i];
      if (strncmp(input, s, n) == 0) {
	strncpy(buffer + prefix_len, s, 1024 - prefix_len);
	buffer[1023] = 0;
	linenoiseAddCompletion(lc, buffer);
      }
    }
  }
}

static inline char *complete_parens(const char * input) {
  char nest[1024];
  int si = 0;
  for (int i = 0, n = strlen(input); i < n; i++) {
    if (input[i] == '(') {
      nest[si++] = ')';
    } else if (input[i] == '[') {
      nest[si++] = ']';
    } else if (input[i] == '{') {
      nest[si++] = '}';
    } else if (input[i] == '"' && (si == 0 || nest[si - 1] != '"')) {
      nest[si++] = '"';
    } else if (input[i] == ')' || input[i] == ']' || input[i] == '}' || input[i] == '"') {
      if (si > 0 && nest[si - 1] == input[i]) {
	si--;
      } else {
	return NULL;
      }
    } else if (input[i] == '\\' && i + 1 < n && input[i + 1] == '"') {
      i++;
    }
  }
  if (si == 0) {
    return NULL;
  }
  char * h = (char *)linenoise_sc->malloc(si + 1);
  int hi = 0;
  for (int i = si - 1; i >= 0; i--) {
    h[hi++] = nest[i];
  }
  h[hi] = 0;

  return h;
}
 
static inline char *hints(const char *input, int *color, int *bold) {
  char * h = complete_parens(input);
  if (!h) return NULL;
  
  *color = 35;
  *bold = 0;
  return h;  
}

static inline void free_hints(void * ptr) {
  linenoise_sc->free(ptr);
}

static inline void on_mouse_motion(int x, int y) {
  double f = linenoise_sc->window_scale_factor;
  nanoclj_val_t p = mk_vector_2d(linenoise_sc, x / f, y / f);
  intern(linenoise_sc, linenoise_sc->global_env, linenoise_sc->MOUSE_POS, p);
}

static inline void on_window_size() {
  update_window_info(linenoise_sc, decode_pointer(get_out_port(linenoise_sc)));
}

static inline void init_linenoise(nanoclj_t * sc) {
  linenoise_sc = sc;
    
  linenoiseSetMultiLine(0);
  linenoiseSetupSigWinchHandler();
  linenoiseSetCompletionCallback(completion);
  linenoiseSetHintsCallback(hints);
  linenoiseSetFreeHintsCallback(free_hints);
#if 0
  linenoiseSetMouseMotionCallback(on_mouse_motion);
#endif
  linenoiseSetWindowSizeCallback(on_window_size);
  linenoiseHistorySetMaxLen(10000);
}

static inline nanoclj_val_t linenoise_readline(nanoclj_t * sc, nanoclj_val_t args) {
  nanoclj_val_t out = get_out_port(sc);
  if (is_cell(out)) {
    nanoclj_cell_t * o = decode_pointer(out);
    if (is_writable(o)) _line_unchecked(o)++;
  }
  
  strview_t prompt = to_strview(first(sc, decode_pointer(args)));
  char * line = linenoise(prompt.ptr, prompt.size);
  if (line == NULL) return mk_nil();
  
  nanoclj_val_t r;
  char * missing_parens = complete_parens(line);
  if (!missing_parens) {
    r = mk_string(sc, line);
  } else {
    int l1 = strlen(line), l2 = strlen(missing_parens);
    char * tmp = (char *)sc->malloc(l1 + l2 + 1);
    strcpy(tmp, line);
    strcpy(tmp + l1, missing_parens);
    sc->free(missing_parens);
    r = mk_string(sc, tmp);
    sc->free(tmp);
  }
  sc->free(line);

  return r;
}

static inline nanoclj_val_t linenoise_history_load(nanoclj_t * sc, nanoclj_val_t args0) {
  nanoclj_cell_t * args = decode_pointer(args0);
  char * fn = alloc_c_str(sc, to_strview(first(sc, args)));
  linenoiseHistoryLoad(fn);
  sc->free(fn);
  return mk_nil();
}

static inline nanoclj_val_t linenoise_history_append(nanoclj_t * sc, nanoclj_val_t args0) {
  nanoclj_cell_t * args = decode_pointer(args0);
  char * line = alloc_c_str(sc, to_strview(first(sc, args)));
  linenoiseHistoryAdd(line);
  args = next(sc, args);
  if (args) {
    char * fn = alloc_c_str(sc, to_strview(first(sc, args)));
    linenoiseHistorySave(fn, line);
    sc->free(fn);
  }
  sc->free(line);
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
  intern_foreign_func(sc, System, "setProperty", System_setProperty, 0, 1);
  intern_foreign_func(sc, System, "glob", System_glob, 1, 1);
  intern_foreign_func(sc, System, "getSystemTimes", System_getSystemTimes, 0, 0);
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

  intern(sc, Math, def_symbol(sc, "E"), mk_real(M_E));
  intern(sc, Math, def_symbol(sc, "PI"), mk_real(M_PI));

  intern_foreign_func(sc, numeric_tower, "expt", numeric_tower_expt, 2, 2);

  intern_foreign_func(sc, browse, "browse-url", browse_url, 1, 1);

  intern_foreign_func(sc, sc->Image, "load", Image_load, 1, 1);
  intern_foreign_func(sc, sc->Image, "resize", Image_resize, 3, 3);
  intern_foreign_func(sc, sc->Image, "transpose", Image_transpose, 1, 1);
  intern_foreign_func(sc, sc->Image, "save", Image_save, 2, 2);
  intern_foreign_func(sc, sc->Image, "horizontalGaussianBlur", Image_horizontalGaussianBlur, 2, 2);

  intern_foreign_func(sc, sc->Audio, "load", Audio_load, 1, 1);
  intern_foreign_func(sc, sc->Audio, "lowpass", Audio_lowpass, 1, 1);

  intern_foreign_func(sc, Geo, "load", Geo_load, 1, 1);
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
