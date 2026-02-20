#include "game_module.hpp"
#include "../engine_module.hpp"
#include "../luxlib.hpp"
#include "../modules/input_module.hpp"
#include "../modules/physics_module.hpp"
#include "box2d/box2d.h"
#include "flecs.h"
#include "flecs/addons/cpp/c_types.hpp"
#include "glm/geometric.hpp"
#include "glm/trigonometric.hpp"
#include "imgui.h"
#include <map>

glm::vec2 viewport_to_world(glm::vec2 viewport_pos, glm::vec2 size) {
  auto world = viewport_pos - size / 2.0f;
  return glm::vec2{world.x, -world.y};
}

game_module::game_module(flecs::world &world) {
  world.module<game_module>();

  world.component<cCharacter>();
  world.component<cPlayer>().is_a<cCharacter>();
  world.component<cDragData>()
      .member<glm::vec2>("start")
      .member<glm::vec2>("end")
      .member<bool>("dragging");

  world.component<rSmoothFollow>()
      .member<float>("smoothness")
      .add(flecs::Relationship);

  world.component<eApplyForce>();

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
            physics_module::apply_force(e, force);
          }
        }
      });

  world.system<cPhysicsBody>("Rotate characters")
      .with<cCharacter>()
      .each([](cPhysicsBody &body) {
        b2Body_SetAngularVelocity(body.id, glm::radians(90.0));
      });

  world.system<cPhysicsBody>("Clamp characters speed")
      .with<cCharacter>()
      .each([](cPhysicsBody &body) {
        const auto SPEED_LIMIT = 40.0f;
        auto b2velocity = b2Body_GetLinearVelocity(body.id);
        auto velocity = glm::vec2{b2velocity.x, b2velocity.y};
        auto speed = glm::length(velocity);
        if (speed > SPEED_LIMIT) {
          auto limited_velocity = glm::normalize(velocity) * SPEED_LIMIT;
          b2Body_SetLinearVelocity(body.id,
                                   {limited_velocity.x, limited_velocity.y});
        }
      });

  world.system<const rSmoothFollow, cPosition2>("Smooth Follow")
      .term_at(0)
      .second(flecs::Wildcard)
      .each([](flecs::entity e, const rSmoothFollow &smooth, cPosition2 &pos) {
        auto target_entity = e.target<rSmoothFollow>();
        if (!target_entity.is_valid() || !target_entity.has<cPosition2>())
          return;

        auto target_pos = target_entity.get<cPosition2>().value;
        pos.value = glm::mix(target_pos, pos.value, smooth.smoothness);
        ImGui::Text("Target: %f, %f", target_pos.x, target_pos.y);
      });

  world.observer<cHealth>().event<eDealDamage>().each(
      [](flecs::iter &it, size_t, cHealth health) {
        auto &info = it.param<eDealDamage>()->info;
        health.value -= info.damage;
      });

  world.script_run_file("./assets/game.flecs");
}
