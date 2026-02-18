#include "common_module.hpp"
#include "glm/glm.hpp"
#include <string>

common_module::common_module(flecs::world &world) {
  world.module<common_module>();

  world.component<std::string>()
      .opaque(flecs::String)
      .serialize([](const flecs::serializer *s, const std::string *data) {
        const char *str = data->c_str();
        return s->value(flecs::String, &str);
      })
      .assign_string(
          [](std::string *data, const char *value) { *data = value; });

  world.component<glm::vec2>().member<float>("x").member<float>("y");
}
