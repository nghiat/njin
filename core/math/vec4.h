//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#ifndef NJ_CORE_MATH_VEC4_H
#define NJ_CORE_MATH_VEC4_H

#include "core/njtype.h"

struct nj_v4_t {
  union {
    struct {
      njf32 x;
      njf32 y;
      njf32 z;
      njf32 w;
    };
    njf32 a[4] = {};
  };
};

nj_v4_t operator+(const nj_v4_t& v1, const nj_v4_t& v2);
nj_v4_t operator-(const nj_v4_t& v1, const nj_v4_t& v2);
nj_v4_t operator*(const nj_v4_t& v, njf32 f);
nj_v4_t operator/(const nj_v4_t& v, njf32 f);

nj_v4_t& operator+=(nj_v4_t& v1, const nj_v4_t& v2);
nj_v4_t& operator-=(nj_v4_t& v1, const nj_v4_t& v2);
nj_v4_t& operator*=(nj_v4_t& v, njf32 f);
nj_v4_t& operator/=(nj_v4_t& v, njf32 f);

bool operator==(const nj_v4_t& v1, const nj_v4_t& v2);

njf32 vec4_dot(const nj_v4_t& lhs, const nj_v4_t& rhs);
njf32 vec4_len(const nj_v4_t& v);
nj_v4_t vec4_normalize(const nj_v4_t& v);

#endif // NJ_CORE_MATH_VEC4_H
