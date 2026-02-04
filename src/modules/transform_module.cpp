#include "transform_module.hpp"
#include "glm/glm.hpp"

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
        // Build Local Matrix
        glm::mat3 local = glm::mat3(1.0f);

        float cos_a = cos(local_rot.value);
        float sin_a = sin(local_rot.value);

        local[0][0] = cos_a * local_scale.value.x;
        local[1][0] = -sin_a * local_scale.value.y;
        local[2][0] = local_pos.value.x;
        local[0][1] = sin_a * local_scale.value.x;
        local[1][1] = cos_a * local_scale.value.y;
        local[2][1] = local_pos.value.y;
        local[0][2] = 0.0f;
        local[1][2] = 0.0f;
        local[2][2] = 1.0f;

        // 2. Combine with Parent
        if (parent_world) {
          world_out.model = parent_world->model * local;
        } else {
          world_out.model = local;
        }
      });
}
