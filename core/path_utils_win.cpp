//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#include "core/path_utils.h"

#include <Windows.h>

bool nj_path_utils_init() {
  wchar_t exe_path[NJ_MAX_PATH];
  DWORD len = GetModuleFileName(NULL, exe_path, NJ_MAX_PATH);
  memcpy(g_exe_path, exe_path, (len + 1) * sizeof(wchar_t));
  for (DWORD i = len - 1; i >= 0; --i) {
    if (exe_path[i] == L'\\') {
      memcpy(g_exe_dir, exe_path, (i + 2) * sizeof(wchar_t));
      g_exe_dir[i + 1] = 0;
      break;
    }
  }
  return true;
}
