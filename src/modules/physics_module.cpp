#include "physics_module.hpp"
#include "box2d/box2d.h"
#include "box2d/id.h"
#include "box2d/math_functions.h"
#include "box2d/types.h"
#include "common_module.hpp"
#include "glm/ext/vector_float2.hpp"
#include "glm/trigonometric.hpp"
#include "transform_module.hpp"
#include <cstddef>

void init_children_shapes(flecs::entity root, flecs::entity parent,
                          b2BodyId body_id) {
  parent.children([&body_id, &root](flecs::entity child) {
    // Skip entities that already have a body
    if (child.has<cPhysicsBody>())
      return;

    auto shape = child.try_get_mut<cPhysicsShape>();
    if (!shape)
      return;

    b2ShapeDef shape_def = b2DefaultShapeDef();
    if (auto density = child.try_get<cDensity>())
      shape_def.density = density->value;

    if (auto friction = child.try_get<cFriction>())
      shape_def.material.friction = friction->value;

    if (auto restitution = child.try_get<cRestitution>())
      shape_def.material.restitution = restitution->value;

    auto sensor = child.try_get<cSensor>();
    shape_def.isSensor = sensor != nullptr;
    if (sensor)
      shape_def.enableSensorEvents = sensor->evaluate_events;

    auto position = glm::vec2(0.0, 0.0);
    if (auto pos = child.try_get<cPosition2>())
      position = pos->value;

    switch (shape->type) {
    case Circle: {
      b2Circle circle_shape = b2Circle{.center = {position.x, position.y},
                                       .radius = shape->size.x / 2.0f};
      shape->id = b2CreateCircleShape(body_id, &shape_def, &circle_shape);
      break;
    }

    case Box: {
      b2Polygon box_shape = b2MakeBox(shape->size.x / 2.0, shape->size.y / 2.0);
      box_shape.centroid = {position.x, position.y};
      shape->id = b2CreatePolygonShape(body_id, &shape_def, &box_shape);
      break;
    }
    }

    child.add<rPhysicsRoot>(root);

    init_children_shapes(root, child, body_id);
  });
}

physics_module::physics_module(flecs::world &world) {
  world.module<physics_module>();

  world.component<eApplyForce>();
  world.component<eTouchBegin>();
  world.component<eTouchEnd>();

  world.component<b2WorldId>().member<uint16_t>("index").member<uint16_t>(
      "generation");
  world.component<sPhysicsWorld>().member<b2WorldId>("id").add(
      flecs::Singleton);

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
      .add(flecs::With, world.component<tPhysicsInit>())
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
  world.component<cSensor>().member<bool>("evaluate_events");

  world
      .system<const sPhysicsWorld, cPhysicsBody, const cFriction,
              const cDensity, const cRestitution, const cPhysicsBodyType,
              cPhysicsShape, cPosition2 *, cRotation2 *>()
      .with<tPhysicsInit>()
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

        b2ShapeDef shape_def = b2DefaultShapeDef();
        shape_def.density = density.value;
        shape_def.material.friction = friction.value;
        shape_def.material.restitution = restitution.value;

        auto sensor = e.try_get<cSensor>();
        shape_def.isSensor = sensor != nullptr;
        if (sensor)
          shape_def.enableSensorEvents = sensor->evaluate_events;

        switch (shape.type) {
        case Circle: {
          b2Circle circle_shape =
              b2Circle{.center = {0.0, 0.0}, .radius = shape.size.x / 2.0f};
          shape.id = b2CreateCircleShape(body.id, &shape_def, &circle_shape);
          break;
        }

        case Box: {
          b2Polygon box_shape =
              b2MakeBox(shape.size.x / 2.0, shape.size.y / 2.0);
          shape.id = b2CreatePolygonShape(body.id, &shape_def, &box_shape);
          break;
        }
        }

        init_children_shapes(e, e, body.id);

        e.remove<tPhysicsInit>();
      });

  world.observer<cPhysicsBody>()
      .event(flecs::OnRemove)
      .each(
          [](flecs::entity e, cPhysicsBody &body) { b2DestroyBody(body.id); });

  // Process
  world.system<const sPhysicsWorld>("Physics Update")
      .kind(flecs::PreUpdate)
      .each([](const sPhysicsWorld &world) {
        // Iterate physics
        b2World_Step(world.id, 0.016f, 4);

        // Trigger sensor events
        auto sensor_events = b2World_GetSensorEvents(world.id);
        for (int i = 0; i < sensor_events.beginCount; ++i) {
          auto begin_event = sensor_events.beginEvents + i;
          flecs::entity *entity =
              (flecs::entity *)b2Shape_GetUserData(begin_event->sensorShapeId);
          if (entity && entity->is_valid()) {
            emit_event<eTouchBegin, cPhysicsBody>(*entity,
                                                  {.event = begin_event});
            spdlog::info("Sensor event");
          }
        }

        for (int i = 0; i < sensor_events.endCount; ++i) {
          auto end_event = sensor_events.endEvents + i;
          flecs::entity *entity =
              (flecs::entity *)b2Shape_GetUserData(end_event->sensorShapeId);
          if (entity && entity->is_valid()) {
            emit_event<eTouchEnd, cPhysicsBody>(*entity, {.event = end_event});
          }
        }
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
