#pragma once

#include "flecs.h"
#include "flecs/addons/cpp/mixins/pipeline/decl.hpp"
#include "glm/ext/vector_float2.hpp"
#include "sokol_app.h"
#include <map>
#include <vector>

struct sInputState {
  std::map<sapp_keycode, bool> pressed_keys;
  std::map<sapp_mousebutton, bool> pressed_mouse;
  float mouse_scroll;
  glm::vec2 mouse_viewport_position;
};

struct input_module {
  input_module(flecs::world &world);
};
