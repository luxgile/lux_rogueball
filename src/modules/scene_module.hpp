#pragma once

#include "flecs.h"

struct cScene {};
struct rLinkedScene {};

struct scene_module {
  scene_module(flecs::world &world);
};
