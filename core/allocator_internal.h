//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#ifndef NJ_CORE_ALLOCATOR_INTERNAL_H
#define NJ_CORE_ALLOCATOR_INTERNAL_H

#include "core/build.h"
#include "core/njtype.h"

/// Here is the sketch for a general allocation.
/// start                    p         size        end
///  |                       |----------------------|
///  o_______________________o______________________o
///  |  |  allocation_header | allocation | padding |
///   o                                       |
///   |                                       o
/// prefix padding                        suffix padding (the remaning space
///  (alignment)                          is not enough for another
///                                       allocation_header)
/// An allocation_header is placed before any pointer returned by alloc()
/// or aligned_alloc() to keep track of the allocation (prefix padding is
/// used for alignment). In a case when space between the current allocation
/// and the next allocation is not enough for any allocation then the current
/// allocation will acquire the remaining space (suffix padding).
struct allocation_header_t {
  nju8* start;
  njsp size;
  njsp alignment;
#if NJ_IS_DEV()
  nju8* p;
#endif
};

allocation_header_t* get_allocation_header(void* p);
nju8* align_forward(nju8* p, njsp alignment);
bool check_aligned_alloc(njsp size, njsp alignment);
bool check_p_in_dev(void* p);

#endif // NJ_CORE_ALLOCATOR_INTERNAL_H
