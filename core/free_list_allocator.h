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
  bool init(const char* name, njsz size);
  void destroy() override;
  void* aligned_alloc(nju32 size, nju32 alignment) override;
  void* realloc(void* p, nju32 size) override;
  void free(void* p) override;

  nju8* start;
  freeblock_t* first_block;
};

#endif // NJ_CORE_FREE_LIST_ALLOCATOR_H
