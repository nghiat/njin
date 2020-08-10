//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#include "core/core_init.h"

#include "core/core_allocators.h"
#include "core/debug.h"
#include "core/log.h"
#include "core/mono_time.h"
#include "core/path_utils.h"

bool nj_core_init(const nj_os_char* log_path) {
  bool rv = true;
  rv &= nj_mono_time_init();
  rv &= nj_core_allocators_init();
  rv &= nj_path_utils_init();
  nj_os_char abs_log_path[NJ_MAX_PATH];
  nj_path_from_exe_dir(log_path, abs_log_path, NJ_MAX_PATH);
  rv &= nj_log_init(abs_log_path);
  rv &= nj_debug_init();
  return rv;
}
