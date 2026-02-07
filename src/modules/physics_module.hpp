#pragma once

#include "box2d/box2d.h"
#include "box2d/collision.h"
#include "box2d/id.h"
#include "box2d/math_functions.h"
#include "box2d/types.h"
#include "flecs.h"
#include "flecs/addons/cpp/c_types.hpp"
#include "flecs/addons/cpp/mixins/pipeline/decl.hpp"
#include "glm/ext/vector_float2.hpp"
#include "spdlog/spdlog.h"
#include "transform_module.hpp"

struct tPhysicsInit {};
struct sPhysicsWorld {
  float pixel_to_meters;
  b2WorldId id;
};

struct cPhysicsBody {
  b2BodyId id;
};

struct cFriction {
  float value;
};

struct cDensity {
  float value;
};

enum ShapeType {
  Circle,
  Box,
};

struct cPhysicsShape {
  b2ShapeId id;
  ShapeType type;
  glm::vec2 size; // If it's a circle, only X is used.
};

struct physics_module {
  physics_module(flecs::world &world);
};
