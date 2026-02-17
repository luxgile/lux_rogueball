#include "transform_module.hpp"
#include "glm/ext/vector_float2.hpp"
#include "glm/glm.hpp"
#include "glm/trigonometric.hpp"
#include "lua_module.hpp"

transform_module::transform_module(flecs::world &world) {
  world.module<transform_module>();

  auto &clua = world.get_mut<sLuaState>();
  world.component<glm::vec2>().member<float>("x").member<float>("y");
  clua.lua.new_usertype<glm::vec2>("vec2", "x", &glm::vec2::x, "y",
                                   &glm::vec2::y);
  world.component<cPosition2>().member<glm::vec2>("value");
  clua.lua.new_usertype<cPosition2>("cPosition2", "value", &cPosition2::value);
  clua.type_pusher[world.id<cPosition2>()] = [&](void *ptr) {
    auto comp = (cPosition2 *)ptr;
    return sol::make_object(clua.lua, comp);
  };

  world.component<cRotation2>().member<float>("value");
  world.component<cScale2>().member<glm::vec2>("value");

  world.component<cWorldTransform2>()
      .add(flecs::With, world.component<cPosition2>())
      .add(flecs::With, world.component<cRotation2>())
      .add(flecs::With, world.component<cScale2>());

  world
      .system<cWorldTransform2, const cPosition2, const cRotation2,
              const cScale2, const cWorldTransform2 *>("Update World Transform")
      .term_at(4)
      .parent()
      .cascade()
      .optional()
      .each([](cWorldTransform2 &world_out, const cPosition2 &local_pos,
               const cRotation2 &local_rot, const cScale2 &local_scale,
               const cWorldTransform2 *parent_world) {
        // Build Local Matrix
        glm::mat3 local = glm::mat3(1.0f);

        float rad = glm::radians(local_rot.value);
        float cos_a = cos(rad);
        float sin_a = sin(rad);

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
