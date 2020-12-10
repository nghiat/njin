//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#include "core/file.h"
#include "core/file_internal.h"

#include "core/allocator.h"
#include "core/core_allocators.h"
#include "core/log.h"
#include "core/utils.h"

#include <string.h>

static const int gc_max_buffer = 16;
static const int gc_max_buffer_size = 8 * 1024;

static file_buffer g_buffers[gc_max_buffer];


bool nj_file_init() {
  for (int i = 0; i < gc_max_buffer; ++i) {
    g_buffers[i].buffer = (nju8*)g_persistent_allocator->alloc(gc_max_buffer_size);
    g_buffers[i].is_in_used = false;
    if (!g_buffers[i].buffer)
      return false;
  }

  return true;
}

bool nj_file_open(nj_file_t* file, const nj_os_char* path, enum nj_file_mode mode) {
  bool rv = nj_file_open_plat(file, path, mode);
  if (!rv)
    return false;
  for (int i = 0; i < gc_max_buffer; ++i) {
    if (!g_buffers[i].is_in_used) {
      g_buffers[i].len = 0;
      g_buffers[i].offset = 0;
      g_buffers[i].is_in_used = true;
      g_buffers[i].is_writing = false;
      file->internal_buffer = &g_buffers[i];
      break;
    }
  }
  NJ_CHECK_LOG_RETURN_VAL(file->internal_buffer, false, "Can't get a file buffer, too many files are being opened");
  return true;
}

void nj_file_close(nj_file_t* file) {
  nj_file_flush(file);
  nj_file_close_plat(file);
  if (file->internal_buffer) {
    for (int i = 0; i < gc_max_buffer; ++i) {
      if (file->internal_buffer == &g_buffers[i]) {
        g_buffers[i].is_in_used = false;
        file->internal_buffer = NULL;
        break;
      }
    }
  }
  NJ_CHECK_RETURN(!file->internal_buffer);
}

bool nj_file_read(nj_file_t* file, void* out, njsp size, njsp* bytes_read) {
  NJ_CHECK_RETURN_VAL(size, false)
  file_buffer* fbuf = file->internal_buffer;
  NJ_CHECK_RETURN_VAL(fbuf, false)

  njsp bytes_left = fbuf->len - fbuf->offset;
  njsp total_bytes_read = 0;
  if (!fbuf->is_writing) {
    if (bytes_left >= size) {
      memcpy(out, fbuf->buffer + fbuf->offset, size);
      fbuf->offset += size;
      nj_maybe_assign(bytes_read, size);
      return true;
    } else if (bytes_left) {
      // Copy the rest of the file buffer.
      memcpy(out, fbuf->buffer + fbuf->offset, bytes_left);
      fbuf->offset = fbuf->len;
      size -= bytes_left;
      out = (nju8*)out + bytes_left;
      total_bytes_read = bytes_left;
    }
  }
  // Update the file buffer.
  njsp bytes_read_plat;
  if (size >= gc_max_buffer_size) {
    // Too big for a file buffer, just read straight into the out buffer.
    nj_file_read_plat(file, out, size, &bytes_read_plat);
    total_bytes_read += bytes_read_plat;
    nj_maybe_assign(bytes_read, total_bytes_read);
    return total_bytes_read != 0;
  }
  if (!nj_file_read_plat(file, fbuf->buffer, gc_max_buffer_size, &bytes_read_plat)) {
    nj_maybe_assign(bytes_read, total_bytes_read);
    return total_bytes_read != 0;
  }
  fbuf->len = bytes_read_plat;
  fbuf->is_writing = false;
  njsp copy_len = nj_min(bytes_read_plat, size);
  memcpy(out, fbuf->buffer, copy_len);
  fbuf->offset = copy_len;
  total_bytes_read += copy_len;
  nj_maybe_assign(bytes_read, total_bytes_read);
  return true;
}

bool nj_file_write(nj_file_t* file, const void* in, njsp size, njsp* bytes_written) {
  NJ_CHECK_RETURN_VAL(size, false);
  file_buffer* fbuf = file->internal_buffer;
  NJ_CHECK_RETURN_VAL(fbuf, false);

  njsp bytes_left = fbuf->len - fbuf->offset;
  njsp total_bytes_written = 0;
  if (fbuf->is_writing) {
    if (bytes_left >= size) {
      memcpy(fbuf->buffer + fbuf->offset, in, size);
      fbuf->offset += size;
      nj_maybe_assign(bytes_written, size);
      return true;
    } else if (bytes_left) {
      // Copy to the rest of the file buffer.
      memcpy(fbuf->buffer + fbuf->offset, in, bytes_left);
      fbuf->offset = fbuf->len;
      size -= bytes_left;
      in = (nju8*)in + bytes_left;
      njsp bytes_written_plat;
      if (!nj_file_write_plat(file, fbuf->buffer, fbuf->offset, &bytes_written_plat))
        return false;
      total_bytes_written = bytes_left;
    }
  }
  // Update the file buffer.
  if (size >= gc_max_buffer_size) {
    // Too big for a file buffer, just read straight into the out buffer.
    njsp bytes_written_plat;
    bool rv = nj_file_write_plat(file, in, size, &bytes_written_plat);
    total_bytes_written += bytes_written_plat;
    nj_maybe_assign(bytes_written, total_bytes_written);
    return rv;
  }
  fbuf->len = gc_max_buffer_size;
  fbuf->is_writing = true;
  memcpy(fbuf->buffer, in, size);
  fbuf->offset = size;
  nj_maybe_assign(bytes_written, total_bytes_written);
  return true;
}

void nj_file_seek(nj_file_t* file, enum nj_file_from from, njsp distance) {
  file_buffer* fbuf = file->internal_buffer;
  if (!fbuf) {
    nj_file_seek_plat(file, from, distance);
    return;
  }

  if (from != NJ_FILE_FROM_CURRENT) {
    if (fbuf->is_writing)
      nj_file_flush(file);
    nj_file_seek_plat(file, from, distance);
    return;
  }
  // from == NJ_FILE_FROM_CURRENT
  njsp bytes_left = fbuf->len - fbuf->offset;
  if (distance >= bytes_left) {
    if (fbuf->is_writing) {
      nj_file_flush(file);
      // We are seeking from fbuf->offset after flushing.
      nj_file_seek_plat(file, from, distance);
    } else {
      // We are seeking from the end of the file_buffer.
      nj_file_seek_plat(file, from, distance - bytes_left);
    }
    return;
  }

  // Seeking inside the file buffer.
  if (fbuf->is_writing) {
    memset(fbuf->buffer, 0, distance);
  }
  fbuf->offset += distance;
}

void nj_file_flush(nj_file_t* file) {
  file_buffer* fbuf = file->internal_buffer;
  if (fbuf) {
    if (fbuf->is_writing && fbuf->offset > 0) {
      NJ_CHECK_RETURN(nj_file_write_plat(file, fbuf->buffer, fbuf->offset, NULL));
      fbuf->is_writing = false;
      fbuf->offset = 0;
      fbuf->len = 0;
    }
  }
}
