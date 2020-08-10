//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#include "core/mono_time.h"

#include <time.h>

bool nj_mono_time_init() { return true; }

njs64 nj_mono_time_now() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000000000 + ts.tv_nsec;
}

njs64 nj_s_to_mono_time(njf64 s) {
  return s * 1000000000.0;
}

njf64 nj_mono_time_to_s(njs64 t) {
  return t / 1000000000.0;
}

njf64 nj_mono_time_to_ms(njs64 t) {
  return t / 1000000.0;
}

njf64 nj_mono_time_to_us(njs64 t) {
  return t / 1000.0;
}
