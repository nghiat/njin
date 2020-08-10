//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#ifndef NJ_CORE_MONO_TIME_H
#define NJ_CORE_MONO_TIME_H

#include "core/njtype.h"

bool nj_mono_time_init();
njs64 nj_mono_time_now();
njs64 nj_s_to_mono_time(njf64 s);
njf64 nj_mono_time_to_s(njs64 t);
njf64 nj_mono_time_to_ms(njs64 t);
njf64 nj_mono_time_to_us(njs64 t);

#endif // NJ_CORE_MONO_TIME_H
