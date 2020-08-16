//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#ifndef NJ_CORE_MATH_QUAT_H
#define NJ_CORE_MATH_QUAT_H

#include "core/math/mat4.h"
#include "core/math/vec3.h"

struct nj_quat_t {
  njf32 a;
  njf32 b;
  njf32 c;
  njf32 d;
};

nj_quat_t quat_inverse(const nj_quat_t& q);
njf32 quat_norm(const nj_quat_t& q);
nj_quat_t quat_normalize(const nj_quat_t& q);
nj_m4_t nj_quat_to_mat(const nj_quat_t& q);
nj_quat_t quat_rotate_v3(const nj_v3_t& v, njf32 angle);

#endif // NJ_CORE_MATH_QUAT_H
