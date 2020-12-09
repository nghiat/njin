//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#ifndef NJ_CORE_MATH_MAT4_INL
#define NJ_CORE_MATH_MAT4_INL

#include "core/math/mat4.h"

#include "core/math/vec4.inl"

#include <string.h>

inline nj_m4_t nj_m4_identity() {
  nj_m4_t m;
  m.a[0][0] = 1.0f;
  m.a[1][1] = 1.0f;
  m.a[2][2] = 1.0f;
  m.a[3][3] = 1.0f;
  return m;
}

inline nj_v4_t operator*(const nj_m4_t& m, const nj_v4_t& v) {
  nj_v4_t result;
  for (int i = 0; i < 4; ++i) {
    result.a[i] = nj_v4_dot(m.v[i], v);
  }
  return result;
}

inline nj_m4_t operator*(const nj_m4_t& m1, const nj_m4_t& m2) {
  nj_m4_t result;
  // Multiply |each row of m1| with m2.
  for (int i = 0; i < 4; ++i) {
    // Each row of result is the sum of products of m1 row with every m2 row.
    for (int j = 0; j < 4; ++j) {
      for (int k = 0; k < 4; ++k) {
        result.a[i][k] += m1.a[i][j] * m2.a[j][k];
      }
    }
  }
  return result;
}

inline bool operator==(const nj_m4_t& m1, const nj_m4_t& m2) {
  return !memcmp(m1.a, m2.a, sizeof(nj_m4_t));
}

inline nj_m4_t& operator*=(nj_m4_t& m1, const nj_m4_t& m2) {
  m1 = m1 * m2;
  return m1;
}

#endif // NJ_CORE_MATH_MAT4_INL
