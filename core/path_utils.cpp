//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#include "core/path_utils.h"

#include "core/allocator.h"
#include "core/log.h"

nj_os_char g_exe_path[NJ_MAX_PATH];
nj_os_char g_exe_dir[NJ_MAX_PATH];

nj_os_char* nj_path_from_exe_dir(const nj_os_char* sub_path, nj_os_char* out, int len) {
  int exe_dir_len = nj_str_get_len(g_exe_dir);
  int sub_path_len = nj_str_get_len(sub_path);
  NJ_CHECKF_RETURN_VAL(exe_dir_len + sub_path_len < len, NULL, "Path is too long");
  memcpy(out, g_exe_dir, exe_dir_len * sizeof(nj_os_char));
  memcpy(out + exe_dir_len, sub_path, sub_path_len * sizeof(nj_os_char));
  out[exe_dir_len + sub_path_len] = 0;
  return out;
}
