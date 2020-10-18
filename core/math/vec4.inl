//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#ifndef NJ_CORE_MATH_VEC4_INL
#define NJ_CORE_MATH_VEC4_INL

#include "core/math/vec4.h"

#include "core/math/vec3.h"

#include <math.h>
#include <string.h>

inline nj_v4_t operator+(const nj_v4_t& v1, const nj_v4_t& v2) {
  return {v1.x + v2.x, v1.y + v2.y, v1.z + v2.z, v1.w + v2.w};
}

inline nj_v4_t operator-(const nj_v4_t& v1, const nj_v4_t& v2) {
  return {v1.x - v2.x, v1.y - v2.y, v1.z - v2.z, v1.w - v2.w};
}

inline nj_v4_t operator*(const nj_v4_t& v, njf32 f) {
  return {v.x * f, v.y * f, v.z * f, v.w * f};
}

inline nj_v4_t operator/(const nj_v4_t& v, njf32 f) {
  return {v.x / f, v.y / f, v.z / f, v.w / f};
}

inline nj_v4_t& operator+=(nj_v4_t& v1, const nj_v4_t& v2) {
  v1 = v1 + v2;
  return v1;
}

inline nj_v4_t& operator-=(nj_v4_t& v1, const nj_v4_t& v2) {
  v1 = v1 - v2;
  return v1;
}

inline nj_v4_t& operator*=(nj_v4_t& v, njf32 f) {
  v = v * f;
  return v;
}

inline nj_v4_t& operator/=(nj_v4_t& v, njf32 f) {
  v = v / f;
  return v;
}

inline bool operator==(const nj_v4_t& v1, const nj_v4_t& v2) {
  return v1.x == v2.x && v1.y == v2.y && v1.z == v2.z && v1.w == v2.w;
}

inline njf32 nj_v4_dot(const nj_v4_t& v1, const nj_v4_t& v2) {
  return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z + v1.w * v2.w;
}

inline njf32 nj_v4_len(const nj_v4_t& v) {
  return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

inline nj_v4_t nj_v4_normalize(const nj_v4_t& v) {
  return v / nj_v4_len(v);
}

#endif // NJ_CORE_MATH_VEC4_INL
