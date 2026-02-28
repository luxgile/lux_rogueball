#include "physics_module.hpp"
#include "../luxlib.hpp"
#include "box2d/box2d.h"
#include "box2d/id.h"
#include "box2d/math_functions.h"
#include "box2d/types.h"
#include "common_module.hpp"
#include "flecs/addons/cpp/iter.hpp"
#include "glm/ext/quaternion_trigonometric.hpp"
#include "glm/ext/vector_float2.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/trigonometric.hpp"
#include "imgui.h"
#include "sokol_time.h"
#include "transform_module.hpp"
#include <cstddef>
#include <ranges>

void draw_physics_solid_circles(b2Transform xform, float radius,
                                b2HexColor color, void *context) {
  auto &rendering = Luxlib::instance().render_server;
  auto &pworld = Luxlib::instance().world.get<sPhysicsWorld>();

  auto pos = glm::vec2{xform.p.x, xform.p.y} * pworld.pixel_to_meters;
  auto pixel_radius = radius * pworld.pixel_to_meters;
  rendering.draw_circle(pos, pixel_radius, Srgba::from_hex(color));
}

void draw_physics_solid_polygon(b2Transform xform, const b2Vec2 *vertices,
                                int32_t vertexCount, float radius,
                                b2HexColor color, void *context) {
  if (vertexCount != 4)
    return;

  auto &rendering = Luxlib::instance().render_server;
  auto &pworld = Luxlib::instance().world.get<sPhysicsWorld>();

  auto v0 = glm::vec2{vertices[0].x, vertices[0].y} * pworld.pixel_to_meters;
  auto v1 = glm::vec2{vertices[1].x, vertices[1].y} * pworld.pixel_to_meters;
  auto v2 = glm::vec2{vertices[2].x, vertices[2].y} * pworld.pixel_to_meters;
  auto v3 = glm::vec2{vertices[3].x, vertices[3].y} * pworld.pixel_to_meters;
  rendering.draw_quad(v0, v1, v2, v3,
                      glm::vec2{xform.p.x, xform.p.y} * pworld.pixel_to_meters,
                      b2Rot_GetAngle(xform.q), Srgba::from_hex(color));
}

void draw_physics_point(b2Vec2 position, float size, b2HexColor color,
                        void *context) {
  auto &rendering = Luxlib::instance().render_server;
  auto &pworld = Luxlib::instance().world.get<sPhysicsWorld>();
  auto pos = glm::vec2{position.x, position.y} * pworld.pixel_to_meters;
  rendering.draw_point(pos, Srgba::from_hex(color),
                       size * pworld.pixel_to_meters);
}

void draw_physics_transform(const b2Transform xform, void *context) {
  auto &rendering = Luxlib::instance().render_server;
  auto &pworld = Luxlib::instance().world.get<sPhysicsWorld>();
  auto pos = glm::vec2{xform.p.x, xform.p.y} * pworld.pixel_to_meters;
  rendering.draw_point(pos, Srgba::from_hex(0xFF0000FF), 1.0f);
}

void draw_physics_segment(b2Vec2 p1, b2Vec2 p2, b2HexColor color,
                          void *context) {
  auto &rendering = Luxlib::instance().render_server;
  auto &pworld = Luxlib::instance().world.get<sPhysicsWorld>();
  auto wp1 = glm::vec2{p1.x, p1.y} * pworld.pixel_to_meters;
  auto wp2 = glm::vec2{p2.x, p2.y} * pworld.pixel_to_meters;
  rendering.draw_line(wp1, wp2, Srgba::from_hex(color));
}

void init_entity_physics_shape(const sPhysicsWorld &pworld, flecs::entity root,
                               flecs::entity e, b2BodyId body) {
  auto shape = e.try_get_mut<cPhysicsShape>();
  if (!shape)
    return;

  b2ShapeDef shape_def = b2DefaultShapeDef();
  shape_def.userData = (void *)e.id();
  if (auto density = e.try_get<cDensity>())
    shape_def.density = density->value;

  if (auto friction = e.try_get<cFriction>())
    shape_def.material.friction = friction->value;

  if (auto restitution = e.try_get<cRestitution>())
    shape_def.material.restitution = restitution->value;

  shape_def.isSensor = e.has<cSensor>();
  shape_def.enableSensorEvents = shape_def.isSensor || e.has<cSensorEvents>();

  auto center = b2Vec2_zero;
  if (e != root) {
    if (auto position = e.try_get<cPosition2>()) {
      center.x = position->value.x / pworld.pixel_to_meters;
      center.y = position->value.y / pworld.pixel_to_meters;
    }
  }

  switch (shape->type) {
  case Circle: {
    auto size = shape->size.x / pworld.pixel_to_meters;
    b2Circle circle_shape = b2Circle{.center = {0.0, 0.0}, .radius = size};
    circle_shape.center = center;
    shape->id = b2CreateCircleShape(body, &shape_def, &circle_shape);
    break;
  }

  case Box: {
    auto size = shape->size / pworld.pixel_to_meters;
    b2Polygon box_shape =
        b2MakeOffsetBox(size.x, size.y, center, b2Rot_identity);
    shape->id = b2CreatePolygonShape(body, &shape_def, &box_shape);
    break;
  }
  }

  e.add<rPhysicsRoot>(root);
}

