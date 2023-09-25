#ifndef _NANOCLJ_FUNCTIONS_H_
#define _NANOCLJ_FUNCTIONS_H_

#include <stdint.h>
#include <unistd.h>
#include <glob.h>

#include "linenoise.h"
#include "nanoclj_utils.h"

#ifdef WIN32
#include <direct.h>
#define getcwd _getcwd
#define environ _environ
#define STBIW_WINDOWS_UTF8
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
    ms_sleep(ms);
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
    for (int i = 0; i < l; i++) {
      char * p = environ[i];
      int j = 0;
      for (; p[j] != '=' && p[j] != 0; j++) {
	
      }
      nanoclj_val_t key = mk_counted_string(sc, p, j);
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
    if (get_elem(sc, sc->properties, first(sc, args), &v)) {
      return v;
    } else {
      return mk_nil();
    }
  } else {
    return mk_pointer(sc->properties);
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
  
  if (r != 0 && r != GLOB_NOMATCH) {
    return mk_nil();
  } else {
    return mk_pointer(x);
  }
}

/* Math */

static nanoclj_val_t Math_sin(nanoclj_t * sc, nanoclj_val_t args) {
  return mk_real(sin(to_double(car(args))));
}

static nanoclj_val_t Math_cos(nanoclj_t * sc, nanoclj_val_t args) {
  return mk_real(cos(to_double(car(args))));
}

static nanoclj_val_t Math_exp(nanoclj_t * sc, nanoclj_val_t args) {
  return mk_real(exp(to_double(car(args))));
}

static nanoclj_val_t Math_log(nanoclj_t * sc, nanoclj_val_t args) {
  return mk_real(log(to_double(car(args))));
}

static nanoclj_val_t Math_log10(nanoclj_t * sc, nanoclj_val_t args) {
  return mk_real(log10(to_double(car(args))));
}

static nanoclj_val_t Math_tan(nanoclj_t * sc, nanoclj_val_t args) {
  return mk_real(tan(to_double(car(args))));
}

static nanoclj_val_t Math_asin(nanoclj_t * sc, nanoclj_val_t args) {
  return mk_real(asin(to_double(car(args))));
}

static nanoclj_val_t Math_acos(nanoclj_t * sc, nanoclj_val_t args) {
  return mk_real(acos(to_double(car(args))));
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
  return mk_real(sqrt(to_double(car(args))));
}

static nanoclj_val_t Math_cbrt(nanoclj_t * sc, nanoclj_val_t args) {
  return mk_real(cbrt(to_double(car(args))));
}

static nanoclj_val_t Math_pow(nanoclj_t * sc, nanoclj_val_t args) {
  return mk_real(pow(to_double(car(args)), to_double(cadr(args))));
}

static nanoclj_val_t Math_floor(nanoclj_t * sc, nanoclj_val_t args) {
  return mk_real(floor(to_double(car(args))));
}

static nanoclj_val_t Math_ceil(nanoclj_t * sc, nanoclj_val_t args) {
  return mk_real(ceil(to_double(car(args))));
}

