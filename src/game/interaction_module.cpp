#include "interaction_module.hpp"
#include "../engine_module.hpp"
#include "../luxlib.hpp"
#include "../modules/common_module.hpp"
#include "../modules/input_module.hpp"
#include "../modules/physics_module.hpp"
#include "../modules/transform_module.hpp"
#include "base_module.hpp"

static glm::vec2 viewport_to_world(glm::vec2 viewport_pos, glm::vec2 size) {
  auto world = viewport_pos - size / 2.0f;
  return glm::vec2{world.x, -world.y};
}

interaction_module::interaction_module(flecs::world &world) {
  world.module<interaction_module>();

  world.component<cDragData>()
      .member<glm::vec2>("start")
      .member<glm::vec2>("end")
      .member<bool>("dragging");

  world
      .system<sInputState, const sWindowSize, cPosition2, cDragData>(
          "Grab and push player")
      .with<cPlayer>()
      .each([](flecs::entity e, sInputState &input, const sWindowSize &size,
               cPosition2 &pos, cDragData &drag) {
        if (input.pressed_mouse[SAPP_MOUSEBUTTON_LEFT]) {
          auto mouse_world =
              viewport_to_world(input.mouse_viewport_position, size.get_size());

          if (!drag.dragging) {
            if (glm::distance(pos.value, mouse_world) < 32.0f) {
              drag.start = pos.value;
              drag.dragging = true;
            }
          } else {
            drag.end = mouse_world;
            Luxlib::instance().render_server.draw_line(drag.start, drag.end,
                                                       WHITE);
          }
        } else {
          if (drag.dragging) {
            drag.dragging = false;

            auto force = drag.start - drag.end;
            physics_module::apply_force(e, force * 10.0f);
          }
        }
      });
}
