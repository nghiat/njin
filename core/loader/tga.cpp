//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#include "core/loader/tga.h"

#include "core/file.h"
#include "core/log.h"

struct color_map_spec_t {
  nju16 first_entry_index;
  nju16 color_map_length;
  nju8 color_map_entry_size;
} ;

struct image_spec_t {
  nju16 x_origin;
  nju16 y_origin;
  nju16 width;
  nju16 height;
  nju8 depth;
  nju8 descriptor;
};

bool nj_tga_write(const nju8* data, int width, int height, const nj_os_char* path) {
  nj_file_t f;
  NJ_CHECKF_RETURN_VAL(nj_file_open(&f, path, NJ_FILE_MODE_WRITE), false, "Can't open " NJ_OS_PCT " to write tga",  path);
  {
    uint8_t id_length = 0;
    nj_file_write(&f, &id_length, sizeof(id_length), NULL);
  }
  {
    uint8_t color_map_type = 0;
    nj_file_write(&f, &color_map_type, sizeof(color_map_type), NULL);
  }
  {
    uint8_t image_type = 2;
    nj_file_write(&f, &image_type, sizeof(image_type), NULL);
  }
  {
    color_map_spec_t spec = {};
    nj_file_write(&f, &spec, sizeof(spec), NULL);
  }
  {
    image_spec_t spec; 
    spec.x_origin = 0;
    spec.y_origin = 0;
    spec.width = width;
    spec.height = height;
    spec.depth = 24;
    spec.descriptor = 0;
    nj_file_write(&f, &spec, sizeof(spec), NULL);
  }
  nj_file_write(&f, data, width * height * 3, NULL);
  nj_file_close(&f);
  return true;
}
