//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#ifndef NJ_CORE_THREAD_H
#define NJ_CORE_THREAD_H

#include "core/os.h"

#if NJ_OS_WIN()
#include "core/windows_lite.h"
typedef HANDLE nj_thread_handle_t;

#elif NJ_OS_LINUX()
#include <pthread.h>
typedef pthread_t nj_thread_handle_t;

#else
#error "?"
#endif

typedef void (*nj_thread_func_t)(void*);

struct nj_thread_t {
  nj_thread_handle_t handle;
  nj_thread_func_t start_func;
  void* args;
};

bool nj_thread_init(nj_thread_t* thread, nj_thread_func_t start_func, void* args);
void nj_thread_wait_for(nj_thread_t thread);
int nj_thread_get_nums();

#endif // NJ_CORE_THREAD_H
