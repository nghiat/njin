//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#include "core/dynamic_lib.h"

#include <Windows.h>

bool nj_dl_open(nj_dynamic_lib_t* dl, const char* name) {
  *dl = (void*)LoadLibraryA(name);
  return *dl;
}

void nj_dl_close(nj_dynamic_lib_t* dl) { FreeLibrary((HMODULE)(*dl)); }

void* nj_dl_get_proc(nj_dynamic_lib_t* dl, const char* name) {
  return (void*)GetProcAddress((HMODULE)(*dl), name);
}
