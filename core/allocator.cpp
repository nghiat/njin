//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#include "core/allocator.h"

#include <string.h>

void* nj_allocator_t::alloc(njsp size) {
  return aligned_alloc(size, 16);
}

void* nj_allocator_t::alloc_zero(njsp size) {
  void* p = aligned_alloc(size, 16);
  memset(p, 0, size);
  return p;
}