physics_module::physics_module(flecs::world &world) {
  world.module<physics_module>();

  world.component<eApplyForce>();

  world.component<b2WorldId>().member<uint16_t>("index").member<uint16_t>(
      "generation");
  world.component<sPhysicsWorld>().member<b2WorldId>("id").add(
      flecs::Singleton);

  world.component<sPhysicsTime>()
      .member<uint64_t>("last_time")
      .member<float>("fixed_dt")
      .add(flecs::Singleton);
  world.add<sPhysicsTime>();

  world.component<b2DebugDraw>()
      .member<bool>("drawShapes")
      .member<bool>("drawJoints")
      .member<bool>("drawJointExtras")
      .member<bool>("drawBounds")
      .member<bool>("drawMass")
      .member<bool>("drawBodyNames")
      .member<bool>("drawContacts")
      .member<bool>("drawGraphColors")
      .member<bool>("drawContactNormals")
      .member<bool>("drawContactImpulses")
      .member<bool>("drawContactFeatures")
      .member<bool>("drawFrictionImpulses")
      .member<bool>("drawIslands");
  world.component<sPhysicsDebugDraw>().member<b2DebugDraw>("debug").add(
      flecs::Singleton);

  world.component<eTouchBegin>()
      .member<flecs::entity>("sensor")
      .member<flecs::entity>("visitor");
  world.component<eTouchEnd>()
      .member<flecs::entity>("sensor")
      .member<flecs::entity>("visitor");

  // World management
  world.observer<sPhysicsWorld>()
      .event(flecs::OnAdd)
      .each([](sPhysicsWorld &world) {
        b2WorldDef pworld_def = b2DefaultWorldDef();
        pworld_def.gravity = {0.0, -9.8};
        world.pixel_to_meters = 32;
        world.id = b2CreateWorld(&pworld_def);
      });

  world.observer<sPhysicsWorld>()
      .event(flecs::OnRemove)
      .each([](sPhysicsWorld &world) { b2DestroyWorld(world.id); });

  world.add<sPhysicsWorld>();

  auto debug_draw = b2DefaultDebugDraw();
  debug_draw.drawShapes = true;
  debug_draw.drawMass = true;
  debug_draw.useDrawingBounds = false;
  debug_draw.DrawSolidCircleFcn = draw_physics_solid_circles;
  debug_draw.DrawSolidPolygonFcn = draw_physics_solid_polygon;
  debug_draw.DrawTransformFcn = draw_physics_transform;
  debug_draw.DrawPointFcn = draw_physics_point;
  debug_draw.DrawSegmentFcn = draw_physics_segment;
  world.set(sPhysicsDebugDraw{debug_draw});

  // Body management
  world.component<b2BodyId>()
      .member<uint32_t>("index")
      .member<uint16_t>("world")
      .member<uint16_t>("generation");

  world.component<b2ShapeId>()
      .member<uint32_t>("index")
      .member<uint16_t>("world")
      .member<uint16_t>("generation");

  world.component<cPhysicsBody>()
      .member<b2BodyId>("id")
      .add(flecs::With, world.component<cWorldTransform2>())
      .add(flecs::With, world.component<cFriction>())
      .add(flecs::With, world.component<cRestitution>())
      .add(flecs::With, world.component<cPhysicsShape>())
      .add(flecs::With, world.component<cPhysicsBodyType>())
      .add(flecs::With, world.component<cPhysicsInit>())
      .add(flecs::With, world.component<cDensity>());

  world.component<cFriction>().member<float>("value");
  world.component<cDensity>().member<float>("value");
  world.component<cRestitution>().member<float>("value");
  world.component<ShapeType>()
      .constant("Circle", ShapeType::Circle)
      .constant("Box", ShapeType::Box);
  world.component<cPhysicsShape>()
      .member<b2ShapeId>("id")
      .member<ShapeType>("type")
      .member<glm::vec2>("size");
  world.component<cSensor>();
  world.component<cSensorEvents>();

  world
      .system<const sPhysicsWorld, cPhysicsBody, const cFriction,
              const cDensity, const cRestitution, const cPhysicsBodyType,
              cPhysicsShape, cPosition2 *, cRotation2 *>()
      .with<cPhysicsInit>()
      .kind(flecs::OnLoad)
      .each([](flecs::entity e, const sPhysicsWorld pworld, cPhysicsBody &body,
               const cFriction &friction, const cDensity &density,
               const cRestitution &restitution,
               const cPhysicsBodyType &body_type, cPhysicsShape &shape,
               cPosition2 *pos, cRotation2 *rot) {
        b2BodyDef body_def = b2DefaultBodyDef();
        body_def.userData = &e;
        switch (body_type) {
        case Dynamic:
          body_def.type = b2_dynamicBody;
          break;
        case Kinematic:
          body_def.type = b2_kinematicBody;
          break;
        case Static:
          body_def.type = b2_staticBody;
          break;
        }

        if (pos)
          body_def.position = {.x = pos->value.x / pworld.pixel_to_meters,
                               .y = pos->value.y / pworld.pixel_to_meters};
        if (rot)
          body_def.rotation = b2MakeRot(glm::radians(rot->value));
        body.id = b2CreateBody(pworld.id, &body_def);

        init_entity_physics_shape(pworld, e, e, body.id);

        e.children([&body, &pworld, &e](flecs::entity child) {
          init_entity_physics_shape(pworld, e, child, body.id);
        });

        e.remove<cPhysicsInit>();
      });

  world.observer<cPhysicsBody>()
      .event(flecs::OnRemove)
      .each(
          [](flecs::entity e, cPhysicsBody &body) { b2DestroyBody(body.id); });

  // Process
  world.system<const sPhysicsWorld, sPhysicsTime>("Physics Update")
      .kind(flecs::PreUpdate)
      .each([](flecs::iter &it, size_t, const sPhysicsWorld &world,
               sPhysicsTime &time) {
        // Iterate physics
        time.acc += stm_sec(stm_since(time.last_time)) * time.scale;
        time.last_time = stm_now();
        auto iterations = 0;
        while (time.fixed_dt > 0.0f && time.acc >= time.fixed_dt) {
          b2World_Step(world.id, time.fixed_dt, 4);
          time.acc -= time.fixed_dt;
          iterations += 1;
        }

        // Return early to avoid raising the same events multiple frames
        if (iterations == 0)
          return;

        // Trigger sensor events
        auto sensor_events = b2World_GetSensorEvents(world.id);
        for (int i = 0; i < sensor_events.beginCount; ++i) {
          auto begin_event = sensor_events.beginEvents + i;
          auto visitor_id =
              (flecs::entity_t)b2Shape_GetUserData(begin_event->visitorShapeId);
          auto visitor = it.world().get_alive(visitor_id);
          auto sensor_id =
              (flecs::entity_t)b2Shape_GetUserData(begin_event->sensorShapeId);
          auto sensor = it.world().get_alive(sensor_id);
          if (visitor && visitor.is_valid() && sensor && sensor.is_valid()) {
            emit_event<eTouchBegin, cSensor>(
                sensor, {.sensor = sensor, .visitor = visitor});
          }
        }

        for (int i = 0; i < sensor_events.endCount; ++i) {
          auto end_event = sensor_events.endEvents + i;
          auto visitor_id =
              (flecs::entity_t)b2Shape_GetUserData(end_event->visitorShapeId);
          auto visitor = it.world().get_alive(visitor_id);
          auto sensor_id =
              (flecs::entity_t)b2Shape_GetUserData(end_event->sensorShapeId);
          auto sensor = it.world().get_alive(sensor_id);
          if (visitor && visitor.is_valid() && sensor && sensor.is_valid()) {
            emit_event<eTouchEnd, cSensor>(
                visitor, {.sensor = sensor, .visitor = visitor});
          }
        }
      });

  world.system<const sPhysicsWorld, sPhysicsDebugDraw>("Draw Physics")
      .each([](const sPhysicsWorld &pworld, sPhysicsDebugDraw &draw) {
        auto &rendering = Luxlib::instance().render_server;
        b2World_Draw(pworld.id, &draw.debug);
      });

  // Sync position and rotation from physics
  // TODO: Use body events instead for better performance
  world
      .system<const sPhysicsWorld, const cPhysicsBody, cPosition2>(
          "Sync Position to Physics")
      .kind(flecs::OnUpdate)
      .each([](const sPhysicsWorld &pworld, const cPhysicsBody &body,
               cPosition2 &position) {
        b2Vec2 pos = b2Body_GetPosition(body.id);
        position.value.x = pos.x * pworld.pixel_to_meters;
        position.value.y = pos.y * pworld.pixel_to_meters;
      });

  world.system<const cPhysicsBody, cRotation2>("Sync Rotation to Physics")
      .kind(flecs::OnUpdate)
      .each([](const cPhysicsBody &body, cRotation2 &rotation) {
        b2Rot pos = b2Body_GetRotation(body.id);
        rotation.value = glm::degrees(b2Rot_GetAngle(pos));
      });

  // Apply force
  world.observer<cPhysicsBody>().event<eApplyForce>().each(
      [](flecs::iter &it, size_t, cPhysicsBody &body) {
        auto force = it.param<eApplyForce>()->force;
        spdlog::info("Applying force {}, {}", force.x, force.y);
        b2Body_ApplyForce(body.id, {force.x, force.y}, b2Vec2_zero, true);
      });
}
