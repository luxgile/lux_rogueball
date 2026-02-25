#pragma once

#include "flecs.h"

// Forward declare submodules if needed, or just include them
#include "base_module.hpp"
#include "combat_module.hpp"
#include "interaction_module.hpp"
#include "mechanics_module.hpp"
#include "spawner_module.hpp"

struct game_module {
  game_module(flecs::world &world);
};
