//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2020             //
//----------------------------------------------------------------------------//

#ifndef NJ_CORE_WINDOW_WINDOW_H
#define NJ_CORE_WINDOW_WINDOW_H

#include "core/os_string.h"
#include "core/window/input.h"

struct nj_allocator_t;
struct nj_window_platform_t;

struct nj_window_t {
  nj_window_t(nj_allocator_t* allocator, const nj_os_char* title, int width, int height)
      : allocator(allocator), title(title), width(width), height(height) {}
  bool init();
  virtual void destroy();

  void os_loop();
  void show_cursor(bool show);
  void set_cursor_pos(int x, int y);
  virtual void loop() {}
  virtual void on_mouse_event(enum nj_mouse mouse, int x, int y, bool is_down) {}
  virtual void on_mouse_move(int x, int y) {}
  virtual void on_key_event(enum nj_key key, bool is_down) {}

  bool key_down[NJ_KEY_COUNT] = {};
  bool mouse_down[NJ_MOUSE_COUNT] = {};
  int old_mouse_x[NJ_MOUSE_COUNT] = {};
  int old_mouse_y[NJ_MOUSE_COUNT] = {};
  bool is_cursor_visible = true;
  nj_allocator_t* allocator;
  const nj_os_char* title;
  int width;
  int height;

  nj_window_platform_t* platform_data;
  void* handle = NULL;
};

#endif // NJ_CORE_WINDOW_WINDOW_H
