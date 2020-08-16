//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#ifndef NJ_CORE_MATH_SPHERE_H
#define NJ_CORE_MATH_SPHERE_H

#include "math/vec3.h"

struct nj_sphere_t {
  nj_v3_t center;
  float radius;
};

#endif // NJ_CORE_MATH_SPHERE_H
