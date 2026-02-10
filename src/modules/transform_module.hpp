#pragma once

#include "flecs.h"
#include "glm/ext/vector_float2.hpp"
#include "glm/glm.hpp"

struct cPosition2 {
  glm::vec2 value = {0.0, 0.0};
};

struct cRotation2 {
  float value = 0.0;
};

struct cScale2 {
  glm::vec2 value = {1.0, 1.0};
};

struct cWorldTransform2 {
  glm::mat3 model;
};

struct transform_module {
  transform_module(flecs::world &world);
};
