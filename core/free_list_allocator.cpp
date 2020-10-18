//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#include "core/free_list_allocator.h"

#include "core/allocator_internal.h"
#include "core/log.h"

#include <stdlib.h>
#include <string.h>

// freeblock_t and allocations use a shared memory region.
struct freeblock_t {
  /// |size| includes this struct.
  njsp size;
  freeblock_t* next;
};

static_assert(
    sizeof(allocation_header_t) >= sizeof(freeblock_t),
    "We assume that sizeof(allocation_header_t) >= sizeof(freeblock_t) so when "
    "freeating an allocation, we always have space for a freeblock_t");

// Finds the smallest possible block that can fits the |requiredSize| with
// |alignment| and returns the pointer to the beginning of the allocation
// region.
static nju8* find_best_fit_free_block(const nj_free_list_allocator_t* fla,
                                      njsp size,
                                      njsp alignment,
                                      freeblock_t** o_fit_block,
                                      freeblock_t** o_prior_block) {
  nju8* p = NULL;
  freeblock_t* curr_block = fla->first_block;
  freeblock_t* curr_prior_block = NULL;
  *o_fit_block = NULL;
  *o_prior_block = NULL;
  while (curr_block) {
    nju8* curr_p = (nju8*)curr_block + sizeof(allocation_header_t);
    // Align the returned pointer if neccessary.
    curr_p = align_forward(curr_p, alignment);
    if (curr_block->size >= (curr_p - (nju8*)curr_block) + size) {
      if (!*(o_fit_block) || (curr_block->size < (*o_fit_block)->size)) {
        *o_fit_block = curr_block;
        *o_prior_block = curr_prior_block;
        p = curr_p;
      }
    }
    curr_prior_block = curr_block;
    curr_block = curr_block->next;
  }
  if (!(*o_fit_block))
    return NULL;
  return p;
}

// Get the free_block before and after |p|.
static void get_adjacent_blocks(const nj_free_list_allocator_t* fla,
                                nju8* p,
                                freeblock_t** prior_block,
                                freeblock_t** next_block) {
  *prior_block = NULL;
  *next_block = fla->first_block;
  while (*next_block && p > (nju8*)(*next_block)) {
    *prior_block = *next_block;
    *next_block = (*next_block)->next;
  }
}

// Links 2 consecutive separated blocks. If the |priorBlock| is null,
// set |first_free_block_| to |block|.
static void link_separated_blocks(nj_free_list_allocator_t* fla,
                                  freeblock_t* block,
                                  freeblock_t* prior_block) {
  if (prior_block)
    prior_block->next = block;
  else
    fla->first_block = block;
}

// Returns true if |size| is bigger than sizeof(allocation_header_t) which we
// assume that it is bigger than sizeof(freeblock_t).
static bool is_enough_for_allocation_header(njsp size) {
  return size > sizeof(allocation_header_t);
}

static bool is_allocaiton_adjacent_to_free_block(allocation_header_t* header,
                                                 freeblock_t* block) {
  return block && (nju8*)header + sizeof(allocation_header_t) + header->size == (nju8*)block;
}

// Link two blocks together. If they are contiguous, merge them
// and assign *second to *first.
static void link_and_merge_free_blocks(nj_free_list_allocator_t* fla,
                                       freeblock_t** first,
                                       freeblock_t** second) {
  if (!*first) {
    fla->first_block = *second;
    return;
  }
  // Merge two blocks if they are contiguous.
  if ((nju8*)(*first) + (*first)->size == (nju8*)(*second)) {
    (*first)->size += (*second)->size;
    (*first)->next = (*second)->next;
    *second = *first;
  } else
    (*first)->next = *second;
}

// Add a freeblock_t to the freeblock_t linked list.
// If the freeblock_t is adjacent to the prior block or the next block, they
// will be merged
static void add_and_merge_free_block(nj_free_list_allocator_t* fla,
                                     freeblock_t* block) {
  freeblock_t* prior_block = NULL;
  freeblock_t* next_block = fla->first_block;
  get_adjacent_blocks(fla, (nju8*)block, &prior_block, &next_block);
  if (prior_block)
    link_and_merge_free_blocks(fla, &prior_block, &block);
  else
    fla->first_block = block;

  if (next_block)
    link_and_merge_free_blocks(fla, &block, &next_block);
}

