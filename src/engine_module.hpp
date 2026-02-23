#pragma once
#include "glm/glm.hpp"
#include <flecs.h>

struct sTime {
  float elapsed;
  float real_elapsed;
  float delta;
  float real_delta;
  float scale = 1.0f;
};

struct sWindowSize {
  int width;
  int height;

  glm::vec2 get_size() const { return glm::vec2{width, height}; }
};

struct engine_module {
  engine_module(flecs::world &world);
};
