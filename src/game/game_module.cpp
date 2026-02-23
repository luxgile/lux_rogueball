#include "game_module.hpp"
#include "../engine_module.hpp"
#include "../luxlib.hpp"
#include "../modules/common_module.hpp"
#include "../modules/input_module.hpp"
#include "../modules/physics_module.hpp"
#include "box2d/box2d.h"
#include "flecs.h"
#include "flecs/addons/cpp/c_types.hpp"
#include "glm/geometric.hpp"
#include "glm/trigonometric.hpp"
#include "imgui.h"
#include <map>
#include <random>

glm::vec2 viewport_to_world(glm::vec2 viewport_pos, glm::vec2 size) {
  auto world = viewport_pos - size / 2.0f;
  return glm::vec2{world.x, -world.y};
}

game_module::game_module(flecs::world &world) {
  world.module<game_module>();

  world.component<cCharacter>();
  world.component<cPlayer>().is_a<cCharacter>();
  world.component<cEnemy>().is_a<cCharacter>();
  world.component<cDragData>()
      .member<glm::vec2>("start")
      .member<glm::vec2>("end")
      .member<bool>("dragging");

  world.component<cConstRotation>().member<float>("degrees");
  world.component<cHealth>().member<int>("value");
  world.component<cWeapon>().member<int>("damage");

  world.component<rSmoothFollow>()
      .member<float>("smoothness")
      .add(flecs::Relationship);

  world.component<eApplyForce>();

  world.component<sHitStop>().member<float>("acc").add(flecs::Singleton);
  world.add<sHitStop>();

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

  world.system<const cPhysicsBody, const cConstRotation>("Constant rotation")
      .each([](const cPhysicsBody &body, const cConstRotation &rotation) {
        b2Body_SetAngularVelocity(body.id, glm::radians(rotation.degrees));
      });

  world.system<cPhysicsBody>("Clamp characters speed")
      .with<cCharacter>()
      .each([](cPhysicsBody &body) {
        const auto SPEED_LIMIT = 250.0f;
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
      });

  world.observer<cHealth>().event<eDealDamage>().each(
      [](flecs::iter &it, size_t i, cHealth &health) {
        auto &info = it.param<eDealDamage>()->info;
        health.value -= info.damage;

        // Hit stop
        if (auto stop = it.world().try_get_mut<sHitStop>()) {
          stop->acc = 0.2f;
        }

        if (health.value <= 0) {
          it.entity(i).destruct();
        }
      });

  // Damage system
  world.observer<cSensor>().event<eTouchBegin>().each(
      [](flecs::iter &it, size_t i, cSensor sensor) {
        auto collision = it.param<eTouchBegin>();
        if (auto weapon = collision->sensor.try_get<cWeapon>()) {
          // Deal the damage
          if (auto target_health = collision->visitor.try_get<cHealth>()) {
            emit_event<eDealDamage, cHealth>(
                collision->visitor,
                eDealDamage{.info = {.dealer = collision->sensor,
                                     .damage = weapon->damage}});
          }

          // Invert root rotation
          auto root = collision->sensor.target<rPhysicsRoot>();
          if (root.is_valid()) {
            if (auto rotation = root.try_get_mut<cConstRotation>()) {
              rotation->degrees = -rotation->degrees;
            }
          }
        }
      });

  world.system<sHitStop>("Hit stop")
      .each([](flecs::iter &it, size_t, sHitStop &stop) {
        if (stop.acc > 0.0f) {
          it.world().set_time_scale(0.0);
          it.world().get_mut<sPhysicsTime>().scale = 0.0;
          stop.acc -= it.delta_time();
          if (stop.acc < 0.0f) {
            it.world().set_time_scale(1.0);
            it.world().get_mut<sPhysicsTime>().scale = 2.5;
            stop.acc = 0.0f;
          }
        }
      });

  world.system("Enemy spawner").interval(3.0f).run([](flecs::iter &it) {
    auto world = it.world();
    auto rng = std::mt19937(std::random_device()());
    auto dist = std::uniform_real_distribution<float>(-128.0f, 128.0f);
    auto entity = world.entity();
    entity.add<cEnemy>();
    entity.set(cPosition2{glm::vec2{dist(rng), dist(rng)}});
    entity.set(cHealth{3});
    entity.add<cSensorEvents>();
    entity.add<cPhysicsBody>();
    entity.set(cPhysicsBodyType::Dynamic);
    entity.set(cDensity{1.0f});
    entity.set(cFriction{0.0f});
    entity.set(cRestitution{1.0f});
    entity.set(cPhysicsShape{.type = ShapeType::Circle, .size = {24.0f, 0.0f}});
  });

  world.script_run_file("./assets/game.flecs");
}
