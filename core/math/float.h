//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#ifndef NJ_CORE_MATH_FLOAT_H
#define NJ_CORE_MATH_FLOAT_H

#define NJ_EPSILON_F 0.0000001f
#define NJ_PI_F 3.14159265358979323846 

bool nj_float_equal(float a, float b);
bool nj_float_equal_0(float a);
float nj_degree_to_rad(float deg);

#endif // NJ_CORE_MATH_FLOAT_H