static nanoclj_val_t Math_round(nanoclj_t * sc, nanoclj_val_t args) {
  return mk_real(round(to_double(car(args))));
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

static nanoclj_val_t numeric_tower_expt(nanoclj_t * sc, nanoclj_val_t args) {
  nanoclj_val_t x = car(args);
  nanoclj_val_t y = cadr(args);

  int tx = prim_type(x), ty = prim_type(y);

  if (tx == T_INTEGER && ty == T_INTEGER) {
    int base = decode_integer(x), exp = decode_integer(y);
    long long res;
    if (base == 0 || base == 1) {
      return mk_int(base);
    } else if (exp >= 0 && exp < 256 && !ipow_overflow(base, exp, &res)) {
      return mk_integer(sc, res);
    } else if (exp > -256 && exp < 0 && !ipow_overflow(base, -exp, &res)) {
      return mk_ratio_long(sc, 1, res);
    }
  } else if (tx != T_REAL && ty != T_REAL) {
    tx = expand_type(x, tx);
    ty = expand_type(y, ty);

    if (tx == T_RATIO && (ty == T_INTEGER || ty == T_LONG)) {
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
    } else if (tx != T_RATIO && ty != T_RATIO) {
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
  }

  return mk_real(pow(to_double(x), to_double(y)));
}

static inline nanoclj_val_t browse_url(nanoclj_t * sc, nanoclj_val_t args) {  
#ifdef WIN32
  char * url = alloc_c_str(sc, to_strview(car(args)));
  ShellExecute(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
  sc->free(url);
  return (nanoclj_val_t)kTRUE;
#else
  pid_t r = fork();
  if (r == 0) {
    char * url = alloc_c_str(sc, to_strview(car(args)));
    const char * cmd = "xdg-open";
    execlp(cmd, cmd, url, NULL);
    exit(1);
  } else {
    return r < 0 ? (nanoclj_val_t)kFALSE : (nanoclj_val_t)kTRUE;
  }
#endif
}

static inline nanoclj_val_t shell_sh(nanoclj_t * sc, nanoclj_val_t args0) {
  nanoclj_cell_t * args = decode_pointer(args0);
  size_t n = seq_length(sc, args);
  if (n >= 1) {
#ifdef WIN32
    /* TODO */
    return mk_nil();
#else
    pid_t r = fork();
    if (r == 0) {
      char * cmd = (char *)alloc_c_str(sc, to_strview(first(sc, args)));
      args = rest(sc, args);
      n--;
      char ** output_args = (char **)sc->malloc(n * sizeof(char*));
      for (size_t i = 0; i < n; i++, args = rest(sc, args)) {
	output_args[i] = alloc_c_str(sc, to_strview(first(sc, args)));
      }
      execvp(cmd, output_args);
      exit(1);
    } else {
      return r < 0 ? (nanoclj_val_t)kFALSE : (nanoclj_val_t)kTRUE;
    }
  } else {
    return mk_nil();
  }
#endif
}

static inline nanoclj_val_t Image_load(nanoclj_t * sc, nanoclj_val_t args) {
  char * filename = alloc_c_str(sc, to_strview(car(args)));
	    
  int w, h, channels;
  unsigned char * data = stbi_load(filename, &w, &h, &channels, 0);
  sc->free(filename);
  if (!data) {
    return mk_exception(sc, "Failed to load Image");
  }
  
  nanoclj_val_t image = mk_image(sc, w, h, channels, data);
  
  stbi_image_free(data);

  return image;
}

static inline nanoclj_val_t Image_resize(nanoclj_t * sc, nanoclj_val_t args) {
  nanoclj_val_t image00 = car(args);
  nanoclj_val_t target_width0 = cadr(args), target_height0 = caddr(args);
  if (!is_image(image00) || !is_number(target_width0) || !is_number(target_height0)) {
    return mk_exception(sc, "Invalid type");
  }
  
  int target_width = to_int(target_width0), target_height = to_int(target_height0);  
  nanoclj_cell_t * image0 = decode_pointer(image00);
  nanoclj_image_t * image = _image_unchecked(image0);
  
  size_t target_size = target_width * target_height * image->channels;

  unsigned char * tmp = (unsigned char *)sc->malloc(target_size);
  
  stbir_resize_uint8(image->data, image->width, image->height, 0, tmp, target_width, target_height, 0, image->channels);

  nanoclj_val_t target_image = mk_image(sc, target_width, target_height, image->channels, tmp);

  sc->free(tmp);
  
  return target_image;
}

static inline nanoclj_val_t Image_transpose(nanoclj_t * sc, nanoclj_val_t args) {
  nanoclj_val_t image00 = car(args);
  if (!is_image(image00)) {
    return mk_exception(sc, "Not an Image");
  }
  nanoclj_cell_t * image0 = decode_pointer(image00);
  nanoclj_image_t * image = _image_unchecked(image0);
  int w = image->width, h = image->height, channels = image->channels;
  
  unsigned char * tmp = (unsigned char *)sc->malloc(w * h * channels);

  if (channels == 4) {
    for (int y = 0; y < h; y++) {
      for (int x = 0; x < w; x++) {
	tmp[4 * (x * h + y) + 0] = image->data[4 * (y * w + x) + 0];
	tmp[4 * (x * h + y) + 1] = image->data[4 * (y * w + x) + 1];
	tmp[4 * (x * h + y) + 2] = image->data[4 * (y * w + x) + 2];     
	tmp[4 * (x * h + y) + 3] = image->data[4 * (y * w + x) + 3];     
      }
    }
  } else {
    sc->free(tmp);
    return mk_exception(sc, "Unsupported number of channels");
  }
  
  nanoclj_val_t new_image = mk_image(sc, h, w, channels, tmp);

  sc->free(tmp);

  return new_image;
}

static inline nanoclj_val_t Image_save(nanoclj_t * sc, nanoclj_val_t args) {
  nanoclj_val_t image00 = car(args), filename0 = cadr(args);
  if (!is_image(image00)) {
    return mk_exception(sc, "Not an Image");
  }
  if (!is_string(filename0)) {
    return mk_exception(sc, "Not a string");
  }
  nanoclj_cell_t * image0 = decode_pointer(image00);
  nanoclj_image_t * image = _image_unchecked(image0);

  char * filename = alloc_c_str(sc, to_strview(filename0));

  int w = image->width, h = image->height, channels = image->channels;
  unsigned char * data = image->data;
  
  char * ext = strrchr(filename, '.');
  if (!ext) {
    sc->free(filename);
    return mk_exception(sc, "Could not determine file format");
  } else {
    int success = 0;
    if (strcmp(ext, ".png") == 0) {
      fprintf(stderr, "saving png: %d %d %d\n", w, h, channels);
      
      success = stbi_write_png(filename, w, h, channels, data, w);
    } else if (strcmp(ext, ".bmp") == 0) {
      success = stbi_write_bmp(filename, w, h, channels, data);
    } else if (strcmp(ext, ".tga") == 0) {
      success = stbi_write_tga(filename, w, h, channels, data);
    } else if (strcmp(ext, ".jpg") == 0) {
      success = stbi_write_jpg(filename, w, h, channels, data, 95);
    } else {
      return mk_exception(sc, "Unsupported file format");
    }

    sc->free(filename);

    if (!success) {
      return mk_exception(sc, "Error writing file");
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

nanoclj_val_t Image_gaussian_blur(nanoclj_t * sc, nanoclj_val_t args) {
  nanoclj_val_t image00 = car(args), h_radius = cadr(args), v_radius = caddr(args);
  if (!is_image(image00)) {
    return mk_exception(sc, "Not an Image");
  }
  if (!is_number(h_radius)) {
    return mk_exception(sc, "Not a number");
  }
  if (!is_nil(v_radius) && v_radius.as_long != sc->EMPTY.as_long && !is_number(v_radius)) {
    return mk_exception(sc, "Not a number or nil");
  }

  nanoclj_cell_t * image0 = decode_pointer(image00);
  nanoclj_image_t * image = _image_unchecked(image0);
  int w = image->width, h = image->height, channels = image->channels;
  
  int hsize, vsize;
  int * hkernel = mk_kernel(sc, to_double(h_radius), &hsize);

  int htotal = 0, vtotal = 0;
  for (int i = 0; i < hsize; i++) htotal += hkernel[i];

  int * vkernel;
  if (v_radius.as_long == sc->EMPTY.as_long || is_nil(v_radius)) {
    vsize = hsize;
    vkernel = hkernel;
    vtotal = htotal;
  } else {
    vkernel = mk_kernel(sc, to_double(v_radius), &vsize);
    for (int i = 0; i < vsize; i++) vtotal += vkernel[i];    
  }
  
  unsigned char * output_data = (unsigned char *)sc->malloc(w * h * channels);
  unsigned char * tmp = (unsigned char *)sc->malloc(w * h * channels);

  if (channels == 4) {
    if (hsize) {
      memset(tmp, 0, w * h * channels);
      for (int row = 0; row < h; row++) {
        for (int col = 0; col < w; col++) {
          int c0 = 0, c1 = 0, c2 = 0, c3 = 0;
          for (int i = 0; i < hsize; i++) {
	    const unsigned char * ptr = image->data + (row * w + clamp(col + i - vsize / 2, 0, w - 1)) * 4;
            c0 += *ptr++ * hkernel[i];
            c1 += *ptr++ * hkernel[i];
            c2 += *ptr++ * hkernel[i];
            c3 += *ptr++ * hkernel[i];
          }
	  unsigned char * ptr = tmp + (row * w + col) * 4;
          *ptr++ = (unsigned char)(c0 / htotal);
          *ptr++ = (unsigned char)(c1 / htotal);
          *ptr++ = (unsigned char)(c2 / htotal);
          *ptr++ = (unsigned char)(c3 / htotal);
        }
      }
    } else {
      memcpy(tmp, image->data, w * h * 4);
    }
    if (vsize) {      
      memset(output_data, 0, w * h * channels);
      for (int col = 0; col < w; col++) {
        for (int row = 0; row < h; row++) {
          int c0 = 0, c1 = 0, c2 = 0, c3 = 0;
          for (int i = 0; i < vsize; i++) {
            const unsigned char * ptr = tmp + (clamp(row + i - vsize / 2, 0, h - 1) * w + col) * 4;
            c0 += *ptr++ * vkernel[i];
            c1 += *ptr++ * vkernel[i];
            c2 += *ptr++ * vkernel[i];
            c3 += *ptr++ * vkernel[i];
          }
	  unsigned char * ptr = output_data + (row * w + col) * 4;
          *ptr++ = (unsigned char)(c0 / vtotal);
          *ptr++ = (unsigned char)(c1 / vtotal);
          *ptr++ = (unsigned char)(c2 / vtotal);
          *ptr++ = (unsigned char)(c3 / vtotal);
        }
      }
    } else {
      memcpy(output_data, tmp, w * h * 4);
    }
  } else if (channels == 1) {
    if (hsize) {
      memset(tmp, 0, w * h * channels);
      for (int row = 0; row < h; row++) {
        for (int col = 0; col < w; col++) {
          int c0 = 0;
          for (int i = 0; i < hsize; i++) {
            const unsigned char * ptr = image->data + (row * w + clamp(col + i - vsize / 2, 0, w - 1));
            c0 += *ptr * hkernel[i];
          }
          unsigned char * ptr = tmp + row * w + col;
          *ptr = (unsigned char)(c0 / htotal);
        }
      }
    } else {
      memcpy(tmp, image->data, w * h);
    }
    if (vsize) {
      memset(output_data, 0, w * h * channels);
      for (int col = 0; col < w; col++) {
        for (int row = 0; row < h; row++) {
          int c0 = 0;
          for (int i = 0; i < vsize; i++) {
            const unsigned char * ptr = tmp + (clamp(row + i - vsize / 2, 0, h - 1) * w + col);
            c0 += *ptr * vkernel[i];
          }
          unsigned char * ptr = output_data + row * w + col;
          *ptr = (unsigned char)(c0 / vtotal);
        }
      }
    } else {
      memcpy(output_data, tmp, w * h);
    }
  }

  if (vkernel != hkernel) sc->free(vkernel);
  sc->free(hkernel);
  
  sc->free(tmp);
  nanoclj_val_t r = mk_image(sc, w, h, channels, output_data);
  sc->free(output_data);
  return r;
}

static inline nanoclj_val_t Audio_load(nanoclj_t * sc, nanoclj_val_t args) {
  char * filename = alloc_c_str(sc, to_strview(car(args)));
  drwav wav;
  if (!drwav_init_file(&wav, filename, NULL)) {
    return mk_exception(sc, "Failed to load Audio");
  }
  sc->free(filename);

  /* Read interleaved frames */
  float * data = (float *)sc->malloc(wav.totalPCMFrameCount * wav.channels * sizeof(float));
  size_t samples_decoded = drwav_read_pcm_frames_f32(&wav, wav.totalPCMFrameCount, data);

  nanoclj_val_t audio = mk_audio(sc, samples_decoded, wav.channels, wav.sampleRate, data);
  sc->free(data);
  
  drwav_uninit(&wav);
  
  return audio;
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
	  completion_symbols[num_completion_symbols++] = decode_symbol(v)->name;
	} else if (is_mapentry(v)) {
	  completion_symbols[num_completion_symbols++] = decode_symbol(car(v))->name;
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
  update_window_info(linenoise_sc, get_out_port(linenoise_sc));
}

static inline nanoclj_val_t linenoise_readline(nanoclj_t * sc, nanoclj_val_t args) {
  if (!linenoise_sc) {
    linenoise_sc = sc;

    linenoiseSetMultiLine(0);
    linenoiseSetupSigWinchHandler();
    linenoiseSetCompletionCallback(completion);
    linenoiseSetHintsCallback(hints);
    linenoiseSetFreeHintsCallback(free_hints);
    linenoiseSetMouseMotionCallback(on_mouse_motion);
    linenoiseSetWindowSizeCallback(on_window_size);
    linenoiseHistorySetMaxLen(10000);
    linenoiseHistoryLoad(".nanoclj_history");
  }
  strview_t prompt = to_strview(car(args));
  char * line = linenoise(prompt.ptr, prompt.size);
  if (line == NULL) return mk_nil();
  
  linenoiseHistoryAdd(line);
  linenoiseHistorySave(".nanoclj_history");

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
#endif

static inline void register_functions(nanoclj_t * sc) {
  nanoclj_cell_t * Thread = def_namespace(sc, "Thread");
  nanoclj_cell_t * System = def_namespace(sc, "System");
  nanoclj_cell_t * Math = def_namespace(sc, "Math");
  nanoclj_cell_t * numeric_tower = def_namespace(sc, "clojure.math.numeric-tower");
  nanoclj_cell_t * clojure_java_browse = def_namespace(sc, "clojure.java.browse");
  nanoclj_cell_t * clojure_java_shell = def_namespace(sc, "clojure.java.shell");
  nanoclj_cell_t * Image = def_namespace(sc, "Image");
  nanoclj_cell_t * Audio = def_namespace(sc, "Audio");
  
  intern(sc, Thread, def_symbol(sc, "sleep"), mk_foreign_func_with_arity(sc, Thread_sleep, 1, 1));
  
  intern(sc, System, def_symbol(sc, "exit"), mk_foreign_func_with_arity(sc, System_exit, 1, 1));
  intern(sc, System, def_symbol(sc, "currentTimeMillis"), mk_foreign_func_with_arity(sc, System_currentTimeMillis, 0, 0));
  intern(sc, System, def_symbol(sc, "nanoTime"), mk_foreign_func_with_arity(sc, System_nanoTime, 0, 0));
  intern(sc, System, def_symbol(sc, "gc"), mk_foreign_func_with_arity(sc, System_gc, 0, 0));
  intern(sc, System, def_symbol(sc, "getenv"), mk_foreign_func_with_arity(sc, System_getenv, 0, 1));
  intern(sc, System, def_symbol(sc, "getProperty"), mk_foreign_func_with_arity(sc, System_getProperty, 0, 1));
  intern(sc, System, def_symbol(sc, "setProperty"), mk_foreign_func_with_arity(sc, System_setProperty, 0, 1));
  intern(sc, System, def_symbol(sc, "glob"), mk_foreign_func_with_arity(sc, System_glob, 1, 1));
  
  intern(sc, Math, def_symbol(sc, "sin"), mk_foreign_func_with_arity(sc, Math_sin, 1, 1));
  intern(sc, Math, def_symbol(sc, "cos"), mk_foreign_func_with_arity(sc, Math_cos, 1, 1));
  intern(sc, Math, def_symbol(sc, "exp"), mk_foreign_func_with_arity(sc, Math_exp, 1, 1));
  intern(sc, Math, def_symbol(sc, "log"), mk_foreign_func_with_arity(sc, Math_log, 1, 1));
  intern(sc, Math, def_symbol(sc, "log10"), mk_foreign_func_with_arity(sc, Math_log10, 1, 1));
  intern(sc, Math, def_symbol(sc, "tan"), mk_foreign_func_with_arity(sc, Math_tan, 1, 1));
  intern(sc, Math, def_symbol(sc, "asin"), mk_foreign_func_with_arity(sc, Math_asin, 1, 1));
  intern(sc, Math, def_symbol(sc, "acos"), mk_foreign_func_with_arity(sc, Math_acos, 1, 1));
  intern(sc, Math, def_symbol(sc, "atan"), mk_foreign_func_with_arity(sc, Math_atan, 1, 2));
  intern(sc, Math, def_symbol(sc, "sqrt"), mk_foreign_func_with_arity(sc, Math_sqrt, 1, 1));
  intern(sc, Math, def_symbol(sc, "cbrt"), mk_foreign_func_with_arity(sc, Math_cbrt, 1, 1));
  intern(sc, Math, def_symbol(sc, "pow"), mk_foreign_func_with_arity(sc, Math_pow, 2, 2));
  intern(sc, Math, def_symbol(sc, "floor"), mk_foreign_func_with_arity(sc, Math_floor, 1, 1));
  intern(sc, Math, def_symbol(sc, "ceil"), mk_foreign_func_with_arity(sc, Math_ceil, 1, 1));
  intern(sc, Math, def_symbol(sc, "round"), mk_foreign_func_with_arity(sc, Math_round, 1, 1));

  intern(sc, Math, def_symbol(sc, "E"), mk_real(M_E));
  intern(sc, Math, def_symbol(sc, "PI"), mk_real(M_PI));
  intern(sc, Math, def_symbol(sc, "SQRT2"), mk_real(M_SQRT2));

  intern(sc, numeric_tower, def_symbol(sc, "expt"), mk_foreign_func_with_arity(sc, numeric_tower_expt, 2, 2));

  intern(sc, clojure_java_browse, def_symbol(sc, "browse-url"), mk_foreign_func_with_arity(sc, browse_url, 1, 1));

  intern(sc, clojure_java_shell, def_symbol(sc, "sh"), mk_foreign_func(sc, shell_sh));

  intern(sc, Image, def_symbol(sc, "load"), mk_foreign_func_with_arity(sc, Image_load, 1, 1));
  intern(sc, Image, def_symbol(sc, "resize"), mk_foreign_func_with_arity(sc, Image_resize, 3, 3));
  intern(sc, Image, def_symbol(sc, "transpose"), mk_foreign_func_with_arity(sc, Image_transpose, 1, 1));
  intern(sc, Image, def_symbol(sc, "save"), mk_foreign_func_with_arity(sc, Image_save, 2, 2));
  intern(sc, Image, def_symbol(sc, "blur"), mk_foreign_func_with_arity(sc, Image_gaussian_blur, 2, 2));
  intern(sc, Image, def_symbol(sc, "gaussian-blur"), mk_foreign_func_with_arity(sc, Image_gaussian_blur, 2, 2));

  intern(sc, Audio, def_symbol(sc, "load"), mk_foreign_func_with_arity(sc, Audio_load, 1, 1));
  
#if NANOCLJ_USE_LINENOISE
  nanoclj_cell_t * linenoise = def_namespace(sc, "linenoise");
  intern(sc, linenoise, def_symbol(sc, "read-line"), mk_foreign_func_with_arity(sc, linenoise_readline, 1, 1));
#endif  
}

#endif
