//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#ifndef NJ_CORE_LOADER_PNG_H
#define NJ_CORE_LOADER_PNG_H

#include "core/os_string.h"

struct nj_allocator_t;

struct nj_png_t {
  nj_allocator_t* allocator;
  nju8* data = NULL;
  nju32 width = 0;
  nju32 height = 0;
  nju32 bit_depth = 0;
  nju32 bit_per_pixel = 0;
};

bool nj_png_init(nj_png_t* png, const nj_os_char* path, nj_allocator_t* allocator);
void nj_png_destroy(nj_png_t* png);

#endif // NJ_CORE_LOADER_PNG_H
