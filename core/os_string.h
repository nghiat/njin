//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#ifndef NJ_CORE_OS_STRING_H
#define NJ_CORE_OS_STRING_H

#include "core/njtype.h"
#include "core/os.h"

#if NJ_OS_WIN()
#  define NJ_OS_LIT(x) L##x
#  define NJ_OS_PCT "%ls"
typedef wchar_t nj_os_char;

#elif NJ_OS_LINUX()
#  define NJ_OS_LIT(x) x
#  define NJ_OS_PCT "%s"
typedef char nj_os_char;

#else
#error "?"
#endif

const nj_os_char* nj_str_find_substr(const nj_os_char* str, const nj_os_char* substr);
njsz nj_str_get_len(const nj_os_char* str);
bool nj_str_compare(const nj_os_char* s1, const nj_os_char* s2);

#endif // NJ_CORE_OS_STRING_H
