/*
#include "lua_module.hpp"
#include "sol/forward.hpp"
#include "spdlog/spdlog.h"

lua_module::lua_module(flecs::world &world) {
  world.module<lua_module>();

  world.component<sLuaState>().add(flecs::Singleton);

  world.add<sLuaState>();
  auto &clua = world.get_mut<sLuaState>();
  clua.lua.open_libraries(sol::lib::base, sol::lib::string, sol::lib::table);

  auto ecs = clua.lua["ecs"].get_or_create<sol::table>();

  ecs["declare_component"] = [&](std::string name, sol::table layout) {
    flecs::untyped_component component = world.component(name.c_str());
    for (auto &[key, value] : layout) {
      std::string member_name = key.as<std::string>();
      flecs::entity_t type_id;

      std::string type = value.as<std::string>();
      if (type == "int")
        type_id = flecs::I32;
      if (type == "float")
        type_id = flecs::F32;
      if (type == "bool")
        type_id = flecs::Bool;
      if (type == "string")
        type_id = flecs::String;
      if (type == "entity")
        type_id = flecs::Entity;

      component.member(type_id, member_name.c_str());
    }
  };

  ecs["system"] = [&](sol::table system_desc) {
    auto system_name = system_desc.get_or<std::string>("name", "");
    auto query = system_desc.get_or<std::string>("query", "");
    auto callback = system_desc.get<sol::protected_function>("each");

    world.system(system_name.c_str())
        .expr(query.c_str())
        .run([callback, &clua](flecs::iter &it) {
          while (it.next()) {
            auto comp_id = it.type().get(0).entity().id();
            auto pusher = clua.type_pusher[comp_id];

            flecs::untyped_field data = it.field(0);
            for (auto i : it) {
              auto comp = data[i];
              auto result = callback(pusher(comp));
              if (!result.valid()) {
                sol::error err = result;
                spdlog::error("lua error: {}", err.what());
              }
            }
          }
        });
  };

  clua.lua.new_usertype<flecs::entity>("entity", "name", &flecs::entity::name,
                                       "id", &flecs::entity::id);
  clua.type_pusher[world.id<flecs::Identifier>()] = [&](void *ptr) {
    auto comp = (flecs::Identifier *)ptr;
    return sol::make_object(clua.lua, comp);
  };

  world.system().kind(flecs::OnStart).run([&clua](flecs::iter &it) {
    clua.lua.script(
        R"(
                                local c = ecs.components
                                ecs.system({
                                        name = 'DebugPosition',
                                        query = {
                                                get = [c.Position2],
                                        },
                                        each = function(p)
                                                print("My position is: " ..
p.value.x .. ", " .. p.value.y) end
                                })
                                )");
  });
}
*/
