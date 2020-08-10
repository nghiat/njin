//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#ifndef NJ_CORE_PATH_UTILS_H
#define NJ_CORE_PATH_UTILS_H

#include "core/os.h"
#include "core/os_string.h"

#define NJ_MAX_PATH (256)

extern nj_os_char g_exe_path[NJ_MAX_PATH];
extern nj_os_char g_exe_dir[NJ_MAX_PATH];

bool nj_path_utils_init();
void nj_path_from_exe_dir(const nj_os_char* sub_path, nj_os_char* out, int len);

#endif // NJ_CORE_PATH_UTILS_H
