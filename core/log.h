//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#ifndef NJ_CORE_LOG_H
#define NJ_CORE_LOG_H

#include "core/build.h"
#include "core/debug.h"
#include "core/os_string.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

enum nj_log_level {
  NJ_LOG_LEVEL_INFO = 0,
  NJ_LOG_LEVEL_DEBUG = 1,
  NJ_LOG_LEVEL_WARNING = 2,
  NJ_LOG_LEVEL_FATAL = 3,
};

#if NJ_IS_DEV()
void nj_log_internal(enum nj_log_level level, const char* file, int line, const char* format, ...);
#else
#define nj_log_internal(..)
#endif

#define NJ_LOG_SIZE (1024 + NJ_MAX_STACK_TRACE_LENGTH)

bool nj_log_init(const nj_os_char* log_path);

// See log_level
#define NJ_LOGI(format, ...) nj_log_internal(NJ_LOG_LEVEL_INFO, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define NJ_LOGD(format, ...) nj_log_internal(NJ_LOG_LEVEL_DEBUG, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define NJ_LOGW(format, ...) nj_log_internal(NJ_LOG_LEVEL_WARNING, __FILE__, __LINE__, format, ##__VA_ARGS__)

#if NJ_IS_DEV()
#  define NJ_LOGF(format, ...)                                                      \
    nj_log_internal(NJ_LOG_LEVEL_FATAL, __FILE__, __LINE__, format, ##__VA_ARGS__); \
    if (nj_debug_is_debugger_attached()) {                                          \
      NJ_DEBUG_BREAK_DEBUGGER();                                                    \
    }
#else
#  define NJ_LOGF(format, ...) nj_log_internal(NJ_LOG_LEVEL_FATAL, __FILE__, __LINE__, format, ##__VA_ARGS__)
#endif

#define NJ_STRINGIFY_INTERNAL(condition) #condition
#define NJ_STRINGIFY(condition) NJ_STRINGIFY_INTERNAL(condition)

#define NJ_CHECKF(condition, format, ...) \
  if (!(condition))                       \
    NJ_LOGF(format, ##__VA_ARGS__);

#define NJ_LOGF_RETURN(format, ...) \
  NJ_LOGF(format, ##__VA_ARGS__);   \
  return;

#define NJ_LOGF_RETURN_VAL(retval, format, ...) \
  NJ_LOGF(format, ##__VA_ARGS__);               \
  return retval;

#define NJ_CHECKF_RETURN(condition, format, ...) \
  if (!(condition))                              \
    NJ_LOGF_RETURN(format, ##__VA_ARGS__);

#define NJ_CHECKF_RETURN_VAL(condition, retval, format, ...) \
  if (!(condition))                                          \
    NJ_LOGF_RETURN_VAL(retval, format, ##__VA_ARGS__);

#define NJ_UNUSED(a) (void)a

#endif // NJ_CORE_LOG_H
