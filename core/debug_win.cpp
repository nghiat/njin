//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2019             //
//----------------------------------------------------------------------------//

#include "core/debug.h"

#include "core/log.h"

#include <Windows.h>
#include <DbgHelp.h>
#include <stdio.h>

bool nj_debug_init() {
  SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_UNDNAME | SYMOPT_LOAD_LINES);
  SymInitialize(GetCurrentProcess(), NULL, TRUE);
  return true;
}

void nj_debug_get_stack_trace(char* buffer, int len) {
  // TODO: mutex
  // std::lock_guard<std::mutex> lk(g_mutex);
  memset(buffer, 0, len);
  NJ_CHECKF(len <= NJ_MAX_STACK_TRACE_LENGTH, "len is too long");
  void* frames[NJ_MAX_TRACES];
  int count = CaptureStackBackTrace(0, NJ_MAX_TRACES, frames, NULL);
  int remaining_size = len;
  for (int i = 0; i < count; ++i) {
    DWORD64 displacement = 0;
    DWORD_PTR address = (DWORD_PTR)frames[i];
    char symbol_buffer[sizeof(SYMBOL_INFO) + NJ_MAX_SYMBOL_LENGTH * sizeof(TCHAR)];
    char traceBuffer[NJ_MAX_SYMBOL_LENGTH];
    memset(symbol_buffer, 0, sizeof(symbol_buffer));
    memset(traceBuffer, 0, sizeof(traceBuffer));

    PSYMBOL_INFO symbol_info = (PSYMBOL_INFO)symbol_buffer;

    symbol_info->SizeOfStruct = sizeof(SYMBOL_INFO);
    symbol_info->MaxNameLen = NJ_MAX_SYMBOL_LENGTH - 1;
    DWORD line_displacement = 0;
    IMAGEHLP_LINE line = {};
    line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
    HANDLE current_process = GetCurrentProcess();
    int trace_chars_written = 0;
    if (SymFromAddr(current_process, address, &displacement, symbol_info)) {
      trace_chars_written = snprintf(traceBuffer, NJ_MAX_SYMBOL_LENGTH, "%s ", symbol_info->Name);
    }
    if (SymGetLineFromAddr(current_process, address, &line_displacement, &line)) {
      snprintf(traceBuffer + trace_chars_written,
               NJ_MAX_SYMBOL_LENGTH - trace_chars_written,
               "%s:%lu",
               line.FileName,
               line.LineNumber);
    }
    const char* format;
    if (i)
      format = "\n\t%s";
    else
      format = "\t%s";
    int stack_trace_chars_written = snprintf(buffer, remaining_size, format, traceBuffer);
    buffer += stack_trace_chars_written;
    remaining_size -= stack_trace_chars_written;
  }
}

bool nj_debug_is_debugger_attached() {
  return IsDebuggerPresent();
}
