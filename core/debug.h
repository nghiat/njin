//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#ifndef NJ_CORE_DEBUG_H
#define NJ_CORE_DEBUG_H

#include "core/os.h"

#include <stdbool.h>

// Maximum number of traces.
#define NJ_MAX_TRACES 64

// Maximum char for each trace.
// Hopfully we don't use more than |gc_max_symbol_length| characters which means
// we have to use heap allocation.
#define NJ_MAX_SYMBOL_LENGTH 1024

// Maximum char for all traces.
#define NJ_MAX_STACK_TRACE_LENGTH (NJ_MAX_TRACES * NJ_MAX_SYMBOL_LENGTH)

#if NJ_OS_WIN()
#define NJ_DEBUG_BREAK_DEBUGGER() __debugbreak()
#elif NJ_OS_LINUX()
#define NJ_DEBUG_BREAK_DEBUGGER() __asm__("int $3")
#else
#error "?"
#endif

bool nj_debug_init();
void nj_debug_get_stack_trace(char* buffer, int len);
bool nj_debug_is_debugger_attached();

#endif // NJ_CORE_DEBUG_H
