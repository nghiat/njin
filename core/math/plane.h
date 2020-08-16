//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#ifndef NJ_CORE_MATH_PLANE_H
#define NJ_CORE_MATH_PLANE_H

#include "core/math/vec3.h"

// dot((p - p0), n) = 0
struct nj_plane_t {
  nj_v3_t normal;
  nj_v3_t p0;
};

#endif // NJ_CORE_MATH_PLANE_H
