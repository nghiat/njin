// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#ifndef NJ_CORE_MATH_MAT4_INL
#define NJ_CORE_MATH_MAT4_INL

#include "math/mat4.h"

#include <string.h>

inline nj_m4_t nj_m4_identity() {
  nj_m4_t m;
  m.a[0][0] = 1.0f;
  m.a[1][1] = 1.0f;
  m.a[2][2] = 1.0f;
  m.a[3][3] = 1.0f;
  return m;
}

inline nj_v4_t operator*(const nj_v4_t& v, const nj_m4_t& m) {
  nj_v4_t result;
  nj_m4_t temp;
  for (int j = 0; j < 4; ++j) {
    temp.a[j][0] = v.a[j] * m.a[j][0];
    temp.a[j][1] = v.a[j] * m.a[j][1];
    temp.a[j][2] = v.a[j] * m.a[j][2];
    temp.a[j][3] = v.a[j] * m.a[j][3];
  }
  result.x = temp.a[0][0] + temp.a[1][0] + temp.a[2][0] + temp.a[3][0];
  result.y = temp.a[0][1] + temp.a[1][1] + temp.a[2][1] + temp.a[3][1];
  result.z = temp.a[0][2] + temp.a[1][2] + temp.a[2][2] + temp.a[3][2];
  result.w = temp.a[0][3] + temp.a[1][3] + temp.a[2][3] + temp.a[3][3];
  return result;
}

nj_m4_t operator*(const nj_m4_t& m1, const nj_m4_t& m2) {
  nj_m4_t result;
  for (int i = 0; i < 4; ++i) {
    result.v[i] = vec4_mul_mat4(m1.v[i], m2);
  }
  return result;
}

inline bool operator==(const nj_m4_t& m1, const nj_m4_t& m2) {
  return !memcmp(m1.a, m2.a, sizeof(nj_m4_t));
}

inline nj_v4_t& operator=*(nj_v4_t& v, const nj_m4_t& m) {
  v = v * m;
  return v;
}

inline nj_m4_t& operator=*(nj_m4_t& m1, const nj_m4_t& m2) {
  m1 = m1 * m2;
  return m1;
}

#endif // NJ_CORE_MATH_MAT4_INL
