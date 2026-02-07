#pragma once

#include "flecs.h"
#include "glm/ext/vector_float2.hpp"
#include "glm/glm.hpp"

struct cPosition2 {
  glm::vec2 value;
};

struct cRotation2 {
  float value;
};

struct cScale2 {
  glm::vec2 value;
};

struct cWorldTransform2 {
  glm::mat3 model;
};

struct transform_module {
  transform_module(flecs::world &world);
};
