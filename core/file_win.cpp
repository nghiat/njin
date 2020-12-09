//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#include "core/file.h"

#include "core/log.h"
#include "core/utils.h"

#include <Windows.h>

bool nj_file_open_plat(nj_file_t* file, const wchar_t* path, enum nj_file_mode mode) {
  NJ_CHECKF_RETURN_VAL(file, false, "Invalid file");
  NJ_CHECKF_RETURN_VAL(path, false, "Invalid path");

  file->path = path;
  DWORD access = 0;
  if (mode & NJ_FILE_MODE_READ)
    access |= GENERIC_READ;
  if (mode & NJ_FILE_MODE_WRITE)
    access |= GENERIC_READ | GENERIC_WRITE;
  if (mode & NJ_FILE_MODE_APPEND)
    access |= GENERIC_READ | FILE_APPEND_DATA;

  DWORD share_mode = 0;
  share_mode |= FILE_SHARE_READ;

  DWORD create_disposition = 0;
  if (mode & NJ_FILE_MODE_READ)
    create_disposition = OPEN_EXISTING;
  if (mode & NJ_FILE_MODE_WRITE)
    create_disposition = CREATE_ALWAYS;
  if (mode & NJ_FILE_MODE_APPEND)
    create_disposition = OPEN_ALWAYS;
  file->handle = CreateFile(
      file->path, access, share_mode, NULL, create_disposition, 0, NULL);
  NJ_CHECKF_RETURN_VAL(nj_file_is_valid(file), false, "Can't open file %ls", path);
  return true;
}

void nj_file_close_plat(nj_file_t* file) {
  NJ_CHECKF_RETURN(nj_file_is_valid(file), "Invalid file");
  CloseHandle(file->handle);
  file->handle = INVALID_HANDLE_VALUE;
}

void nj_file_delete(nj_file_t* file) {
  NJ_CHECKF_RETURN(nj_file_is_valid(file), "Invalid file");
  nj_file_delete_path(file->path);
}

void nj_file_delete_path(const wchar_t* path) {
  NJ_CHECKF_RETURN(path, "Invalid path");
  DeleteFile(path);
}

bool nj_file_read_plat(nj_file_t* file, void* buffer, njsp size, njsp* bytes_read) {
  NJ_CHECKF_RETURN_VAL(nj_file_is_valid(file), false, "Invalid file");
  DWORD read = 0;
  ReadFile(file->handle, buffer, size, &read, NULL);
  if (bytes_read)
    *bytes_read = read;
  return read;
}

bool nj_file_write_plat(nj_file_t* file, const void* buffer, njsp size, njsp* bytes_written) {
  NJ_CHECKF_RETURN_VAL(nj_file_is_valid(file), false, "Invalid file");
  DWORD bytes_written_plat = 0;
  bool rv = WriteFile(file->handle, buffer, size, &bytes_written_plat, NULL);
  nj_maybe_assign(bytes_written, (njsp)bytes_written_plat);
  return rv;
}

void nj_file_seek_plat(nj_file_t* file, enum nj_file_from from, njsp distance) {
  NJ_CHECKF_RETURN(nj_file_is_valid(file), "Invalid file");
  DWORD move_method;
  switch (from) {
  case NJ_FILE_FROM_BEGIN:
    move_method = FILE_BEGIN;
    break;
  case NJ_FILE_FROM_CURRENT:
    move_method = FILE_CURRENT;
    break;
  case NJ_FILE_FROM_END:
    move_method = FILE_END;
    break;
  }
  SetFilePointer(file->handle, distance, NULL, move_method);
}

njsp nj_file_get_pos(const nj_file_t* file) {
  NJ_CHECKF_RETURN_VAL(nj_file_is_valid(file), NJ_FILE_INVALID_POS, "Invalid file");
  return SetFilePointer(file->handle, 0, NULL, FILE_CURRENT);
}

bool nj_file_is_valid(const nj_file_t* file) {
  NJ_CHECKF_RETURN_VAL(file, false, "Invalid file");
  return file->handle != INVALID_HANDLE_VALUE;
}

njsp nj_file_get_size(const nj_file_t* file) {
  NJ_CHECKF_RETURN_VAL(nj_file_is_valid(file), NJ_FILE_INVALID_SIZE, "Invalid file");
  return GetFileSize(file->handle, NULL);
}
