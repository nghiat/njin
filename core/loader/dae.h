//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#ifndef NJ_CORE_LOADER_DAE_H
#define NJ_CORE_LOADER_DAE_H

#include "core/dynamic_array.h"
#include "core/math/vec4.h"
#include "core/os_string.h"

struct nj_allocator_t;

struct nj_xml_node_t {
  char* tag_name;
  char* text;
  nj_dynamic_array_t<char*> attr_names;
  nj_dynamic_array_t<char*> attr_vals;
  nj_dynamic_array_t<struct nj_xml_node_t*> children;
};


struct nj_dae_t {
  nj_dynamic_array_t<nj_v4_t> vertices;
};

bool nj_dae_init(nj_dae_t* dae, nj_allocator_t* allocator, const nj_os_char* path);
void nj_dae_destroy(nj_dae_t* dae);

#endif // NJ_CORE_LOADER_DAE_H
