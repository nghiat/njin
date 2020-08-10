//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#ifndef NJ_CORE_CORE_ALLOCATORS_H
#define NJ_CORE_CORE_ALLOCATORS_H

struct nj_allocator_t;

// Allocate once and will never change.
extern nj_allocator_t* g_persistent_allocator;

// General purpose allocator.
extern nj_allocator_t* g_general_allocator;

bool nj_core_allocators_init();

#endif // NJ_CORE_CORE_ALLOCATORS_H
