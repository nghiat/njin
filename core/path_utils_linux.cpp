//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#include "core/path_utils.h"

#include "core/allocator.h"

#include <linux/limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

bool nj_common_paths_init() {
  char exe_path[NJ_MAX_PATH];
  ssize_t len = readlink("/proc/self/exe", exe_path, NJ_MAX_PATH);
  memcpy(g_exe_path, exe_path, len + 1);
  for (ssize_t i = len - 1; i >= 0; --i) {
    if (exe_path[i] == '/') {
      memcpy(g_exe_dir, exe_path, i + 2);
      g_exe_dir[i + 1] = 0;
      break;
    }
  }
  return true;
}
