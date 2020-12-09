//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#ifndef NJ_CORE_GFX_CAM_H
#define NJ_CORE_GFX_CAM_H

#include "core/math/mat4.h"
#include "core/math/vec3.h"
#include "core/window/input.h"

struct nj_window_t;

struct nj_cam_t {
  nj_window_t* w;
  nj_m4_t view_mat;
  nj_v3_t eye;
  nj_v3_t forward;
  nj_v3_t up;
  njf32 dist;
  bool is_mouse_down;
};

bool nj_cam_init(nj_cam_t* cam, const nj_v3_t& eye, const nj_v3_t& target, nj_window_t* w);
bool nj_cam_update(nj_cam_t* cam);
void nj_cam_mouse_move(nj_cam_t* cam, int x, int y);
void nj_cam_mouse_event(nj_cam_t* cam, enum nj_mouse mouse, int x, int y, bool is_down);

#endif // NJ_CORE_GFX_CAM_H
