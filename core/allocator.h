//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#ifndef CORE_ALLOCATOR_H
#define CORE_ALLOCATOR_H

#include "core/njtype.h"

struct nj_allocator_t {
  virtual void destroy() = 0;
  virtual void* aligned_alloc(njsp size, njsp alignment) = 0;
  virtual void* realloc(void* p, njsp size) = 0;
  virtual void free(void* p) = 0;

  void* alloc(njsp size);

  const char* name = nullptr;
  /// Total size of the allocator in bytes.
  njsp total_size = 0;
  /// Allocations' sizes and supporting data of the allocator in bytes.
  njsp used_size = 0;
};

inline
void* nj_allocator_t::alloc(njsp size) {
  return aligned_alloc(size, 16);
}

#endif // CORE_ALLOCATOR_H