// Shrink |block| (remove |size| on the left) and link the new |freeblock_t|
// with |prior_block| if it is possible. Returns true if the block can be
// shrunk (means that there is enough space for at least a allocation_header_t
// after shrinking), false otherwise.
static bool shrink_free_block(nj_free_list_allocator_t* fla,
                              freeblock_t* block,
                              freeblock_t* prior_block,
                              njsp size) {
  njsp block_size_after = block->size - size;
  if (!is_enough_for_allocation_header(block_size_after)) {
    if (prior_block)
      prior_block->next = block->next;
    else
      fla->first_block = block->next;
    fla->used_size += block->size;
    return false;
  }
  freeblock_t* shrunk_block = (freeblock_t*)((nju8*)block + size);
  *shrunk_block = (freeblock_t){.size = block_size_after, .next = block->next};
  link_separated_blocks(fla, shrunk_block, prior_block);
  fla->used_size += size;
  return true;
}

static void realloc_smaller(nj_free_list_allocator_t* fla,
                            nju8* p,
                            njsp size,
                            freeblock_t* prior_block,
                            freeblock_t* next_block) {
  allocation_header_t* header = get_allocation_header(p);
  njsp size_after_shrunk = header->size - size;
  // If there is a freeblock_t right after the allocation, we shift the
  // freeblock_t to the end of the new allocation.
  if (is_allocaiton_adjacent_to_free_block(header, next_block)) {
    header->size = size;
    freeblock_t* shifted_block = (freeblock_t*)(p + size);
    shifted_block->size = next_block->size + size_after_shrunk;
    shifted_block->next = next_block->next;
    link_and_merge_free_blocks(fla, &prior_block, &shifted_block);
    fla->used_size -= size_after_shrunk;
    return;
  }
  // There is not enough space for a new freeblock_t, nothing changes.
  if (!is_enough_for_allocation_header(size_after_shrunk)) {
    return;
  }
  // Else, we create a new freeblock_t.
  header->size = size;
  freeblock_t* new_block = (freeblock_t*)(p + size);
  new_block->size = size_after_shrunk;
  new_block->next = next_block;
  link_separated_blocks(fla, new_block, prior_block);
  fla->used_size -= size_after_shrunk;
}

static nju8* realloc_bigger(nj_free_list_allocator_t* fla,
                               nju8* p,
                               njsp size,
                               freeblock_t* prior_block,
                               freeblock_t* next_block) {
  allocation_header_t* header = get_allocation_header(p);
  // Check if |next_block| is adjacent to |p| so we may extend |p|.
  if (is_allocaiton_adjacent_to_free_block(header, next_block)) {
    if (header->size + next_block->size >= size) {
      njsp size_after_extended = header->size + next_block->size - size;
      if (is_enough_for_allocation_header(size_after_extended)) {
        freeblock_t* new_block = (freeblock_t*)(p + size);
        new_block->size = size_after_extended;
        new_block->next = next_block->next;
        link_and_merge_free_blocks(fla, &prior_block, &new_block);
        fla->used_size += size - header->size;
        header->size = size;
      } else {
        fla->used_size += next_block->size;
        header->size += next_block->size;
        link_separated_blocks(fla, next_block->next, prior_block);
      }
      return p;
    }
  }
  // Else, find other freeblock_t.
  // If sizeof(allocation_header_t) is smaller than sizeof(freeblock_t), then
  // the |new_block| will overwrite the data of p
  allocation_header_t backup_header = *header;
  freeblock_t backup_prior_block;
  freeblock_t backup_next_block;
  if (prior_block)
    backup_prior_block = *prior_block;
  if (next_block)
    backup_next_block = *next_block;
  freeblock_t* backup_first_block = fla->first_block;
  freeblock_t* new_block = (freeblock_t*)backup_header.start;
  new_block->size = (njsp)(header->size + (p - header->start));
  new_block->next = NULL;
  add_and_merge_free_block(fla, new_block);
  freeblock_t* fit_block;
  freeblock_t* prior_fit_block;
  nju8* returned_pointer = find_best_fit_free_block(
      fla, size, backup_header.alignment, &fit_block, &prior_fit_block);
  if (returned_pointer) {
    fla->used_size -= backup_header.size + (p - backup_header.start);
    memmove(returned_pointer, p, size);
    njsp padding_and_header = returned_pointer - (nju8*)fit_block;
    bool rv = shrink_free_block(
        fla, fit_block, prior_fit_block, padding_and_header + size);
    header = get_allocation_header(returned_pointer);
    header->start = (nju8*)fit_block;
    header->size = rv ? size : fit_block->size - padding_and_header;
    header->alignment = backup_header.alignment;
#if NJ_IS_DEV()
    header->p = returned_pointer;
#endif
    return returned_pointer;
  }
  // Restores |header|
  if (prior_block)
    *prior_block = backup_prior_block;
  if (next_block)
    *next_block = backup_next_block;
  fla->first_block = backup_first_block;
  *header = backup_header;
  NJ_LOGD("Free list allocator \"%s\" doesn't have enough space to alloc %d "
          "bytes",
          fla->name,
          size);
  return NULL;
}

