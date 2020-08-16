//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#ifndef NJ_CORE_MATH_RAY_H
#define NJ_CORE_MATH_RAY_H

#include "core/math/plane.h"
#include "core/math/sphere.h"
#include "core/math/triangle.h"
#include "core/math/vec3.h"

struct nj_ray_t {
  nj_v3_t origin;
  nj_v3_t dir;
};

nj_v3_t nj_ray_at(const nj_ray_t& r, njf32 t);
bool nj_ray_hit_plane(const nj_ray_t& r, const nj_plane_t& p, njf32* out_t);
bool nj_ray_hit_sphere(const nj_ray_t& r, const nj_sphere_t& s, njf32* out_t);
bool nj_ray_hit_triangle(const nj_ray_t& r, const nj_triangle_t& t);

#endif // NJ_CORE_MATH_RAY_H
