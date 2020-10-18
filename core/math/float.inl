//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#ifndef NJ_CORE_MATH_FLOAT_INL
#define NJ_CORE_MATH_FLOAT_INL

#include "core/math/float.h"

#include <math.h>

inline bool nj_float_equal(float a, float b) {
  return fabs(a - b) < NJ_EPSILON_F;
}

inline bool nj_float_equal_0(float a) {
  return fabs(a) < NJ_EPSILON_F;
}

float nj_degree_to_rad(float deg) {
  return deg / 180.0f * NJ_PI_F;
}

#endif // NJ_CORE_MATH_FLOAT_INL
