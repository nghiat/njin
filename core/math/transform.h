//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#ifndef NJ_CORE_MATH_TRANSFORM_H
#define NJ_CORE_MATH_TRANSFORM_H

#include "core/math/mat4.h"
#include "core/math/vec3.h"

nj_m4_t nj_look_forward_lh(const nj_v3_t& eye, const nj_v3_t& forward, const nj_v3_t& up);
nj_m4_t nj_look_at_lh(const nj_v3_t& eye, const nj_v3_t& target, const nj_v3_t& up);
nj_m4_t nj_perspective(njf32 fovy, njf32 aspect, njf32 z_near, njf32 z_far);
nj_m4_t nj_rotate(const nj_v3_t& axis, njf32 angle);
nj_m4_t nj_scale(njf32 sx, njf32 sy, njf32 sz);
nj_m4_t nj_translate(const nj_v3_t& v);

#endif // NJ_CORE_MATH_TRANSFORM_H
