/* dynload.h */
/* Original Copyright (c) 1999 Alexander Shendi     */
/* Modifications for NT and dl_* interface: D. Souflis */

#ifndef NANOCLJ_DYNLOAD_H
#define NANOCLJ_DYNLOAD_H

#include "nanoclj-private.h"

SCHEME_EXPORT pointer scm_load_ext(nanoclj_t *sc, nanoclj_val_t arglist);

#endif
