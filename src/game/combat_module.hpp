#pragma once

#include "flecs.h"

struct DamageInfo {
  flecs::entity dealer;
  int damage;
};

struct eDealDamage {
  DamageInfo info;
};

struct sHitStop {
  float acc;
};

struct combat_module {
  combat_module(flecs::world &world);
};
