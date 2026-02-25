#pragma once

#include "flecs.h"

struct cCharacter {};
struct cPlayer {};
struct cEnemy {};

struct cHealth {
  int value;
};

struct cWeapon {
  int damage;
};

struct base_module {
  base_module(flecs::world &world);
};
