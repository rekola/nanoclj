/* dynload.h */
/* Original Copyright (c) 1999 Alexander Shendi     */
/* Modifications for NT and dl_* interface: D. Souflis */

#ifndef DYNLOAD_H
#define DYNLOAD_H

#include "scm_priv.h"

SCHEME_EXPORT pointer scm_load_ext(scheme * sc, pointer arglist);

void scm_unload_ext(SCHEME_EXPORT pointer ptr);

#endif
