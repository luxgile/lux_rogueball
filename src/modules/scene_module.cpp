#include "scene_module.hpp"
#include "flecs/addons/cpp/c_types.hpp"

scene_module::scene_module(flecs::world &world) {
  world.module<scene_module>();

  world.component<cScene>();
  world.component<rLinkedScene>();
}
