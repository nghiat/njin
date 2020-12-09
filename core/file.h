//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#ifndef NJ_CORE_FILE_H
#define NJ_CORE_FILE_H

#include "core/njtype.h"
#include "core/os.h"
#include "core/os_string.h"

#if NJ_OS_WIN()
#  include "core/windows_lite.h"
#endif

#define NJ_FILE_INVALID_POS (-1)
#define NJ_FILE_INVALID_SIZE (-1)

enum nj_file_mode {
  // open file if it exists, otherwise, create a new file.
  NJ_FILE_MODE_READ = 1 << 0,
  NJ_FILE_MODE_WRITE = 1 << 1,
  NJ_FILE_MODE_APPEND = 1 << 2,
};

enum nj_file_from {
  NJ_FILE_FROM_BEGIN,
  NJ_FILE_FROM_CURRENT,
  NJ_FILE_FROM_END
};

struct file_buffer;

struct nj_file_t {
#if NJ_OS_WIN()
  HANDLE handle;
#elif NJ_OS_LINUX()
  int handle;
#else
#error "?"
#endif
  const nj_os_char* path;
  file_buffer* internal_buffer = NULL;
};

bool nj_file_init();

bool nj_file_open(nj_file_t* file, const nj_os_char* path, enum nj_file_mode mode);
void nj_file_close(nj_file_t* file);

void nj_file_delete(nj_file_t* file);
void nj_file_delete_path(const nj_os_char* path);

bool nj_file_read(nj_file_t* file, void* buffer, njsp size, njsp* bytes_read);
bool nj_file_read_line(nj_file_t* file, char* buffer, njsp size);
bool nj_file_write(nj_file_t* file, const void* buffer, njsp size, njsp* bytes_written);
void nj_file_seek(nj_file_t* file, enum nj_file_from from, njsp distance);
void nj_file_flush(nj_file_t* file);

bool nj_file_is_valid(const nj_file_t* file);

njsp nj_file_get_size(const nj_file_t* file);

#endif // NJ_CORE_FILE_H
