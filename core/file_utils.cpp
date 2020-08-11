//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#include "core/file_utils.h"

#include "core/allocator.h"
#include "core/file.h"
#include "core/log.h"

nju8* nj_read_whole_file(nj_allocator_t* allocator, const nj_os_char* path, njsp* read_bytes) {
  nj_file_t f;
  nj_file_open(&f, path, NJ_FILE_MODE_READ);
  NJ_CHECKF_RETURN_VAL(nj_file_is_valid(&f), NULL, "Invalid file");
  njsp file_size = nj_file_get_size(&f);
  nju8* buffer = (nju8*)allocator->alloc(file_size);
  NJ_CHECKF_RETURN_VAL(buffer, NULL, "Can't allocate memory to read file");
  nj_file_read(&f, buffer, file_size, NULL);
  buffer[file_size] = '\0';
  nj_file_close(&f);
  if (read_bytes)
    *read_bytes = file_size;
  return buffer;
}
