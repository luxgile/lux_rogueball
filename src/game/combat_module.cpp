#include "combat_module.hpp"
#include "../modules/common_module.hpp"
#include "../modules/physics_module.hpp"
#include "game_module.hpp"

combat_module::combat_module(flecs::world &world) {
  world.module<combat_module>();

  world.component<sHitStop>().member<float>("acc").add(flecs::Singleton);
  world.add<sHitStop>();

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
}
