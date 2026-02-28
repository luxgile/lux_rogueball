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

  auto position() const -> glm::vec2 {
    return glm::vec2{model[2][0], model[2][1]};
  }

  auto rotation() const -> float {
    return glm::degrees(atan2(model[1][0], model[0][0]));
  }

  auto scale() const -> glm::vec2 {
    return glm::vec2{glm::length(model[0]), glm::length(model[1])};
  }
};

struct transform_module {
  transform_module(flecs::world &world);
};
