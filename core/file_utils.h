//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#ifndef NJ_CORE_FILE_UTILS_H
#define NJ_CORE_FILE_UTILS_H

#include "core/os_string.h"

struct nj_allocator_t;

nju8* ez_read_whole_file(nj_allocator_t* allocator, const nj_os_char* path, njsp* read_bytes);

#endif // NJ_CORE_FILE_UTILS_H
