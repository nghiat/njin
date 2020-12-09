//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#ifndef NJ_CORE_FREE_LIST_ALLOCATOR_H
#define NJ_CORE_FREE_LIST_ALLOCATOR_H

#include "core/allocator.h"

#include "core/njtype.h"

struct freeblock_t;

/// An allocator that keeps a linked list of blocks of available spaces.
/// When you request an allocation, it finds the smallest block that can keeps
/// the size of the allcation then shrinks that block. When you request a
/// freeation, it creates a new blocks and merges with nearby blocks if
/// they are contiguous.
struct nj_free_list_allocator_t : public nj_allocator_t {
  nj_free_list_allocator_t(const char* name, njsz total_size) : nj_allocator_t(name, total_size) {}
  bool init();
  void destroy() override;
  void* aligned_alloc(njsp size, njsp alignment) override;
  void* realloc(void* p, njsp size) override;
  void free(void* p) override;

  nju8* m_start;
  freeblock_t* m_first_block;
};

#endif // NJ_CORE_FREE_LIST_ALLOCATOR_H
