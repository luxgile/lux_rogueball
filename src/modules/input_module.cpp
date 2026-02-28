#include "input_module.hpp"

input_module::input_module(flecs::world &world) {
  world.module<input_module>();

  world.component<sInputState>()
      .member<glm::vec2>("mouse position")
      .add(flecs::Singleton);

  world.system<sInputState>("Reset input")
      .kind(flecs::OnStore)
      .each([](sInputState &input) { input.mouse_scroll = 0.0f; });

  world.add<sInputState>();
}
