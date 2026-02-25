#pragma once

#include "flecs.h"
#include "glm/glm.hpp"

struct cDragData {
  glm::vec2 start = {0.0f, 0.0f};
  glm::vec2 end = {0.0f, 0.0f};
  bool dragging = false;
};

struct interaction_module {
  interaction_module(flecs::world &world);
};
