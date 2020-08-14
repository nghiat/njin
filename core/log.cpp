//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#include "core/log.h"

#include "core/file.h"
#include "core/linear_allocator.inl"

#include <stdarg.h>
#include <stdlib.h>

#if NJ_OS_WIN()
#include <Windows.h>
#endif

static const char* gc_log_level_strings[] = {
    "INFO",
    "DEBUG",
    "WARNING",
    "FATAL",
};

static nj_file_t g_log_file;

void nj_log_internal(enum nj_log_level level, const char* file, int line, const char* format, ...) {
  nj_linear_allocator_t<> allocator("log_temp_allocator");
  allocator.init();

  const char* filename = strrchr(file, '\\') ? strrchr(file, '\\') + 1 : file;
  filename = strrchr(filename, '/') ? strrchr(filename, '/') + 1 : filename;
  int char_num = snprintf(NULL, 0, "%s:%d %s", filename, line, format) + 1;
  char* new_format = (char*)allocator.alloc(char_num);
  snprintf(new_format, char_num, "%s:%d %s", filename, line, format);

  va_list argptr, argptr2;
  va_start(argptr, format);
  va_copy(argptr2, argptr);
  char_num = (vsnprintf(NULL, 0, new_format, argptr) + 1) * 2;
  char* log_buffer = (char*)allocator.alloc(char_num);
  va_start(argptr, format);
  char_num = vsnprintf(log_buffer, char_num, new_format, argptr);
  va_end(argptr);
  va_end(argptr2);

  if (level == NJ_LOG_LEVEL_FATAL) {
    format = "\nStackTraces:\n%s";
    char trace[NJ_MAX_STACK_TRACE_LENGTH];
    nj_debug_get_stack_trace(trace, NJ_MAX_STACK_TRACE_LENGTH);
    int additional_char_num = snprintf(NULL, 0, format, trace) * 2;
    allocator.realloc(log_buffer, char_num + additional_char_num);
    additional_char_num = snprintf(log_buffer + char_num, additional_char_num, format, trace);
    char_num += additional_char_num;
  }
  format = "%s: %s\n";
  char_num = snprintf(NULL, 0, format, gc_log_level_strings[(int)level], log_buffer) + 1;
  char* log_buffer_to_file = (char*)allocator.alloc(char_num);
  char_num = snprintf(log_buffer_to_file, char_num, format, gc_log_level_strings[(int)level], log_buffer);
  // Log to stream
  FILE* stream;
  if (level == NJ_LOG_LEVEL_INFO || level == NJ_LOG_LEVEL_DEBUG)
    stream = stdout;
  else
    stream = stderr;
  fprintf(stream, "%s", log_buffer_to_file);
#if NJ_OS_WIN()
  OutputDebugStringA(log_buffer_to_file);
#endif
  // Log to file
  nj_file_write(&g_log_file, log_buffer_to_file, char_num);

  allocator.destroy();
}

bool nj_log_init(const nj_os_char* log_path) {
  nj_file_open(&g_log_file, log_path, NJ_FILE_MODE_APPEND);
  return nj_file_is_valid(&g_log_file);
}
