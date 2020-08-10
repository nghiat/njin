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
  virtual void* aligned_alloc(nju32 size, nju32 alignment) = 0;
  virtual void* realloc(void* p, nju32 size) = 0;
  virtual void free(void* p) = 0;

  void* alloc(nju32 size);

  const char* name = nullptr;
  /// Total size of the allocator in bytes.
  njsz total_size = 0;
  /// Allocations' sizes and supporting data of the allocator in bytes.
  njsz used_size = 0;
};

inline
void* nj_allocator_t::alloc(nju32 size) {
  return aligned_alloc(size, 16);
}

#endif // CORE_ALLOCATOR_H
