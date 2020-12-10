//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#include "core/loader/obj.h"

#include "core/dynamic_array.inl"
#include "core/file_utils.h"
#include "core/linear_allocator.h"
#include "core/log.h"
#include "core/math/vec3.h"

#include <ctype.h>
#include <stdlib.h>

static void skip_space(char** p) {
  while (**p == ' ')
    ++(*p);
}

static void skip_till(char** p, char c) {
  while(**p != c)
    ++(*p);
}

static void string_to_vec(char** p, int len, float* v) {
  for (int j = 0; j < len; ++j) {
    skip_till(p, ' ');
    skip_space(p);
    v[j] = atof(*p);
  }
}

static inline int parse_index(int index, int len) {
  if (!index)
    return -1;
  if (index > 0)
    return index - 1;
  return index + len;
}

bool nj_obj_init(nj_obj_t* obj, nj_allocator_t* allocator, const nj_os_char* path) {
  nj_scoped_la_allocator_t<> temp_allocator("obj_temp_allocator");
  temp_allocator.init();

  int vs_count = 0;
  int uvs_count = 0;
  int ns_count = 0;
  int elems_count = 0;
  nj_dynamic_array_t<nju8> f = nj_read_whole_file(&temp_allocator, path, NULL);
  char* s = (char*)&f[0];
  char* e = (char*)f.p + nj_da_len(&f);
  for (;;) {
    while(isspace(*s))
      ++s;
    if(s[0] == 'v' && s[1] == ' ')
      ++vs_count;
    if(s[0] == 'v' && s[1] == 't')
      ++uvs_count;
    if(s[0] == 'v' && s[1] == 'n')
      ++ns_count;
    if(s[0] == 'f' && s[1] == ' ')
      ++elems_count;
    while (*s != '\n' && s != e)
      ++s;
    if (s == e)
      break;
  }
  nj_dynamic_array_t<nj_v4_t> vs;
  nj_dynamic_array_t<nj_v2_t> uvs;
  nj_dynamic_array_t<nj_v4_t> ns;
  nj_da_init(&vs, &temp_allocator);
  nj_da_init(&uvs, &temp_allocator);
  nj_da_init(&ns, &temp_allocator);
  nj_da_reserve(&vs, vs_count);
  nj_da_reserve(&uvs, vs_count);
  nj_da_reserve(&ns, vs_count);

  nj_da_init(&obj->vertices, allocator);
  nj_da_init(&obj->uvs, allocator);
  nj_da_init(&obj->normals, allocator);
  nj_da_reserve(&vs, elems_count);
  if (uvs_count)
    nj_da_reserve(&uvs, elems_count);
  if (ns_count)
    nj_da_reserve(&ns, elems_count);

  s = (char*)f.p;
  while (s != e) {
    while (isspace(*s))
      ++s;
    if (s[0] == 'v' && s[1] == ' ') {
      nj_v4_t v;
      string_to_vec(&s, 3, &v.x);
      v.w = 1.0f;
      nj_da_append(&vs, v);
    } else if (s[0] == 'v' && s[1] == 't') {
      nj_v2_t v;
      string_to_vec(&s, 2, &v.x);
      nj_da_append(&uvs, v);
    } else if (s[0] == 'v' && s[1] == 'n') {
      nj_v3_t v;
      string_to_vec(&s, 3, &v.x);
      v = nj_v3_normalize(v);
      nj_da_append(&ns, {v.x, v.y, v.z, 0.0f});
    } else if (s[0] == 'f' && s[1] == ' ') {
      for (int j = 0; j < 3; ++j) {
        int index;
        skip_till(&s, ' ');
        index = parse_index(atoi(++s), vs_count);
        nj_da_append(&obj->vertices, vs[index]);
        skip_till(&s, '/');
        index = parse_index(atoi(++s), uvs_count);
        if (index != -1)
          nj_da_append(&obj->uvs, uvs[index]);
        skip_till(&s, '/');
        index = parse_index(atoi(++s), ns_count);
        if (index != -1)
          nj_da_append(&obj->normals, ns[index]);
      }
    }
    while (*s != '\n' && s != e)
      ++s;
  }

  return true;
}

void nj_obj_destroy(nj_obj_t* obj) {
  nj_da_destroy(&obj->normals);
  nj_da_destroy(&obj->uvs);
  nj_da_destroy(&obj->vertices);
}
