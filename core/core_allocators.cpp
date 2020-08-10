//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#include "core_allocators.h"

#include "core/free_list_allocator.h"
#include "core/linear_allocator.h"

static nj_linear_allocator_t<> g_internal_persistent_allocator;
static nj_free_list_allocator_t g_internal_general_allocator;

nj_allocator_t* g_persistent_allocator = &g_internal_persistent_allocator;
nj_allocator_t* g_general_allocator = &g_internal_general_allocator;

bool nj_core_allocators_init() {
  bool rv = true;
  rv &= g_internal_persistent_allocator.init("persistent_allocator");
  rv &= g_internal_general_allocator.init("general_allocator", 10 * 1024 * 1024);
  return rv;
}
