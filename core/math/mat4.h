//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#ifndef NJ_CORE_MATH_MAT4_H
#define NJ_CORE_MATH_MAT4_H

#include "core/math/vec4.h"

// Row-major
struct nj_m4_t {
  union {
    nj_v4_t v[4] = {};
    njf32 a[4][4];
  };
};

nj_m4_t nj_m4_identity();

nj_v4_t operator*(const nj_m4_t& m, const nj_v4_t& v);
nj_m4_t operator*(const nj_m4_t& m1, const nj_m4_t& m2);
bool operator==(const nj_m4_t& m1, const nj_m4_t& m2);

nj_m4_t& operator*=(nj_m4_t& m1, const nj_m4_t& m2);

#endif // NJ_CORE_MATH_MAT4_H
