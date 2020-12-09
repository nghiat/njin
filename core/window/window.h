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
      : m_allocator(allocator), m_title(title), m_width(width), m_height(height) {}
  bool init();
  virtual void destroy();

  void os_loop();
  void show_cursor(bool show);
  void set_cursor_pos(int x, int y);
  virtual void loop() {}
  virtual void on_mouse_event(enum nj_mouse mouse, int x, int y, bool is_down) {}
  virtual void on_mouse_move(int x, int y) {}
  virtual void on_key_event(enum nj_key key, bool is_down) {}

  bool m_key_down[NJ_KEY_COUNT] = {};
  bool m_mouse_down[NJ_MOUSE_COUNT] = {};
  int m_old_mouse_x[NJ_MOUSE_COUNT] = {};
  int m_old_mouse_y[NJ_MOUSE_COUNT] = {};
  bool m_is_cursor_visible = true;
  nj_allocator_t* m_allocator;
  const nj_os_char* m_title;
  int m_width;
  int m_height;

  nj_window_platform_t* m_platform_data;
  void* m_handle = NULL;
};

#endif // NJ_CORE_WINDOW_WINDOW_H
