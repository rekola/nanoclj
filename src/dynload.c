/* dynload.c Dynamic Loader for TinyScheme */
/* Original Copyright (c) 1999 Alexander Shendi     */
/* Modifications for NT and dl_* interface, scm_load_ext: D. Souflis */
/* Refurbished by Stephen Gildea */

#define _NANOCLJ_SOURCE
#include "dynload.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif

static void make_filename(const char *name, char *filename);
static void make_init_fn(const char *name, char *init_fn);

#ifdef _WIN32
#include <windows.h>
#else
typedef void *HMODULE;
typedef void (*FARPROC) ();
#ifndef SUN_DL
#define SUN_DL
#endif
#include <dlfcn.h>
#endif

#ifdef _WIN32

#define PREFIX ""
#define SUFFIX ".dll"

static void display_w32_error_msg(const char *additional_message) {
  LPVOID msg_buf;
  
  FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
		NULL, GetLastError(), 0, (LPTSTR) & msg_buf, 0, NULL);
  fprintf(stderr, "nanoclj load-extension: %s: %s", additional_message,
	  msg_buf);
  LocalFree(msg_buf);
}

static HMODULE dl_attach(const char *module) {
  HMODULE dll = LoadLibrary(module);
  if (!dll)
    display_w32_error_msg(module);
  return dll;
}

static FARPROC dl_proc(HMODULE mo, const char *proc) {
  FARPROC procedure = GetProcAddress(mo, proc);
  if (!procedure)
    display_w32_error_msg(proc);
  return procedure;
}

static void dl_detach(HMODULE mo) {
  (void) FreeLibrary(mo);
}

#elif defined(SUN_DL)

#include <dlfcn.h>

#define PREFIX "lib"
#define SUFFIX ".so"

static HMODULE dl_attach(const char *module) {
  HMODULE so = dlopen(module, RTLD_LAZY);
  if (!so) {
    fprintf(stderr, "Error loading nanoclj extension \"%s\": %s\n", module,
	    dlerror());
  }
  return so;
}

static FARPROC dl_proc(HMODULE mo, const char *proc) {
  const char *errmsg;
  FARPROC fp;
  
  *(void **) (&fp) = dlsym(mo, proc);
  if ((errmsg = dlerror()) == 0) {
    return fp;
  }
  fprintf(stderr, "Error initializing nanoclj module \"%s\": %s\n", proc,
	  errmsg);
  return 0;
}

static void dl_detach(HMODULE mo) {
  (void) dlclose(mo);
}
#endif

static HMODULE dll_handle;

nanoclj_val_t scm_load_ext(nanoclj * sc, nanoclj_val_t args) {
  nanoclj_val_t first_arg;
  nanoclj_val_t retval;
  char filename[MAXPATHLEN], init_fn[MAXPATHLEN + 6];
  char *name;
  void (*module_init) (nanoclj * sc);
  
  if ((args != sc->NIL) && is_string((first_arg = pair_car(args)))) {
    name = string_value(first_arg);
    make_filename(name, filename);
    make_init_fn(name, init_fn);
    dll_handle = dl_attach(filename);
    if (dll_handle == 0) {
      retval = sc->F;
    } else {
      module_init = (void (*)(nanoclj *)) dl_proc(dll_handle, init_fn);
      if (module_init != 0) {
        (*module_init) (sc);
        retval = sc->T;
      } else {
        retval = sc->F;
      }
    }
  } else {
    retval = sc->F;
  }

  return (retval);
}

void scm_unload_ext(nanoclj_val_t ptr) {
  // todo: this is not supposed to work yet, but probably not greatly needed anyway
  dl_detach(dll_handle);
}

static void make_filename(const char *name, char *filename) {
  strcpy(filename, name);
  strcat(filename, SUFFIX);
}

static void make_init_fn(const char *name, char *init_fn) {
  const char *p = strrchr(name, '/');
  if (p == 0) {
    p = name;
  } else {
    p++;
  }
  strcpy(init_fn, "init_");
  strcat(init_fn, p);
}
