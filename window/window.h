//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#ifndef NJ_WINDOW_WINDOW_H
#define NJ_WINDOW_WINDOW_H

#include "core/os_string.h"
#include "window/input.h"

struct nj_allocator_t;
struct nj_window_platform_t;

struct nj_window_t {
  bool init(nj_allocator_t* in_allocator, const nj_os_char* in_title);
  virtual void destroy();
  void os_loop();
  virtual void loop() = 0;
  virtual void on_mouse_event(enum nj_mouse mouse, int x, int y, bool is_down) = 0;
  virtual void on_mouse_move(int x, int y) = 0;
  virtual void on_key_event(enum nj_key key, bool is_down) = 0;

  bool key_down[NJ_KEY_COUNT] = {};
  bool mouse_down[NJ_MOUSE_COUNT] = {};
  int old_mouse_x[NJ_MOUSE_COUNT] = {};
  int old_mouse_y[NJ_MOUSE_COUNT] = {};
  const nj_os_char* title;
  nj_allocator_t* allocator;
  nj_window_platform_t* platform_data;
};

#endif // NJ_WINDOW_WINDOW_H
