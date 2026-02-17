#pragma once
#include "modules/input_module.hpp"
#include "modules/lua_module.hpp"
#include "modules/physics_module.hpp"
#include "modules/render_module.hpp"
#include "modules/transform_module.hpp"
#include <flecs.h>

struct sTime {
  float elapsed;
  float delta;
};

struct sWindowSize {
  int width;
  int height;

  glm::vec2 get_size() const { return glm::vec2{width, height}; }
};

struct engine_module {
  engine_module(flecs::world &world) {
    world.module<engine_module>();

    world.component<sWindowSize>()
        .member<int>("width")
        .member<int>("height")
        .add(flecs::Singleton);

    world.observer<sWindowSize>()
        .event(flecs::OnSet)
        .each([](sWindowSize &size) {
          Luxlib::instance().render_server.set_camera_resolution(
              {size.width, size.height});
        });

    world.set<sWindowSize>({1280, 720});

    world.component<sTime>()
        .member<float>("elapsed")
        .member<float>("delta")
        .add(flecs::Singleton);

    world.add<sTime>();

    world.system<sTime>("Update Time")
        .kind(flecs::OnStore)
        .each([](flecs::iter &it, size_t, sTime &time) {
          time.delta = it.delta_time();
          time.elapsed += time.delta;
        });

    world.import <lua_module>();
    world.import <transform_module>();
    world.import <render_module>();
    world.import <physics_module>();
    world.import <input_module>();
  }
};
