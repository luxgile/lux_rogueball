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

struct sPhysicsTime {
  uint64_t last_time = 0;
  float fixed_dt = 0.016f;
};

struct cPhysicsInit {};
struct sPhysicsWorld {
  float pixel_to_meters;
  b2WorldId id;
};

struct sPhysicsDebugDraw {
  b2DebugDraw debug;
};

enum cPhysicsBodyType {
  Dynamic,
  Kinematic,
  Static,
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

struct cRestitution {
  float value;
};

struct eApplyForce {
  glm::vec2 force;
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

struct cSensor {};
struct cSensorEvents {};

struct rPhysicsRoot {};

struct eTouchBegin {
  flecs::entity sensor;
  flecs::entity visitor;
};

struct eTouchEnd {
  flecs::entity sensor;
  flecs::entity visitor;
};

struct physics_module {
  physics_module(flecs::world &world);

  static void apply_force(flecs::entity e, const glm::vec2 &force) {
    e.world()
        .event<eApplyForce>()
        .id<cPhysicsBody>()
        .entity(e)
        .ctx({.force = force})
        .emit();
  }
};
