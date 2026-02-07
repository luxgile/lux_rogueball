#pragma once
#include "modules/physics_module.hpp"
#include "modules/render_module.hpp"
#include "modules/transform_module.hpp"
#include <flecs.h>

struct sTime {
  float elapsed;
  float delta;
};

struct engine_module {
  engine_module(flecs::world &world) {
    world.module<engine_module>();

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

    world.import <transform_module>();
    world.import <render_module>();
    world.import <physics_module>();
  }
};
