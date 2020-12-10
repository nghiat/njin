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
  return la->m_current_page->size - (la->m_top - (nju8*)(la->m_current_page));
}

template <njsz INITIAL_SIZE>
bool nj_linear_allocator_t<INITIAL_SIZE>::init() {
  m_used_size += sizeof(la_page_t);
  m_default_page_size = NJ_LINEAR_ALLOCATOR_DEFAULT_PAGE_SIZE;
  m_current_page = (la_page_t*)&(m_stack_page[0]);
  m_current_page->size = INITIAL_SIZE;
  m_current_page->prev = NULL;
  m_top = (nju8*)(m_current_page + 1);
  return true;
}

template <njsz INITIAL_SIZE>
void nj_linear_allocator_t<INITIAL_SIZE>::destroy() {
  la_page_t* page = m_current_page;
  while (page != (la_page_t*)&(m_stack_page[0])) {
    la_page_t* prev = page->prev;
    ::free(page);
    page = prev;
  }
}

template <njsz INITIAL_SIZE>
void* nj_linear_allocator_t<INITIAL_SIZE>::aligned_alloc(njsp size, njsp alignment) {
  NJ_CHECK_LOG_RETURN_VAL(check_aligned_alloc(size, alignment), NULL, "Alignment is not power of 2");

  nju8* p = m_top + sizeof(allocation_header_t);
  p = align_forward(p, alignment);
  njsp real_size = (p - m_top) + size;
  if (get_current_page_remaning_size(this) < real_size) {
    njsp new_page_size = sizeof(la_page_t) + sizeof(allocation_header_t) + size + alignment;
    if (new_page_size < m_default_page_size)
      new_page_size = m_default_page_size;
    la_page_t* new_page = (la_page_t*)malloc(new_page_size);
    NJ_CHECK_LOG_RETURN_VAL(new_page, NULL, "Out of memory for new page for linear allocator \"%s\"", m_name);
    new_page->size = new_page_size;
    new_page->prev = m_current_page;
    size += new_page_size;
    m_used_size += get_current_page_remaning_size(this);
    m_current_page = new_page;
    m_top = (nju8*)(m_current_page + 1);
    p = align_forward(m_top + sizeof(allocation_header_t), alignment);
    real_size = (p - m_top) + size;
  }
  allocation_header_t* hdr = get_allocation_header(p);
  hdr->start = m_top;
  hdr->size = size;
  hdr->alignment = alignment;
#if NJ_IS_DEV()
  hdr->p = p;
#endif
  m_top += real_size;
  m_used_size += real_size;
  return p;
}

template <njsz INITIAL_SIZE>
void* nj_linear_allocator_t<INITIAL_SIZE>::realloc(void* p, njsp size) {
  NJ_CHECK_LOG_RETURN_VAL(check_p_in_dev(p) && size, NULL, "Invalid pointer to realloc");

  allocation_header_t* header = get_allocation_header(p);
  njsp old_size = header->size;
  // Not at top
  if ((nju8*)p + header->size != m_top) {
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
  m_used_size = m_used_size + size - old_size;
  header->size = size;
  m_top = (nju8*)p + size;
  return p;
}

template <njsz INITIAL_SIZE>
void nj_linear_allocator_t<INITIAL_SIZE>::free(void* p) {
  NJ_CHECK_LOG_RETURN(check_p_in_dev(p), "Invalid pointer to free");
  allocation_header_t* header = get_allocation_header(p);
  if ((nju8*)p + header->size != m_top) {
    return;
  }
  m_top = header->start;
  m_used_size -= header->size + ((nju8*)p - header->start);
}
