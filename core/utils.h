//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#ifndef NJ_CORE_UTILS_H
#define NJ_CORE_UTILS_H

template <typename T>
void nj_maybe_assign(T* t, T v) {
  if (t)
    *t = v;
}

template <typename T>
const T& nj_min(const T& a, const T& b) {
  return a < b ? a : b;
}

template <typename T, njsz N>
njsz nj_static_array_size(const T(&)[N]) {
  return N;
}

#endif // NJ_CORE_UTILS_H
