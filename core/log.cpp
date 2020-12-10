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
static bool g_log_inited = false;

void nj_log_internal(enum nj_log_level level, const char* file, int line, const char* format, ...) {
  if (!g_log_inited)
    return;
  nj_scoped_la_allocator_t<> temp_allocator("log_temp_allocator");
  temp_allocator.init();

  // FILE(LINE) for visual studio click to go to location.
  int log_len = 0;
  const char* log_prefix_format = "%s(%d): %s ";
  const char* level_str = gc_log_level_strings[(int)level];
  int prefix_len = snprintf(NULL, 0, log_prefix_format, file, line, level_str);
  char* log_buffer = (char*)temp_allocator.alloc(log_len + 1);
  snprintf(log_buffer, prefix_len + 1, log_prefix_format, file, line, level_str);
  log_len += prefix_len;

  va_list argptr, argptr2;
  va_start(argptr, format);
  va_copy(argptr2, argptr);
  // +1 for new line char.
  int msg_len = vsnprintf(NULL, 0, format, argptr) + 1;
  va_end(argptr);
  temp_allocator.realloc(log_buffer, log_len + msg_len + 1);
  va_start(argptr2, format);
  vsnprintf(log_buffer + log_len, msg_len + 1, format, argptr2);
  va_end(argptr2);
  log_len += msg_len;
  log_buffer[log_len - 1] = '\n';
  log_buffer[log_len] = 0;

  if (level == NJ_LOG_LEVEL_FATAL && !nj_debug_is_debugger_attached()) {
    const char* trace_format = "StackTraces:\n%s";
    char trace[NJ_MAX_STACK_TRACE_LENGTH];
    nj_debug_get_stack_trace(trace, NJ_MAX_STACK_TRACE_LENGTH);
    int trace_len = snprintf(NULL, 0, trace_format, trace);
    temp_allocator.realloc(log_buffer, log_len + trace_len + 1);
    snprintf(log_buffer + log_len, trace_len + 1, trace_format, trace);
    log_len += trace_len;
  }
  // Log to stream
  FILE* stream;
  if (level == NJ_LOG_LEVEL_INFO || level == NJ_LOG_LEVEL_DEBUG)
    stream = stdout;
  else
    stream = stderr;
  fprintf(stream, "%s", log_buffer);
#if NJ_OS_WIN()
  OutputDebugStringA(log_buffer);
#endif
  nj_file_write(&g_log_file, log_buffer, log_len, NULL);
}

bool nj_log_init(const nj_os_char* log_path) {
  nj_file_open(&g_log_file, log_path, NJ_FILE_MODE_APPEND);
  g_log_inited = nj_file_is_valid(&g_log_file);
  return g_log_inited;
}

void nj_log_destroy() {
  if (g_log_inited)
    nj_file_close(&g_log_file);
  g_log_inited = false;
}
