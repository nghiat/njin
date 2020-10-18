//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#ifndef NJ_CORE_MATH_QUAT_INL
#define NJ_CORE_MATH_QUAT_INL

#include "core/math/quat.h"

#include "core/math/mat4.inl"
#include "core/math/vec3.inl"

#include <math.h>

// res * q = 1
nj_quat_t nj_quat_inverse(const nj_quat_t& q) {
  njf32 sum = q.a * q.a + q.b * q.b + q.c * q.c + q.d * q.d;
  return {q.a / sum, -q.b / sum, -q.c / sum, -q.d / sum};
}

njf32 nj_quat_norm(const nj_quat_t& q) {
  return sqrtf(q.a * q.a + q.b * q.b + q.c * q.c + q.d * q.d);
}

nj_quat_t nj_quat_normalize(const nj_quat_t& q) {
  njf32 norm = nj_quat_norm(q);
  return {q.a / norm, q.b / norm, q.c / norm, q.d / norm};
}

nj_m4_t nj_quat_to_m4(const nj_quat_t& q) {
  njf32 a = q.a;
  njf32 b = q.b;
  njf32 c = q.c;
  njf32 d = q.d;
  njf32 a2 = q.a * q.a;
  njf32 b2 = q.b * q.b;
  njf32 c2 = q.c * q.c;
  njf32 d2 = q.d * q.d;
  return {nj_v4_t{a2 + b2 - c2 - d2, 2 * (b * c + a * d), 2 * (b * d - a * c), 0.f},
          nj_v4_t{2 * (b * c - a * d), a2 - b2 + c2 - d2, 2 * (c * d + a * b), 0.f},
          nj_v4_t{2 * (b * d + a * c), 2 * (c * d - a * b), a2 - b2 - c2 + d2, 0.f},
          nj_v4_t{0.f, 0.f, 0.f, 1.f}};
}

// https://en.wikipedia.org/wiki/Quaternions_and_spatial_rotation#Proof_of_the_quaternion_rotation_identity
// q = cos(angle/2) + v*sin(angle/2)
nj_quat_t nj_quat_rotate_v3(const nj_v3_t& v, njf32 angle) {
  njf32 cos_half = cosf(angle / 2);
  njf32 sin_half = sinf(angle / 2);
  return {cos_half, sin_half * v.x, sin_half * v.y, sin_half * v.z};
}

#endif // NJ_CORE_MATH_QUAT_INL
