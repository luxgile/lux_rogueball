#include "mechanics_module.hpp"
#include "../modules/physics_module.hpp"
#include "../modules/transform_module.hpp"
#include "box2d/box2d.h"
#include "glm/glm.hpp"

mechanics_module::mechanics_module(flecs::world &world) {
  world.module<mechanics_module>();

  world.component<cConstRotation>().member<float>("degrees");

  world.component<rSmoothFollow>()
      .member<float>("smoothness")
      .add(flecs::Relationship);

  world.system<const cPhysicsBody, const cConstRotation>("Constant rotation")
      .each([](const cPhysicsBody &body, const cConstRotation &rotation) {
        b2Body_SetAngularVelocity(body.id, glm::radians(rotation.degrees));
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
}
