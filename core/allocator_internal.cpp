//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#include "core/allocator_internal.h"

#include "core/log.h"

allocation_header_t* get_allocation_header(void* p) {
  return (allocation_header_t*)p - 1;
}

nju8* align_forward(nju8* p, nju32 alignment) {
  if (alignment == 1)
    return p;
  return (nju8*)(((size_t)p + alignment - 1) & ~(size_t)(alignment - 1));
}

bool check_aligned_alloc(nju32 size, nju32 alignment) {
  NJ_CHECKF_RETURN_VAL(size && alignment, false, "Invalid size or alignment");

  // alignment has to be power of two.
  if (alignment & (alignment - 1)) {
    return false;
  }
  return true;
}

bool check_p_in_dev(void* p) {
  NJ_CHECKF_RETURN_VAL(p, false, "Invalid pointer or size");

#if NJ_IS_DEV()
  allocation_header_t* header = get_allocation_header(p);
  if (header->p != p) {
    return false;
  }
#endif
  return true;
}
