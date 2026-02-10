#include "input_module.hpp"

input_module::input_module(flecs::world &world) {
  world.module<input_module>();

  world.component<sInputState>()
      .member<glm::vec2>("mouse position")
      .add(flecs::Singleton);

  world.add<sInputState>();
}
