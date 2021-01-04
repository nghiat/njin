//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#ifndef NJ_CORE_MATH_VEC2_H
#define NJ_CORE_MATH_VEC2_H

#include "core/njtype.h"

struct nj_v2_t {
  union {
    struct {
      njf32 x;
      njf32 y;
    };
    njf32 a[2] = {};
  };
};

nj_v2_t operator+(const nj_v2_t& v1, const nj_v2_t& v2);
nj_v2_t operator-(const nj_v2_t& v1, const nj_v2_t& v2);
nj_v2_t operator*(const nj_v2_t& v1, const nj_v2_t& v2);
nj_v2_t operator/(const nj_v2_t& v1, const nj_v2_t& v2);

nj_v2_t operator*(const nj_v2_t& v, njf32 f);
nj_v2_t operator/(const nj_v2_t& v, njf32 f);

nj_v2_t& operator+=(nj_v2_t& v1, const nj_v2_t& v2);
nj_v2_t& operator-=(nj_v2_t& v1, const nj_v2_t& v2);
nj_v2_t& operator*=(nj_v2_t& v1, const nj_v2_t& v2);
nj_v2_t& operator/=(nj_v2_t& v1, const nj_v2_t& v2);

nj_v2_t& operator*=(nj_v2_t& v, njf32 f);
nj_v2_t& operator/=(nj_v2_t& v, njf32 f);

njf32 nj_v2_dot(const nj_v2_t& v1, const nj_v2_t& v2);
nj_v2_t nj_v2_normalize(const nj_v2_t& v);

#endif // NJ_CORE_MATH_VEC2_H
