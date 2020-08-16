//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#ifndef NJ_CORE_MATH_TRANSFORM_INL
#define NJ_CORE_MATH_TRANSFORM_INL

#include "core/math/transform.h"

#include <math.h>

inline nj_m4_t nj_look_forward_rh(const nj_v3_t& eye, const nj_v3_t& forward, const nj_v3_t& up) {
  nj_v3_t z_axis = nj_v3_normalize(forward * -1.f);
  nj_v3_t x_axis = nj_v3_normalize(nj_v3_cross(up, z_axis));
  nj_v3_t y_axis = nj_v3_cross(z_axis, x_axis);

  return {{x_axis.x, y_axis.x, z_axis.x, 0.f},
          {x_axis.y, y_axis.y, z_axis.y, 0.f},
          {x_axis.z, y_axis.z, z_axis.z, 0.f},
          {-nj_v3_dot(x_axis, eye), -nj_v3_dot(y_axis, eye), -nj_v3_dot(z_axis, eye), 1.f}};
}

inline nj_m4_t nj_look_at_rh(const nj_v3_t& eye, const nj_v3_t& target, const nj_v3_t& up) {
  return nj_look_forward_rh(eye, vec3_sub(eye, target), up);
}

inline nj_m4_t nj_perspective(njf32 fovy, njf32 aspect, njf32 z_near, njf32 z_far) {
  nj_m4_t result = nj_ZERO;
  result.a[1][1] = 1 / tanf(fovy / 2);
  result.a[0][0] = result.a[1][1] / aspect;
  result.a[2][2] = -(z_far + z_near) / (z_far - z_near);
  result.a[2][3] = -1.f;
  result.a[3][2] = -2.f * z_near * z_far / (z_far - z_near);
  return result;
}

inline nj_m4_t nj_rotate(const nj_v3_t& axis, njf32 angle) {
  nj_v3_t n = vec3_normalize(axis);
  njf32 x = n.x;
  njf32 y = n.y;
  njf32 z = n.z;
  njf32 c = cosf(angle);
  njf32 s = sinf(angle);
  return {{c + x * x * (1 - c), x * y * (1 - c) - z * s, x * z * (1 - c) + y * s, 0.f},
          {y * x * (1 - c) + z * s, c + y * y * (1 - c), y * z * (1 - c) - x * s, 0.f},
          {z * x * (1 - c) - y * s, z * y * (1 - c) + x * s, c + z * z * (1 - c), 0.f},
          {0.f, 0.f, 0.f, 1.f}};
}

inline nj_m4_t nj_scale(njf32 sx, njf32 sy, njf32 sz) {
  nj_m4_t m;
  m.a[0][0] = sx;
  m.a[1][1] = sy;
  m.a[2][2] = sz;
  m.a[3][3] = 1.f;
  return m;
}

inline nj_m4_t nj_m4_translate(const nj_v3_t& v) {
  return {{1.f, 0.f, 0.f, 0.f},
          {0.f, 1.f, 0.f, 0.f},
          {0.f, 0.f, 1.f, 0.f},
          {v.x, v.y, v.z, 1.f}};
}

#endif // NJ_CORE_MATH_TRANSFORM_INL
