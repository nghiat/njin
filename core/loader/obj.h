//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#ifndef NJ_CORE_LOADER_OBJ_H
#define NJ_CORE_LOADER_OBJ_H

#include "core/dynamic_array.h"
#include "core/math/vec2.h"
#include "core/math/vec4.h"
#include "core/os_string.h"

struct nj_allocator_t;

struct nj_obj_t {
  nj_dynamic_array_t<nj_v4_t> vertices;
  nj_dynamic_array_t<nj_v2_t> uvs;
  nj_dynamic_array_t<nj_v4_t> normals;
};

bool nj_obj_init(nj_obj_t* obj, nj_allocator_t* allocator, const nj_os_char* path);
void nj_obj_destroy(nj_obj_t* obj);

#endif // NJ_CORE_LOADER_OBJ_H
