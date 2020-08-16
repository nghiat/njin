//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#include "core/loader/dae.h"

#include "core/file_utils.h"
#include "core/linear_allocator.h"
#include "core/log.h"

#include <ctype.h>
#include <stdlib.h>

static char* alloc_string(nj_allocator_t* allocator, const char* start, const char* end) {
  int len = end - start;
  char* str = (char*)allocator->alloc(end - start + 1);
  memcpy(str, start, len);
  str[len] = 0;
  return str;
}

static nj_xml_node_t* parse_xml(nj_allocator_t* allocator, const char* start, const char* end, const char** last_pos) {
  const char* p = start;
  nj_xml_node_t* node = (nj_xml_node_t*)allocator->alloc(sizeof(nj_xml_node_t));
  node->text = NULL;
  while (p != end) {
    while (p != end && *p != '<') ++p;
    if (*p == '<') {
      const char* tag_start = p + 1;
      if (*tag_start == '?') {
        p = (const char*)memchr(p, '>', end - p);
        NJ_CHECKF_RETURN_VAL(p, NULL, "Can't find closing bracket for metadata tag");
        continue;
      }
      while (*(++p) != '>') {}
      const char* tag_end = p;
      ++p;
      // Check self-closing tag
      const char* tag_p = tag_end - 1;
      while(tag_p > tag_start && isspace(*tag_p)) --tag_p;
      if (*tag_p == '/') {
        tag_end = tag_p;
      }
      else {
        nj_da_init(&node->children, allocator);
      }

      // Parse tag_name
      tag_p = tag_start;
      while(tag_p != tag_end && isspace(*tag_p)) ++tag_p;
      const char* tag_name_start = tag_p;
      while (tag_p != tag_end && !isspace(*tag_p)) ++tag_p;
      const char* tag_name_end = tag_p;
      node->tag_name = alloc_string(allocator, tag_name_start, tag_name_end);

      // Parse attribute
      while (tag_p != tag_end) {
        // attribute name
        while(tag_p != tag_end && isspace(*tag_p)) ++tag_p;
        if (tag_p == tag_end) break;

        if (nj_da_len(&node->attr_names) == 0) {
          nj_da_init(&node->attr_names, allocator);
          nj_da_init(&node->attr_vals, allocator);
        }
        const char* a_name_start = tag_p;
        while(tag_p != tag_end && !isspace(*tag_p) && *tag_p != '=') ++tag_p;
        const char* a_name_end = tag_p;
        char* a_name = alloc_string(allocator, a_name_start, a_name_end);
        nj_da_append(&node->attr_names, a_name);
        ++tag_p;

        // attribute val
        while(tag_p != tag_end && *tag_p != '"') ++tag_p;
        const char* a_val_start = ++tag_p;
        while(tag_p != tag_end && *tag_p != '"') ++tag_p;
        const char* a_val_end = tag_p;
        char* a_val = alloc_string(allocator, a_val_start, a_val_end);
        nj_da_append(&node->attr_vals, a_val);
        ++tag_p;
      }

      if (nj_da_len(&node->children) == 0) {
        if (last_pos)
          *last_pos = p;
        return node;
      }

      // Parse content
      while (p != end && isspace(*p))
        ++p;

      // It's the text.
      if (*p != '<') {
        const char* text_start = p;
        const char* text_end = (const char*)memchr(p, '<', end - text_start);
        NJ_CHECKF_RETURN_VAL(text_end, NULL, "Can't find closing tag");
        node->text = alloc_string(allocator, text_start, text_end);
        p = text_end + 1;

        const char* closing_name_start = ++p;
        const char* closing_name_end = (const char*)memchr(p, '>', end - p);
        NJ_CHECKF_RETURN_VAL(closing_name_end, NULL, "Can't find closing bracket of closing tag name");
        NJ_CHECKF_RETURN_VAL(closing_name_end - closing_name_start && !memcmp(closing_name_start, node->tag_name, closing_name_end - closing_name_start), NULL, "Unmatched closing tag name");
        if (last_pos)
          *last_pos = closing_name_end;
        return node;
      }

      // It's the children
      while (p != end) {
        while (p != end && isspace(*p))
          ++p;
        const char* opening_bracket = p;
        NJ_CHECKF_RETURN_VAL(*p == '<', NULL, "This codepath processes children or closing tag which have to start with <");
        ++p;
        // It's closing tag.
        if (*p == '/') {
          const char* closing_name_start = ++p;
          const char* closing_name_end = (const char*)memchr(p, '>', end - p);
          NJ_CHECKF_RETURN_VAL(closing_name_end, NULL, "Can't find closing bracket of closing tag name");
          NJ_CHECKF_RETURN_VAL(closing_name_end - closing_name_start && !memcmp(closing_name_start, node->tag_name, closing_name_end - closing_name_start), NULL, "Unmatched closing tag name");
          if (last_pos)
            *last_pos = closing_name_end;
          return node;
        }

        nj_da_append(&node->children, parse_xml(allocator, opening_bracket, end, &p));
        ++p;
      }
    }
  }
  return node;
}

nj_xml_node_t* dae_find_node(nj_xml_node_t* node, const char* name) {
  nj_xml_node_t* curr = node;
  while (1) {
    int name_len = strlen(name);
    const char* slash = (const char*)memchr(name, '/', name_len);
    int sub_elem_len = slash ? slash - name : strlen(name);
    for (int i = 0; i < nj_da_len(&curr->children); ++i) {
      nj_xml_node_t* child = curr->children[i];
      if (sub_elem_len == strlen(child->tag_name) && !memcmp(name, child->tag_name, sub_elem_len)) {
        curr = child;
        break;
      }
    }
    // Can't find sub element.
    if (!node)
      return NULL;
    // Final subelement.
    if (!slash)
      break;
    name = slash + 1;
  };
  return curr;
}

bool nj_dae_init(nj_dae_t* dae, nj_allocator_t* allocator, const nj_os_char* path) {
  nj_linear_allocator_t<> file_allocator("xml_file_allocator");
  file_allocator.init();
  nj_dynamic_array_t<nju8> buffer = nj_read_whole_file(&file_allocator, path, NULL);
  nj_xml_node_t* root = parse_xml(allocator, (char*)buffer.p, (char*)buffer.p + nj_da_len(&buffer), NULL);
  file_allocator.destroy();

  nj_xml_node_t* mesh_position = dae_find_node(root, "library_geometries/geometry/mesh/source/float_array");
  int arr_len = atoi(mesh_position->attr_vals[1]);
  NJ_CHECKF_RETURN_VAL(arr_len % 3 == 0, false, "Invalid vertex number");
  nj_da_init(&dae->vertices, allocator);
  nj_da_reserve(&dae->vertices, arr_len / 3);
  char* arr_p = mesh_position->text;
  while (*arr_p) {
    float x = strtof(arr_p, &arr_p);
    float y = strtof(arr_p, &arr_p);
    float z = strtof(arr_p, &arr_p);
    nj_da_append(&dae->vertices, {x, y, z, 1.0f});
  }
  NJ_CHECKF_RETURN_VAL(arr_len / 3 == nj_da_len(&dae->vertices), false, "Unmatched vertex number between attribute and text");
  return true;
}

void nj_dae_destroy(nj_dae_t* obj) {
}
