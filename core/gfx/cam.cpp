//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#include "core/gfx/cam.h"

#include "core/log.h"
#include "core/math/quat.inl"
#include "core/math/transform.inl"
#include "core/math/vec3.inl"
#include "core/window/window.h"

bool nj_cam_init(nj_cam_t* cam, const nj_v3_t& eye, const nj_v3_t& target, nj_window_t* w) {
  cam->w = w;
  cam->eye = eye;
  cam->forward = target - cam->eye;
  cam->dist = nj_v3_len(cam->forward);
  cam->forward = nj_v3_normalize(cam->forward);
  cam->up = {0.f, 1.f, 0.f};
  cam->view_mat = nj_look_forward_lh(cam->eye, cam->forward, cam->up);
  return true;
}

bool nj_cam_update(nj_cam_t* cam) {
  bool need_update_view = false;
  nj_v3_t cam_right = nj_v3_normalize(nj_v3_cross(cam->forward, cam->up));
  if (cam->w->m_key_down[NJ_KEY_W]) {
    nj_v3_t forward = cam->forward * 0.1f;
    cam->eye += forward;
    need_update_view = true;
  }
  if (cam->w->m_key_down[NJ_KEY_S]) {
    nj_v3_t backward = cam->forward * -0.1f;
    cam->eye += backward;
    need_update_view = true;
  }
  if (cam->w->m_key_down[NJ_KEY_D]) {
    nj_v3_t right = cam_right * 0.1f;
    cam->eye += right;
    need_update_view = true;
  }
  if (cam->w->m_key_down[NJ_KEY_A]) {
    nj_v3_t left = cam_right * -0.1f;
    cam->eye += left;
    need_update_view = true;
  }
  if (need_update_view) {
    cam->view_mat = nj_look_forward_lh(cam->eye, cam->forward, cam->up);
  }
  return need_update_view;
}

void nj_cam_mouse_move(nj_cam_t* cam, int x, int y) {
  if (cam->w->m_mouse_down[NJ_MOUSE_LEFT]) {
    cam->w->set_cursor_pos(cam->w->m_old_mouse_x[NJ_MOUSE_LEFT], cam->w->m_old_mouse_y[NJ_MOUSE_LEFT]);

    int delta_x = cam->w->m_old_mouse_x[NJ_MOUSE_LEFT] - x;
    int delta_y = cam->w->m_old_mouse_y[NJ_MOUSE_LEFT] - y;
    float angle_x = delta_x * 0.002f;
    float angle_y = delta_y * 0.002f;
    nj_v3_t right = nj_v3_cross(cam->forward, cam->up);
    nj_m4_t rotate_vert = nj_quat_to_m4(nj_quat_rotate_v3(right, angle_y));
    nj_m4_t rotate_hori = nj_quat_to_m4(nj_quat_rotate_v3(cam->up, angle_x));
    nj_m4_t rotate_mat = rotate_hori * rotate_vert;
    cam->forward = nj_v4_normalize(rotate_mat * (nj_v4_t){cam->forward.x, cam->forward.y, cam->forward.z, 1.f});
    cam->up = nj_v4_normalize(rotate_mat * (nj_v4_t){cam->up.x, cam->up.y, cam->up.z, 1.f});
    cam->view_mat = nj_look_forward_lh(cam->eye, cam->forward, cam->up);
  }

  if (cam->w->m_mouse_down[NJ_MOUSE_MIDDLE]) {
    int delta_x = cam->w->m_old_mouse_x[NJ_MOUSE_MIDDLE] - x;
    int delta_y = cam->w->m_old_mouse_y[NJ_MOUSE_MIDDLE] - y;
    float angle_x = delta_x * 0.002f;
    float angle_y = delta_y * 0.002f;
    nj_v3_t backward = cam->forward * -1.0f;
    nj_v3_t full_backward = backward * cam->dist;
    nj_v3_t target = cam->eye - full_backward;
    nj_v3_t right = nj_v3_cross(backward, cam->up);
    nj_m4_t rotate_vert = nj_quat_to_m4(nj_quat_rotate_v3(right, angle_y));
    nj_m4_t rotate_hori = nj_quat_to_m4(nj_quat_rotate_v3(cam->up, -angle_x));
    nj_m4_t rotate_mat = rotate_vert * rotate_hori;
    backward = rotate_mat * (nj_v4_t){backward.x, backward.y, backward.z, 1.f};
    full_backward = backward * cam->dist;
    cam->eye = target + full_backward;
    cam->forward = backward * -1.0f;
    cam->up = rotate_mat * (nj_v4_t){cam->up.x, cam->up.y, cam->up.z, 1.f};
    cam->view_mat = nj_look_forward_lh(cam->eye, cam->forward, cam->up);
  }
}

void nj_cam_mouse_event(nj_cam_t* cam, enum nj_mouse mouse, int x, int y, bool is_down) {
  if (mouse == NJ_MOUSE_LEFT)
    cam->w->show_cursor(!is_down);
}
