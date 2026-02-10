#pragma once

#include "flecs.h"
#include "flecs/addons/cpp/mixins/pipeline/decl.hpp"
#include "glm/ext/vector_float2.hpp"
#include <map>
#include <vector>

struct sInputState {
  std::map<int, bool> pressed_keys;
  glm::vec2 mouse_viewport_position;
};

struct input_module {
  input_module(flecs::world &world);
};
