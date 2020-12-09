//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#include "core/file.h"

struct file_buffer {
  nju8* buffer = NULL;
  njsz len = 0;
  // offset from the start of the buffer.
  njsz offset = 0;
  bool is_in_used : 1;
  bool is_writing : 1;
};

bool nj_file_open_plat(nj_file_t* file, const nj_os_char* path, enum nj_file_mode mode);
void nj_file_close_plat(nj_file_t* file);

bool nj_file_read_plat(nj_file_t* file, void* buffer, njsp size, njsp* bytes_read);
bool nj_file_write_plat(nj_file_t* file, const void* buffer, njsp size, njsp* bytes_written);
void nj_file_seek_plat(nj_file_t* file, enum nj_file_from from, njsp distance);
