//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2018             //
//----------------------------------------------------------------------------//

#include "core/dynamic_lib.h"

#include <dlfcn.h>

bool nj_dl_open(nj_dynamic_lib_t* dl, const char* name) {
  *dl = dlopen(name, RTLD_LAZY | RTLD_LOCAL);
  return *dl;
}

void nj_dl_close(nj_dynamic_lib_t* dl) {
  dlclose(*dl);
}

void* nj_dl_get_proc(nj_dynamic_lib_t* dl, const char* name) {
  return dlsym(*dl, name);
}
