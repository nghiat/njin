//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#ifndef NJ_CORE_DYNAMIC_LIB_H
#define NJ_CORE_DYNAMIC_LIB_H

typedef void* nj_dynamic_lib_t;

bool nj_dl_open(nj_dynamic_lib_t* dl, const char* name);
void nj_dl_close(nj_dynamic_lib_t* dl);
void* nj_dl_get_proc(nj_dynamic_lib_t* dl, const char* name);

#endif // NJ_CORE_DYNAMIC_LIB_H
