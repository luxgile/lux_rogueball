#pragma once

#include "flecs.h"
#include "glm/glm.hpp"

struct cCharacter {};
struct cPlayer {};

struct rSmoothFollow {
  float smoothness;
};

struct cDragData {
  glm::vec2 start = {0.0f, 0.0f};
  glm::vec2 end = {0.0f, 0.0f};
  bool dragging = false;
};

struct DamageInfo {
  flecs::entity dealer;
  int damage;
};

struct eDealDamage {
  DamageInfo info;
};

struct cHealth {
  int value;
};

struct game_module {
  game_module(flecs::world &world);
};
