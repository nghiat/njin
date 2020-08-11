//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#ifndef NJ_CORE_HASH_TABLE_H
#define NJ_CORE_HASH_TABLE_H

#include "core/dynamic_array.h"
#include "core/njtype.h"

#define NJ_HT_INVALID_KEY (0)
#define NJ_HT_INVALID_INDEX (-1)

struct nj_hash_table_t {
  nj_dynamic_array_t<njup> keys;
  nj_dynamic_array_t<njup> values;
  njsz key_size;
  njsp key_count;
  njf32 load_factor;
  njf32 resize_ratio;
  nju8 probe_count;
};

bool nj_ht_init(nj_hash_table_t* ht, nj_allocator_t* allocator, int key_size);
void nj_ht_destroy(nj_hash_table_t* ht);

njsp nj_ht_get_ptr(nj_hash_table_t* ht, void* key, int key_size);
njsp nj_ht_insert_ptr(nj_hash_table_t* ht, void* key, int key_size);
void nj_ht_remove_ptr(nj_hash_table_t* ht, void* key, int key_size);

njsp nj_ht_get_uintptr(nj_hash_table_t* ht, njup key);
njsp nj_ht_insert_uintptr(nj_hash_table_t* ht, njup key);
void nj_ht_remove_uintptr(nj_hash_table_t* ht, njup key);

njsp nj_ht_get_str(nj_hash_table_t* ht, const char* key);
njsp nj_ht_insert_str(nj_hash_table_t* ht, const char* key);
void nj_ht_remove_str(nj_hash_table_t* ht, const char* key);

#endif // NJ_CORE_HASH_TABLE_H
