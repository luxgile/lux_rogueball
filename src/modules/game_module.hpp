#pragma once

#include "flecs.h"

struct game_module {
  game_module(flecs::world &world) {
    world.script_run_file("./assets/game.flecs");
  }
};
