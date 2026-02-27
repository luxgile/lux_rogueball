#pragma once

#include "flecs.h"

// Forward declare submodules if needed, or just include them
#include "combat_module.hpp"

struct cCharacter {};
struct cPlayer {};
struct cEnemy {};

struct cDragData {
  glm::vec2 start = {0.0f, 0.0f};
  glm::vec2 end = {0.0f, 0.0f};
  bool dragging = false;
};

struct cConstRotation {
  float degrees;
};

struct rSmoothFollow {
  float smoothness;
};

struct game_module {
  game_module(flecs::world &world);
};
