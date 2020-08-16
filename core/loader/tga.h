//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#ifndef NJ_CORE_LOADER_TGA_H
#define NJ_CORE_LOADER_TGA_H

#include "core/os_string.h"

bool nj_tga_write(const nju8* data, int width, int height, const nj_os_char* path);

#endif // NJ_CORE_LOADER_TGA_H
