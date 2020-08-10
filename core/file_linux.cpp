//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#include "core/file.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

bool nj_file_open(nj_file_t* file, const char* path, enum nj_file_mode mode) {
  NJ_CHECKF_RETURN_VAL(file, false, "Invalid file");
  NJ_CHECKF_RETURN_VAL(path, false, "Invalid path");

  file->path = path;
  int flags = 0;
  if (mode & NJ_FILE_MODE_READ)
    flags |= O_RDONLY;
  if (mode & NJ_FILE_MODE_WRITE)
    flags |= O_RDWR | O_CREAT | O_TRUNC;
  if (mode & NJ_FILE_MODE_APPEND)
    flags |= O_APPEND | O_RDWR;
  int modes = S_IRWXU;
  file->handle = open(file->path, flags, modes);
  return nj_file_is_valid(file);
}

void nj_file_close(nj_file_t* file) {
  NJ_CHECKF_RETURN(nj_file_is_valid(file), "Invalid file");
  if (!close(file->handle)) {
    file->handle = -1;
  }
}

void nj_file_delete(nj_file_t* file) {
  NJ_CHECKF_RETURN(nj_file_is_valid(file), "Invalid file");
  nj_file_delete_path(file->path);
}

void nj_file_delete_path(const char* path) {
  NJ_CHECKF_RETURN(path, "Invalid path");
  unlink(path);
}

bool nj_file_read(nj_file_t* file, void* buffer, njsz size, njsz* bytes_read) {
  NJ_CHECKF_RETURN(nj_file_is_valid(file), "Invalid file");
  njsz rv = read(file->handle, buffer, size);
  if (bytes_read)
    *bytes_read = rv;
  return rv;
}

void nj_file_write(nj_file_t* file, const void* buffer, njsz size) {
  NJ_CHECKF_RETURN(nj_file_is_valid(file), "Invalid file");
  write(file->handle, buffer, size);
}

void nj_file_seek(nj_file_t* file, enum nj_file_from from, njsz distance) {
  NJ_CHECKF_RETURN(nj_file_is_valid(file), "Invalid file");
  int whence;
  switch (from) {
  case NJ_FILE_FROM_BEGIN:
    whence = SEEK_SET;
    break;
  case NJ_FILE_FROM_CURRENT:
    whence = SEEK_CUR;
    break;
  case NJ_FILE_FROM_END:
    whence = SEEK_END;
    break;
  }
  lseek(file->handle, distance, whence);
}

njs64 nj_file_get_pos(const nj_file_t* file) {
  NJ_CHECKF_RETURN_VAL(nj_file_is_valid(file), NJ_FILE_INVALID_POS, "Invalid file");
  return lseek(file->handle, 0, SEEK_CUR);
}

bool nj_file_is_valid(const nj_file_t* file) {
  NJ_CHECKF_RETURN_VAL(file, false, "Invalid file");
  return file->handle != -1;
}

njsz nj_file_get_size(const nj_file_t* file) {
  NJ_CHECKF_RETURN_VAL(nj_file_is_valid(file), NJ_FILE_INVALID_SIZE, "Invalid file");
  struct stat st;
  stat(file->path, &st);
  return st.st_size;
}
