//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#include "core/thread.h"

#include "core/log.h"

#include <pthread.h>
#include <unistd.h>

static void* platform_thread_start(void* args) {
  nj_thread_t* thread = (nj_thread_t*)args;
  thread->start_func(thread->args);
  return NULL;
}

bool nj_thread_init(nj_thread_t* thread, nj_thread_func_t start_func, void* args) {
  thread->start_func = start_func;
  thread->args = args;
  NJ_CHECK_LOG_RETURN_VAL(pthread_create(&thread->handle, NULL, platform_thread_start, (void*)thread) == 0, false, "Can't create a new thread");
  return true;
}

void nj_thread_wait_for(nj_thread_t* thread) {
  void* res;
  pthread_join(thread->handle, &res);
}

int nj_thread_get_nums() {
  return sysconf(_SC_NPROCESSORS_ONLN);
}
