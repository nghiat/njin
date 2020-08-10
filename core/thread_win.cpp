//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#include "core/thread.h"

#include "core/log.h"

#include <Windows.h>

static DWORD platform_thread_start(void* args) {
  nj_thread_t* thread = (nj_thread_t*)args;
  thread->start_func(thread->args);
  return 0;
}

bool nj_thread_init(nj_thread_t* thread, nj_thread_func_t start_func, void* args) {
  thread->start_func = start_func;
  thread->args = args;
  thread->handle = CreateThread(NULL, 0, platform_thread_start, (void*)thread, 0, NULL);
  NJ_CHECKF_RETURN_VAL(thread->handle != NULL, false, "Can't create a new thread");
  return true;
}

void nj_thread_wait_for(nj_thread_t* thread) {
  WaitForSingleObject(thread->handle, INFINITE);
}

int nj_thread_get_nums() {
  SYSTEM_INFO info;
  GetSystemInfo(&info);
  return info.dwNumberOfProcessors;
}
