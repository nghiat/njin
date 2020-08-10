//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#ifndef NJ_CORE_WINDOWS_LITE_H
#define NJ_CORE_WINDOWS_LITE_H

#include "core/os.h"

#if NJ_OS_WIN()

#  define NJ_FORWARD_DECLARE_HANDLE(name) \
    struct name##__;                      \
    typedef struct name##__* name

NJ_FORWARD_DECLARE_HANDLE(HDC);
NJ_FORWARD_DECLARE_HANDLE(HGLRC);
NJ_FORWARD_DECLARE_HANDLE(HINSTANCE);
NJ_FORWARD_DECLARE_HANDLE(HWND);

#  undef NJ_FORWARD_DECLARE_HANDLE

typedef void* HANDLE;
typedef HINSTANCE HMODULE;

#endif // NJ_OS_WIN()

#endif // NJ_CORE_WINDOWS_LITE_H
