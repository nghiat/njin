//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#ifndef NJ_CORE_COMPILER_H
#define NJ_CORE_COMPILER_H

#if defined(__clang__)
#define _NJ_COMPILER_CLANG 1
#elif defined(__GNUC__) || defined(__GNUG__)
#define _NJ_COMPILER_GCC 1
#elif defined(_MSC_VER)
#define _NJ_COMPILER_MSVC 1
#endif

#define NJ_IS_CLANG() _NJ_COMPILER_CLANG

#endif // NJ_CORE_BUILD_H
