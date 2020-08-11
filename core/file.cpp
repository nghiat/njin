//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#include "core/file.h"

#include <string.h>

bool nj_file_read_line(nj_file_t* file, char* buffer, njsp size) {
  njsp curr_pos = nj_file_get_pos(file);
  njsp read;
  if (!nj_file_read(file, buffer, size - 1, &read)) {
    buffer[read] = '\0';
    return false;
  }
  char* eol = (char*)memchr(buffer, '\n', read);
  if (!eol) {
    buffer[read] = '\0';
    return false;
  }
  njsp delta = eol - buffer;
  buffer[delta] = '\0';
  nj_file_seek(file, NJ_FILE_FROM_BEGIN, curr_pos + delta + 1);
  return true;
}
