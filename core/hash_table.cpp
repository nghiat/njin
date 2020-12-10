//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#include "core/hash_table.h"

#include "core/allocator.h"
#include "core/linear_allocator.h"
#include "core/log.h"

#include <string.h>

static nju64 fnv1_hash(const void* p, njsz size) {
  nju64 hash = 0xcbf29ce484222325;
  for (njsz i = 0; i < size; ++i) {
    hash = hash * 1099511628211;
    hash = hash ^ *((nju8*)p + i);
  }
  return hash;
}

static void ht_rehash(nj_hash_table_t* ht) {
  int key_size = ht->key_size;
  size_t cap = ht->keys.length;
  int new_cap = ht->resize_ratio * cap + 2;
  nj_da_resize(&ht->keys, new_cap);
  nj_da_resize(&ht->values, new_cap);
  memset(&ht->keys[0] + cap, 0, (new_cap - cap) * sizeof(ht->keys[0]));
  nj_linear_allocator_t<> tmp_allocator("ht_resize_tracking_allocator");
  tmp_allocator.init();
  // rehash_tracker: 0 means the index not been moved, 1 otherwise
  nj_dynamic_array_t<char> rehash_tracker;
  nj_da_init(&rehash_tracker, &tmp_allocator);
  nj_da_resize(&rehash_tracker, new_cap);
  memset(&rehash_tracker[0], 0, new_cap);
  for (int i = 0; i < cap; ++i) {
    if (ht->keys[i] != NJ_HT_INVALID_KEY)
      rehash_tracker[i] = 1;
  }
  for(int i = 0; i < new_cap; ++i) {
    if (ht->keys[i] != NJ_HT_INVALID_KEY) {
      int new_index = fnv1_hash((void*)ht->keys[i], key_size) % new_cap;
      while (rehash_tracker[new_index % new_cap])
        ++new_index;
      // No collision
      if (new_index <= i) {
        memcpy(&ht->keys[new_index], &ht->keys[i], sizeof(ht->keys[0]));
        memcpy(&ht->values[new_index], &ht->values[i], sizeof(ht->values[0]));
        rehash_tracker[new_index] = 1;
        continue;
      }
      if (ht->keys[new_index] != NJ_HT_INVALID_KEY) {
        memcpy(&ht->keys[new_index], &ht->keys[i], sizeof(ht->keys[0]));
        memcpy(&ht->values[new_index], &ht->values[i], sizeof(ht->values[0]));
        rehash_tracker[new_index] = 1;
        // We need to check i again.
        --i;
      }
    }
  }
  tmp_allocator.destroy();
}

static njsp ht_get(nj_hash_table_t* ht, njup key, int key_size, nju64 hash) {
  NJ_CHECK_RETURN_VAL(key_size == ht->key_size, NJ_HT_INVALID_INDEX);
  njsp len = ht->keys.length;
  njsp idx = hash % len;
  for (int i = 0; i < ht->probe_count; ++i, ++idx) {
    if (ht->keys[idx] != NJ_HT_INVALID_KEY && !memcmp((void*)key, (void*)ht->keys[idx], key_size)) {
      return idx;
    }
  }
  return NJ_HT_INVALID_INDEX;
}

static njsp ht_insert(nj_hash_table_t* ht, njup key, int key_size, nju64 hash) {
  NJ_CHECK_RETURN_VAL(key_size == ht->key_size, NJ_HT_INVALID_INDEX);
  njsp cap = ht->keys.length;
  if (ht->load_factor * cap < ht->key_count + 1) {
    ht_rehash(ht);
  }
  while (true) {
    cap = ht->keys.length;
    njsp index = hash % cap;
    for (int i = 0; i < ht->probe_count; ++i) {
      if (ht->keys[index] == NJ_HT_INVALID_KEY) {
        ht->keys[index] = (njup)key;
        ++ht->key_count;
        return index;
      }
      index = (index + 1) % cap;
    }
    ht_rehash(ht);
  }
  return NJ_HT_INVALID_INDEX;
}

static void ht_remove(nj_hash_table_t* ht, njup key, int key_size, nju64 hash) {
  NJ_CHECK_RETURN(key_size == ht->key_size);
  njsp len = ht->keys.length;
  njsp idx = hash % len;
  for (int i = 0; i < ht->probe_count; ++i, ++idx) {
    if (ht->keys[idx] != NJ_HT_INVALID_KEY && !memcmp((void*)key, (void*)ht->keys[idx], key_size)) {
       ht->keys[idx] = NJ_HT_INVALID_KEY;
    }
  }
}

bool nj_ht_init(nj_hash_table_t* ht, nj_allocator_t* allocator, int key_size) {
  NJ_CHECK_RETURN_VAL(nj_da_init(&ht->keys, allocator), false);
  NJ_CHECK_RETURN_VAL(nj_da_init(&ht->values, allocator), false);
  ht->key_size = key_size;
  ht->key_count = 0;
  ht->load_factor = 0.65f;
  ht->resize_ratio = 1.5f;
  ht->probe_count = 5;
  return true;
}

void nj_ht_destroy_internal(nj_hash_table_t* ht) {
  nj_da_destroy(&ht->keys);
  nj_da_destroy(&ht->values);
}

njsp nj_ht_get_ptr(nj_hash_table_t* ht, void* key, int key_size) {
  return ht_get(ht, (njup)key, key_size, fnv1_hash(key, key_size));
}

njsp nj_ht_insert_ptr(nj_hash_table_t* ht, void* key, int key_size) {
  return ht_insert(ht, (njup)key, key_size, fnv1_hash(key, key_size));
}

void nj_ht_remove_ptr(nj_hash_table_t* ht, void* key, int key_size) {
  ht_remove(ht, (njup)key, key_size, fnv1_hash(key, key_size));
}

njsp nj_ht_get_uintptr(nj_hash_table_t* ht, njup key) {
  return ht_get(ht, (njup)&key, sizeof(key), fnv1_hash(&key, sizeof(key)));
}

njsp nj_ht_insert_uintptr(nj_hash_table_t* ht, njup key) {
  return ht_insert(ht, (njup)&key, sizeof(key), fnv1_hash(&key, sizeof(key)));
}

void nj_ht_remove_uintptr(nj_hash_table_t* ht, njup key) {
  ht_remove(ht, (njup)&key, sizeof(key), fnv1_hash(&key, sizeof(key)));
}

njsp nj_ht_get_str(nj_hash_table_t* ht, const char* key) {
  nju64 hash = fnv1_hash(key, strlen(key));
  return ht_get(ht, (njup)&hash, sizeof(nju64), hash);
}

njsp nj_ht_insert_str(nj_hash_table_t* ht, const char* key) {
  // We can't insert the str directly cause it causes rehashing more complicated.
  nju64 hash = fnv1_hash(key, strlen(key));
  return ht_insert(ht, (njup)&hash, sizeof(nju64), hash);
}

void nj_ht_remove_str(nj_hash_table_t* ht, const char* key) {
  nju64 hash = fnv1_hash(key, strlen(key));
  ht_remove(ht, (njup)&hash, sizeof(nju64), hash);
}
