#include "game_module.hpp"
#include "../engine_module.hpp"
#include "../luxlib.hpp"
#include "../modules/input_module.hpp"
#include "flecs.h"

glm::vec2 viewport_to_world(glm::vec2 viewport_pos, glm::vec2 size) {
  auto world = viewport_pos - size / 2.0f;
  return glm::vec2{world.x, -world.y};
}

game_module::game_module(flecs::world &world) {
  world.script_run_file("./assets/game.flecs");

  world.system<const sInputState, const sWindowSize>().each(
      [](const sInputState &input, const sWindowSize &size) {
        auto &render_server = Luxlib::instance().render_server;

        render_server.draw_line(viewport_to_world(input.mouse_viewport_position,
                                                  {size.width, size.height}),
                                {0.0, 0.0}, {1.0, 0.0, 1.0, 1.0}, 2.0f);
      });
}
