//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#ifndef NJ_CORE_MATH_TRANSFORM_INL
#define NJ_CORE_MATH_TRANSFORM_INL

#include "core/math/transform.h"

#include "core/math/vec3.inl"

#include <math.h>

inline nj_m4_t nj_look_forward_lh(const nj_v3_t& eye, const nj_v3_t& forward, const nj_v3_t& up) {
  nj_v3_t z_axis = nj_v3_normalize(forward);
  nj_v3_t x_axis = nj_v3_normalize(nj_v3_cross(z_axis, up));
  nj_v3_t y_axis = nj_v3_cross(x_axis, z_axis);

  // M * [x_asix, 0] = [1, 0, 0, 0]
  // M * [y_asix, 0] = [0, 1, 0, 0]
  // M * [z_asix, 0] = [0, 0, 1, 0]
  // M * [eye, 1] = [0, 0, 0, 1]
  return {nj_v4_t{x_axis.x, x_axis.y, x_axis.z, -nj_v3_dot(x_axis, eye)},
          nj_v4_t{y_axis.x, y_axis.y, y_axis.z, -nj_v3_dot(y_axis, eye)},
          nj_v4_t{z_axis.x, z_axis.y, z_axis.z, -nj_v3_dot(z_axis, eye)},
          nj_v4_t{0.0f, 0.0f, 0.0f, 1.0f}};
}

inline nj_m4_t nj_look_at_lh(const nj_v3_t& eye, const nj_v3_t& target, const nj_v3_t& up) {
  return nj_look_forward_lh(eye, eye - target, up);
}

inline nj_m4_t nj_perspective(njf32 fovy, njf32 aspect, njf32 z_near, njf32 z_far) {
  nj_m4_t result;
  result.a[0][0] = 1 / tanf(fovy / 2);
  result.a[1][1] = result.a[0][0] * aspect;
  result.a[2][2] = -z_far / (z_near - z_far);
  result.a[2][3] = z_near*z_far / (z_near - z_far);
  result.a[3][2] = 1.f;
  return result;
}

// Split vector v that is being rotated into othorgonal vectors v1 and v2 (v1 lies on this axis).
// v' is the rotated vector and v' =  v1 + v2'
// v2' = v * cos(a) + u * sin(a) (u = cross(axis, v))
// Finally we have: v' = cos(a)*v + (1-cos(a))(dot(v, axis) * axis) + sin(a) * cross(axis, v)
inline nj_m4_t nj_rotate(const nj_v3_t& axis, njf32 angle) {
  nj_v3_t n = nj_v3_normalize(axis);
  njf32 x = n.x;
  njf32 y = n.y;
  njf32 z = n.z;
  njf32 c = cosf(angle);
  njf32 s = sinf(angle);
  return {nj_v4_t{c + x * x * (1 - c), y * x * (1 - c) + z * s, z * x * (1 - c) - y * s, 0.f},
          nj_v4_t{x * y * (1 - c) - z * s, c + y * y * (1 - c), z * y * (1 - c) + x * s, 0.f},
          nj_v4_t{x * z * (1 - c) + y * s, y * z * (1 - c) - x * s, c + z * z * (1 - c), 0.f},
          nj_v4_t{0.f, 0.f, 0.f, 1.f}};
}

inline nj_m4_t nj_scale(njf32 sx, njf32 sy, njf32 sz) {
  nj_m4_t m;
  m.a[0][0] = sx;
  m.a[1][1] = sy;
  m.a[2][2] = sz;
  m.a[3][3] = 1.f;
  return m;
}

inline nj_m4_t nj_translate(const nj_v3_t& v) {
  return {nj_v4_t{1.f, 0.f, 0.f, v.x},
          nj_v4_t{0.f, 1.f, 0.f, v.y},
          nj_v4_t{0.f, 0.f, 1.f, v.z},
          nj_v4_t{0.f, 0.f, 0.f, 1.f}};
}

#endif // NJ_CORE_MATH_TRANSFORM_INL
