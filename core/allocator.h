//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#ifndef CORE_ALLOCATOR_H
#define CORE_ALLOCATOR_H

#include "core/njtype.h"

struct nj_allocator_t {
  nj_allocator_t(const char* name, njsz total_size) : m_name(name), m_total_size(total_size) {}
  virtual void destroy() = 0;
  virtual void* aligned_alloc(njsp size, njsp alignment) = 0;
  virtual void* realloc(void* p, njsp size) = 0;
  virtual void free(void* p) = 0;

  void* alloc(njsp size);
  void* alloc_zero(njsp size);

  const char* m_name = nullptr;
  /// Total size of the allocator in bytes.
  njsp m_total_size = 0;
  /// Allocations' sizes and supporting data of the allocator in bytes.
  njsp m_used_size = 0;
};

#endif // CORE_ALLOCATOR_H
