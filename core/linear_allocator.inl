//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#include "core/linear_allocator.h"

#include "core/allocator_internal.h"
#include "core/log.h"

#include <stdlib.h>
#include <string.h>

#define NJ_LINEAR_ALLOCATOR_DEFAULT_PAGE_SIZE 1024 * 1024

struct la_page_t {
  njsp size;
  la_page_t* prev;
};

template <njsz INITIAL_SIZE>
static njsp get_current_page_remaning_size(const nj_linear_allocator_t<INITIAL_SIZE>* la) {
  return la->current_page->size - (la->top - (nju8*)(la->current_page));
}

template <njsz INITIAL_SIZE>
bool nj_linear_allocator_t<INITIAL_SIZE>::init() {
  used_size += sizeof(la_page_t);
  default_page_size = NJ_LINEAR_ALLOCATOR_DEFAULT_PAGE_SIZE;
  current_page = (la_page_t*)&(stack_page[0]);
  current_page->size = INITIAL_SIZE;
  current_page->prev = NULL;
  top = (nju8*)(current_page + 1);
  return true;
}

template <njsz INITIAL_SIZE>
void nj_linear_allocator_t<INITIAL_SIZE>::destroy() {
  la_page_t* page = current_page;
  while (page != (la_page_t*)&(stack_page[0])) {
    la_page_t* prev = page->prev;
    free(page);
    page = prev;
  }
}

template <njsz INITIAL_SIZE>
void* nj_linear_allocator_t<INITIAL_SIZE>::aligned_alloc(njsp size, njsp alignment) {
  NJ_CHECKF_RETURN_VAL(check_aligned_alloc(size, alignment), NULL, "Alignment is not power of 2");

  nju8* p = top + sizeof(allocation_header_t);
  p = align_forward(p, alignment);
  njsp real_size = (p - top) + size;
  if (get_current_page_remaning_size(this) < real_size) {
    njsp new_page_size = sizeof(la_page_t) + sizeof(allocation_header_t) + size + alignment;
    if (new_page_size < default_page_size) new_page_size = default_page_size;
    la_page_t* new_page = (la_page_t*)malloc(new_page_size);
    NJ_CHECKF_RETURN_VAL(new_page, NULL, "Out of memory for new page for linear allocator \"%s\"", name);
    new_page->size = new_page_size;
    new_page->prev = current_page;
    size += new_page_size;
    used_size += get_current_page_remaning_size(this);
    current_page = new_page;
    top = (nju8*)(current_page + 1);
    p = align_forward(top + sizeof(allocation_header_t), alignment);
    real_size = (p - top) + size;
  }
  allocation_header_t* hdr = get_allocation_header(p);
  hdr->start = top;
  hdr->size = size;
  hdr->alignment = alignment;
#if NJ_IS_DEV()
  hdr->p = p;
#endif
  top += real_size;
  used_size += real_size;
  return p;
}

template <njsz INITIAL_SIZE>
void* nj_linear_allocator_t<INITIAL_SIZE>::realloc(void* p, njsp size) {
  NJ_CHECKF_RETURN_VAL(check_p_in_dev(p) && size, NULL, "Invalid pointer to realloc");

  allocation_header_t* header = get_allocation_header(p);
  njsp old_size = header->size;
  // Not at top
  if ((nju8*)p + header->size != top) {
    void* new_p = aligned_alloc(size, header->alignment);
    memcpy(new_p, p, header->size);
    return new_p;
  }
  // Remaining space is not enough.
  if (size > get_current_page_remaning_size(this)) {
    void* new_p = aligned_alloc(size, header->alignment);
    memcpy(new_p, p, header->size);
    return new_p;
  }
  if (size == old_size)
    return p;
  used_size = used_size + size - old_size;
  header->size = size;
  top = (nju8*)p + size;
  return p;
}

template <njsz INITIAL_SIZE>
void nj_linear_allocator_t<INITIAL_SIZE>::free(void* p) {
  NJ_CHECKF_RETURN(check_p_in_dev(p), "Invalid pointer to free");
  allocation_header_t* header = get_allocation_header(p);
  if ((nju8*)p + header->size != top) {
    return;
  }
  top = header->start;
  used_size -= header->size + ((nju8*)p - header->start);
}
