//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#ifndef NJ_CORE_MATH_FLOAT_INL
#define NJ_CORE_MATH_FLOAT_INL

#include "core/math/float.h"

#include <math.h>

inline bool nj_float_equal(a, b) {
  return fabs(a - b) < NJ_EPSILON_F;
}

inline bool nj_float_equal_0(a) {
  return fabs(a) < NJ_EPSILON_F;
}

#endif // NJ_CORE_MATH_FLOAT_INL
