//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#ifndef NJ_CORE_MATH_VEC3_H
#define NJ_CORE_MATH_VEC3_H

#include "core/njtype.h"

struct nj_v4_t;

struct nj_v3_t {
  nj_v3_t& operator=(const nj_v4_t& v);

  union {
    struct {
      njf32 x;
      njf32 y;
      njf32 z;
    };
    njf32 a[3] = {};
  };
};

nj_v3_t operator+(const nj_v3_t& v1, const nj_v3_t& v2);
nj_v3_t operator-(const nj_v3_t& v1, const nj_v3_t& v2);
nj_v3_t operator*(const nj_v3_t& v, njf32 f);
nj_v3_t operator/(const nj_v3_t& v, njf32 f);

nj_v3_t& operator+=(nj_v3_t& v1, const nj_v3_t& v2);
nj_v3_t& operator-=(nj_v3_t& v1, const nj_v3_t& v2);
nj_v3_t& operator*=(nj_v3_t& v, njf32 f);
nj_v3_t& operator/=(nj_v3_t& v, njf32 f);

bool operator==(const nj_v3_t& v1, const nj_v3_t& v2);

nj_v3_t nj_v3_cross(const nj_v3_t& v1, const nj_v3_t& v2);
njf32 nj_v3_dot(const nj_v3_t& v1, const nj_v3_t& v2);
njf32 nj_v3_len(const nj_v3_t& v);
nj_v3_t nj_v3_normalize(const nj_v3_t& v);

#endif // NJ_CORE_MATH_VEC3_H
