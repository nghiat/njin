//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#ifndef NJ_CORE_LINEAR_ALLOCATOR_H
#define NJ_CORE_LINEAR_ALLOCATOR_H

#include "core/allocator.h"
#include "core/njtype.h"

struct la_page_t;

// You can only free the most recent allocation.
template <njsz INITIAL_SIZE = 4096>
struct nj_linear_allocator_t : public nj_allocator_t {
  bool init(const char* name);
  void destroy() override;
  void* aligned_alloc(njsp size, njsp alignment) override;
  void* realloc(void* p, njsp size) override;
  void free(void* p) override;

  nju8 stack_page[INITIAL_SIZE];
  njsp default_page_size;
  la_page_t* current_page;
  nju8* top;
};

#endif // NJ_CORE_LINEAR_ALLOCATOR_H
