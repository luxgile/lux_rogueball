#include "physics_module.hpp"
#include "box2d/id.h"
#include "glm/ext/vector_float2.hpp"

physics_module::physics_module(flecs::world &world) {
  world.module<physics_module>();

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
      .add(flecs::With, world.component<cPhysicsShape>())
      .add(flecs::With, world.component<tPhysicsInit>())
      .add(flecs::With, world.component<cDensity>());

  world.component<cFriction>().member<float>("value");
  world.component<cDensity>().member<float>("value");
  world.component<ShapeType>()
      .constant("Circle", ShapeType::Circle)
      .constant("Box", ShapeType::Box);
  world.component<cPhysicsShape>()
      .member<b2ShapeId>("id")
      .member<ShapeType>("type")
      .member<glm::vec2>("size");

  world
      .system<const sPhysicsWorld, cPhysicsBody, const cFriction,
              const cDensity, cPhysicsShape, cPosition2 *, cRotation2 *>()
      .with<tPhysicsInit>()
      .kind(flecs::OnLoad)
      .each([](flecs::entity e, const sPhysicsWorld pworld, cPhysicsBody &body,
               const cFriction &friction, const cDensity &density,
               cPhysicsShape &shape, cPosition2 *pos, cRotation2 *rot) {
        b2BodyDef body_def = b2DefaultBodyDef();
        body_def.type = b2_dynamicBody;
        if (pos)
          body_def.position = {.x = pos->value.x / pworld.pixel_to_meters,
                               .y = pos->value.y / pworld.pixel_to_meters};
        if (rot)
          body_def.rotation = b2MakeRot(rot->value);
        body.id = b2CreateBody(pworld.id, &body_def);

        b2ShapeDef shape_def = b2DefaultShapeDef();
        shape_def.density = density.value;
        shape_def.material.friction = friction.value;

        switch (shape.type) {
        case Circle: {
          b2Circle circle_shape =
              b2Circle{.center = {0.0, 0.0}, .radius = shape.size.x};
          shape.id = b2CreateCircleShape(body.id, &shape_def, &circle_shape);
          break;
        }

        case Box: {
          b2Polygon box_shape = b2MakeBox(shape.size.x, shape.size.y);
          shape.id = b2CreatePolygonShape(body.id, &shape_def, &box_shape);
          break;
        }
        }

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
        b2World_Step(world.id, 0.016f, 4);
      });

  // Sync position and rotation
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
        rotation.value = b2Rot_GetAngle(pos);
      });
}
