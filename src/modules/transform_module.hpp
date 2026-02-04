#pragma once

#include "flecs.h"
#include "glm/ext/vector_float2.hpp"

struct Position2 {
  glm::vec2 value;
};

struct Rotation2 {
  float value;
};

struct Scale2 {
  glm::vec2 value;
};

struct WorldTransform2 {
  glm::vec2 position;
  float rotation;
  glm::vec2 scale;
};

struct transform_module {
  transform_module(flecs::world &world);
};
