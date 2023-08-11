/* dynload.h */
/* Original Copyright (c) 1999 Alexander Shendi     */
/* Modifications for NT and dl_* interface: D. Souflis */

#ifndef DYNLOAD_H
#define DYNLOAD_H

#include "nanoclj_priv.h"

NANOCLJ_EXPORT pointer scm_load_ext(nanoclj * sc, pointer arglist);

void scm_unload_ext(NANOCLJ_EXPORT pointer ptr);

#endif
