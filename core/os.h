//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2010             //
//----------------------------------------------------------------------------//

#ifndef NJ_CORE_OS_H
#define NJ_CORE_OS_H

// OS macros
#if defined(_WIN32) || defined(_WIN64)
#  define NJ_OS_WIN_ 1
#endif

#if defined(__linux__)
#  define NJ_OS_LINUX_ 1
#endif

#define NJ_OS_WIN() NJ_OS_WIN_
#define NJ_OS_LINUX() NJ_OS_LINUX_

#endif // NJ_CORE_OS_H
