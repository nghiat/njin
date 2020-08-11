//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#ifndef NJ_CORE_DYNAMIC_ARRAY_H
#define NJ_CORE_DYNAMIC_ARRAY_H

struct nj_allocator_t;

#include "core/njtype.h"

template <typename T>
struct nj_dynamic_array_t {
  T* p = NULL;
  nj_allocator_t* allocator = NULL;
  njsp length = 0;
  njsp capacity = 0;

  T& operator[](njsz index);
};

template <typename T>
bool nj_da_init(nj_dynamic_array_t<T>* da, nj_allocator_t* allocator);

template <typename T>
void nj_da_destroy(nj_dynamic_array_t<T>* da);

template <typename T>
void nj_da_reserve(nj_dynamic_array_t<T>* da, njsp num);

template <typename T>
void nj_da_resize(nj_dynamic_array_t<T>* da, njsp num);

template <typename T>
void nj_da_remove_range(nj_dynamic_array_t<T>* da, njsp pos, njsp length);

template <typename T>
void nj_da_remove_at(nj_dynamic_array_t<T>* da, njsp pos);

template <typename T>
void nj_da_insert_at(nj_dynamic_array_t<T>* da, njsp index, const T& val);

template <typename T>
void nj_da_append(nj_dynamic_array_t<T>* da, const T& val);

#endif // NJ_CORE_DYNAMIC_ARRAY_H
