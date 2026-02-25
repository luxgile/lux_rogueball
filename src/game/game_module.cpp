#include "game_module.hpp"

game_module::game_module(flecs::world &world) {
  world.module<game_module>();

  world.import<base_module>();
  world.import<mechanics_module>();
  world.import<interaction_module>();
  world.import<combat_module>();
  world.import<spawner_module>();

  world.script_run_file("./assets/game.flecs");
}
