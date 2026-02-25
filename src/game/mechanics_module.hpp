#pragma once

#include "flecs.h"

struct cConstRotation {
  float degrees;
};

struct rSmoothFollow {
  float smoothness;
};

struct mechanics_module {
  mechanics_module(flecs::world &world);
};
