//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#ifndef NJ_CORE_MATH_RAY_INL
#define NJ_CORE_MATH_RAY_INL

#include "core/math/ray.h"

#include <math.h>

nj_v3_t nj_ray_at(nj_ray_t r, njf32 t) {
  return r.origin + r.dir * t;
}

bool ray_hit_plane(nj_ray_t r, plane_t p, njf32* out_t) {
  njf32 denominator = nj_v3_dot(r.dir, p.normal);
  if (!nj_float_equal_0(denominator)) {
    // Solve vec3_dot((o + td - p) , n) = 0;
    njf32 t = nj_v3_dot(p.p0 - r.origin, p.normal) / denominator;
    if (t > 0.f) {
      if (out_t) {
        *out_t = t;
      }
      return true;
    }
  }
  return false;
}

bool nj_ray_hit_sphere(nj_ray_t r, sphere_t s, njf32* out_t) {
  // This equation of t has roots: (r.o + t*r.d.x - s.c.x)^2 + ... = s.r^2
  njf32 temp_x = r.origin.x - s.center.x;
  njf32 temp_y = r.origin.y - s.center.y;
  njf32 temp_z = r.origin.z - s.center.z;
  njf32 a = r.dir.x * r.dir.x + r.dir.y * r.dir.y + r.dir.z * r.dir.z;
  njf32 b = r.dir.x * temp_x + r.dir.y * temp_y + r.dir.z * temp_z;
  njf32 c = temp_x * temp_x + temp_y * temp_y + temp_z * temp_z - s.radius * s.radius;
  njf32 delta = b * b - a * c;
  if (delta < 0.f)
    return false;
  // Both roots have to be positive.
  if (-b / a >= 0.f && c / a >= 0.f) {
    if (out_t) {
      *out_t = (-b - sqrt(delta)) / a;
    }
    return true;
  }
  return false;
}

bool nj_ray_hit_triangle(nj_ray_t r, triangle_t tri) {
  // Möller-Trumbore
  // Same variable names as the original paper.
  nj_v3_t e1 = tri.vertices[1] - tri.vertices[0];
  nj_v3_t e2 = tri.vertices[2] - tri.vertices[0];
  nj_v3_t p = nj_v3_cross(r.dir, e2);
  njf32 det = nj_v3_dot(e1, p);
  if (nj_float_equal_0(det))
    return false;
  njf32 inv_det = 1.f / det;
  nj_v3_t t = r.origin - tri.vertices[0];
  njf32 u = nj_v3_t(t, p) * inv_det;
  if (u < 0.f || u > 1.f)
    return false;
  nj_v3_t q = nj_v3_cross(t, e1);
  njf32 v = nj_v3_dot(r.dir, q) * inv_det;
  if (v < 0.f || u + v > 1.f)
    return false;
  return true;
}

#endif // NJ_CORE_MATH_RAY_INL
