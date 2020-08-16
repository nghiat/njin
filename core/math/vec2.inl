//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#ifndef NJ_CORE_MATH_VEC2_INL
#define NJ_CORE_MATH_VEC2_INL

#include "core/math/vec2.h"

#include <math.h>

inline nj_v2_t operator+(const nj_v2_t& v1, const nj_v2_t& v2) {
  return {v1.x + v2.x, v1.y + v2.y};
}

inline nj_v2_t operator-(const nj_v2_t& v1, const nj_v2_t& v2) {
  return {v1.x - v2.x, v1.y - v2.y};
}

inline nj_v2_t operator*(const nj_v2_t& v, njf32 f) {
  return {v.x * f, v.y * f};
}

inline nj_v2_t operator/(const nj_v2_t& v, njf32 f) {
  return {v.x / f, v.y / f};
}

inline nj_v2_t& operator+=(nj_v2_t& v1, const nj_v2_t& v2) {
  v1 = v1 + v2;
  return v1;
}

inline nj_v2_t& operator-=(nj_v2_t& v1, const nj_v2_t& v2) {
  v1 = v1 - v2;
  return v1;
}

inline nj_v2_t& operator*=(nj_v2_t& v, njf32 f) {
  v = v * f;
  return v;
}

inline nj_v2_t& operator/=(nj_v2_t& v, njf32 f) {
  v = v / f;
  return v;
}

inline njf32 nj_v2_dot(const nj_v2_t& v1, const nj_v2_t& v2) {
  return v1.x * v2.x + v1.y * v2.y;
}

inline nj_v2_t nj_v2_normalize(const nj_v2_t& v) {
  njf32 len = sqrt(v.x * v.x + v.x * v.x);
  return vec2_div(v, len);
}

#endif // NJ_CORE_MATH_VEC2_INL
