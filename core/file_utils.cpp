//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#include "core/file_utils.h"

#include "core/allocator.h"
#include "core/dynamic_array.inl"
#include "core/file.h"
#include "core/log.h"

nj_dynamic_array_t<nju8> nj_read_whole_file(nj_allocator_t* allocator, const nj_os_char* path, njsp* read_bytes) {
  nj_dynamic_array_t<nju8> buffer;
  nj_file_t f;
  nj_file_open(&f, path, NJ_FILE_MODE_READ);
  NJ_CHECKF_RETURN_VAL(nj_file_is_valid(&f), buffer, "Invalid file");
  njsp file_size = nj_file_get_size(&f);
  nj_da_init(&buffer, allocator);
  nj_da_resize(&buffer, file_size);
  nj_file_read(&f, &buffer[0], file_size, NULL);
  buffer[file_size] = 0;
  nj_file_close(&f);
  if (read_bytes)
    *read_bytes = file_size;
  return buffer;
}
