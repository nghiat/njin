//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#include "core/mono_time.h"

#include <Windows.h>

static LARGE_INTEGER g_performance_freq;

bool nj_mono_time_init() {
  return QueryPerformanceFrequency(&g_performance_freq);
}

njs64 nj_s_to_mono_time(njf64 s) {
  return s * g_performance_freq.QuadPart;
}

njs64 nj_mono_time_now() {
  LARGE_INTEGER pc;
  QueryPerformanceCounter(&pc);
  return pc.QuadPart;
}

njf64 nj_mono_time_to_s(njs64 t) {
  return (njf64)t / g_performance_freq.QuadPart;
}

njf64 nj_mono_time_to_ms(njs64 t) {
  return nj_mono_time_to_s(t) * 1000;
}

njf64 nj_mono_time_to_us(njs64 t) {
  return nj_mono_time_to_s(t) * 1000000;
}
