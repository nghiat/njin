//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#ifndef NJ_CORE_DYNAMIC_ARRAY_INL
#define NJ_CORE_DYNAMIC_ARRAY_INL

#include "core/dynamic_array.h"

#include "core/allocator.h"
#include "core/log.h"

#include <stdlib.h>
#include <string.h>

template <typename T>
T& nj_dynamic_array_t<T>::operator[](njsz index) {
  return p[index];
}

template <typename T>
bool nj_da_init(nj_dynamic_array_t<T>* da, nj_allocator_t* allocator) {
  da->allocator = allocator;
  return true;
}

template <typename T>
void nj_da_destroy(nj_dynamic_array_t<T>* da) {
  if (da->p)
    da->allocator->free(da->p);
}

template <typename T>
njsp nj_da_len(const nj_dynamic_array_t<T>* da) {
  return da->length;
}

template <typename T>
void nj_da_reserve(nj_dynamic_array_t<T>* da, njsp num) {
  if (num <= da->capacity)
    return;
  if (!da->p)
    da->p = (T*)da->allocator->alloc(num * sizeof(T));
  else
    da->p = (T*)da->allocator->realloc(da->p, num * sizeof(T));
  NJ_CHECK_LOG_RETURN(da->p, "Can't reserve memory for nj_dynamic_array_t");
  da->capacity = num;
}

template <typename T>
void nj_da_resize(nj_dynamic_array_t<T>* da, njsp num) {
  nj_da_reserve(da, num);
  da->length = num;
}

template <typename T>
void nj_da_remove_range(nj_dynamic_array_t<T>* da, njsp pos, njsp length) {
  NJ_CHECK_LOG_RETURN(pos >= 0 && pos < da->length && pos + length < da->length, "Can't remove invalid rage");
  memmove(da->p + pos, da->p + pos + length, (da->length - pos - length) * sizeof(T));
  da->length -= length;
}

template <typename T>
void nj_da_remove_at(nj_dynamic_array_t<T>* da, njsp pos) {
  nj_da_remove_range(da, pos, 1);
}

template <typename T>
void nj_da_extend_if_necessary(nj_dynamic_array_t<T>* da) {
}

template <typename T>
void nj_da_insert_at(nj_dynamic_array_t<T>* da, njsp index, const T& val) {
  if (da->length == da->capacity) {
    nj_da_reserve(da, (da->capacity + 1) * 3 / 2);
  }
  if (index < da->length)
    memmove(da->p + index + 1, da->p + index, (da->length - index) * sizeof(T));
  da->p[index] = val;
  da->length += 1;
}

template <typename T>
void nj_da_append(nj_dynamic_array_t<T>* da, const T& val) {
  nj_da_insert_at(da, da->length, val);
}

#endif // NJ_CORE_DYNAMIC_ARRAY_INL
