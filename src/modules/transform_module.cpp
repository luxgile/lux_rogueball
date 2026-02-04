#include "transform_module.hpp"

transform_module::transform_module(flecs::world &world) {
  world.module<transform_module>();

  world.component<WorldTransform2>()
      .add(flecs::With, world.component<Position2>())
      .add(flecs::With, world.component<Rotation2>())
      .add(flecs::With, world.component<Scale2>());

  world
      .system<WorldTransform2, const Position2, const Rotation2, const Scale2,
              const WorldTransform2 *>("Update World Transform")
      .term_at(4)
      .parent()
      .cascade()
      .optional()
      .each([](WorldTransform2 &world_out, const Position2 &local_pos,
               const Rotation2 &local_rot, const Scale2 &local_scale,
               const WorldTransform2 *parent_world) {
        if (parent_world) {
          world_out.position = parent_world->position + local_pos.value;
          world_out.rotation = parent_world->rotation + local_rot.value;
          world_out.scale = parent_world->scale * local_scale.value;
        } else {
          world_out.position = local_pos.value;
          world_out.rotation = local_rot.value;
          world_out.scale = local_scale.value;
        }
      });
}
