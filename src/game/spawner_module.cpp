#include "spawner_module.hpp"
#include "../modules/common_module.hpp"
#include "../modules/physics_module.hpp"
#include "../modules/transform_module.hpp"
#include "base_module.hpp"
#include <random>

spawner_module::spawner_module(flecs::world &world) {
  world.module<spawner_module>();

  world.system("Enemy spawner").interval(3.0f).run([](flecs::iter &it) {
    auto world = it.world();
    auto rng = std::mt19937(std::random_device()());
    auto dist = std::uniform_real_distribution<float>(-128.0f, 128.0f);
    auto entity = world.entity();
    entity.add<cEnemy>();
    entity.set(cPosition2{glm::vec2{dist(rng), dist(rng)}});
    entity.set(cHealth{3});
    entity.add<cSensorEvents>();
    entity.add<cPhysicsBody>();
    entity.set(cPhysicsBodyType::Dynamic);
    entity.set(cDensity{1.0f});
    entity.set(cFriction{0.0f});
    entity.set(cRestitution{1.0f});
    entity.set(cPhysicsShape{.type = ShapeType::Circle, .size = {24.0f, 0.0f}});
  });
}
