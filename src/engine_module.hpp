#pragma once
#include "modules/render_module.hpp"
#include <flecs.h>

struct engine_module {
  engine_module(flecs::world &world) {
    world.module<engine_module>();
    world.import <render_module>();
  }
};
