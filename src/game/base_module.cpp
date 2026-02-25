#include "base_module.hpp"
#include "../modules/physics_module.hpp"
#include "box2d/box2d.h"
#include "glm/glm.hpp"

base_module::base_module(flecs::world &world) {
  world.module<base_module>();

  world.component<cCharacter>();
  world.component<cPlayer>().is_a<cCharacter>().set_alias("cPlayer");
  world.component<cEnemy>().is_a<cCharacter>();
  world.component<cHealth>().member<int>("value");
  world.component<cWeapon>().member<int>("damage");

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
}