bool nj_free_list_allocator_t::init() {
  used_size = 0;
  start = (nju8*)malloc(total_size);
  NJ_CHECKF_RETURN_VAL(start, false, "Can't init allocator \"%s\": Out of memory");
  first_block = (freeblock_t*)start;
  first_block->size = total_size;
  first_block->next = NULL;
  return true;
}

void nj_free_list_allocator_t::destroy() {
  if (start)
    ::free(start);
}

void* nj_free_list_allocator_t::aligned_alloc(njsp size, njsp alignment) {
  NJ_CHECKF_RETURN_VAL(check_aligned_alloc(size, alignment), NULL, "Alignment is not power of 2");
  freeblock_t* fit_block;
  freeblock_t* prior_block;
  nju8* p = find_best_fit_free_block(this, size, alignment, &fit_block, &prior_block);
  NJ_CHECKF_RETURN_VAL(p, NULL, "Free list allocator \"%s\" doesn't have enough space to alloc %d bytes", name, size);
  njsp padding_and_header = p - (nju8*)fit_block;
  bool rv = shrink_free_block(this, fit_block, prior_block, padding_and_header + size);
  allocation_header_t* hdr = get_allocation_header(p);
  hdr->start = (nju8*)fit_block;
  hdr->size = rv ? size : fit_block->size - padding_and_header;
  hdr->alignment = alignment;
#if NJ_IS_DEV()
  hdr->p = p;
#endif
  return p;
}

void* nj_free_list_allocator_t::realloc(void* p, njsp size) {
  NJ_CHECKF_RETURN_VAL(check_p_in_dev(p) && size, NULL, "Invalid pointer to realloc");

  allocation_header_t* header = get_allocation_header(p);
  // Remaining free space is surely not enough.
  if (size > header->size + (total_size - used_size))
    return NULL;

  if (size == header->size) {
    return p;
  }

  freeblock_t* prior_block;
  freeblock_t* next_block;
  get_adjacent_blocks(this, (nju8*)p, &prior_block, &next_block);
  // smaller size
  if (size < header->size) {
    realloc_smaller(this, (nju8*)p, size, prior_block, next_block);
    return p;
  }

  return realloc_bigger(this, (nju8*)p, size, prior_block, next_block);
}

void nj_free_list_allocator_t::free(void* p) {
  NJ_CHECKF_RETURN(check_p_in_dev(p), "Invalid pointer to free");
  allocation_header_t* header = get_allocation_header(p);
  njsp freed_size = header->size + ((nju8*)p - header->start);
  used_size -= freed_size;
  freeblock_t* new_block = (freeblock_t*)header->start;
  new_block->size = freed_size;
  new_block->size = NULL;
  add_and_merge_free_block(this, new_block);
}
