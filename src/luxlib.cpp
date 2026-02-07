#include "luxlib.hpp"
#include "engine_module.hpp"
#include "flecs/addons/cpp/entity.hpp"
#include "flecs/addons/cpp/mixins/pipeline/decl.hpp"
#include "modules/game_module.hpp"
#include "modules/physics_module.hpp"
#include "modules/render_module.hpp"
#include "modules/transform_module.hpp"
#include "sokol_time.h"
#include "spdlog/spdlog.h"

GpuTexture load_rgba8_image(std::string path) {
  int width, height, channels;
  stbi_uc *pixels = stbi_load(path.c_str(), &width, &height, &channels, 4);
  sg_image_desc image_desc = {
      .width = width,
      .height = height,
      .pixel_format = SG_PIXELFORMAT_RGBA8,
      .data = {.mip_levels = {
                   {.ptr = pixels, .size = (size_t)(width * height * 4)}}}};
  sg_image image = sg_make_image(image_desc);
  sg_view view = sg_alloc_view();
  sg_init_view(view, {.texture = {.image = image}});
  return {.view = view, .image = image};
}

void Luxlib::init() {
  spdlog::info("starting luxlib...");
  if (initialized) {
    spdlog::critical("more than one lib has been started");
    return;
  }

  initialized = true;

  // Grafics setup
  sg_desc desc = {};
  desc.logger.func = slog_func;
  desc.environment = sglue_environment();
  sg_setup(&desc);

  // Time setup
  stm_setup();
  last_time = stm_now();

  render_server.init();

  // TODO: For some reason this crashes in debug mode
  // world.import <flecs::stats>();
  world.set<flecs::Rest>({});
  world.import <engine_module>();
  world.import <game_module>();

  texture = load_rgba8_image("./assets/placeholder.png");
  texture_circle = load_rgba8_image("./assets/circle.png");

  // TODO: Batch renderer bugs when too many sprites are drawn. Use ImGui to
  // debug
  world.system().kind(flecs::OnUpdate).run([this](flecs::iter &it) {
    it.world()
        .entity()
        .set<cPosition2>({{rand() % 32, 512.0 + rand() % 32}})
        .set<cScale2>({{1.0, 1.0}})
        .set(cSprite{.size = {16.0, 16.0}, .texture = texture_circle})
        .add<cPhysicsBody>()
        .set(cRestitution{.value = 0.1})
        .set(cDensity{.value = 1.0})
        .set(cFriction{.value = 1.0})
        .set(cPhysicsShape{.type = ShapeType::Circle, .size = {0.5, 0.0}});
  });

  world.entity()
      .set(cPosition2{.value = {0.0, -350.0}})
      .set(cRotation2{.value = 0.0})
      .set(cScale2{.value = {1.0, 1.0}})
      .set(cSprite{.size = {2000.0, 32.0}, .texture = texture})
      .add<cPhysicsBody>()
      .set(cPhysicsBodyType::Static)
      .set(cDensity{.value = 1.0})
      .set(cPhysicsShape{.type = ShapeType::Box, .size = {100.0, 1.0}});

  world.entity()
      .set(cPosition2{.value = {600.0, 0.0}})
      .set(cRotation2{.value = 90.0})
      .set(cScale2{.value = {1.0, 1.0}})
      .set(cSprite{.size = {2000.0, 32.0}, .texture = texture})
      .add<cPhysicsBody>()
      .set(cPhysicsBodyType::Static)
      .set(cDensity{.value = 1.0})
      .set(cPhysicsShape{.type = ShapeType::Box, .size = {100.0, 1.0}});

  world.entity()
      .set(cPosition2{.value = {-600.0, -350.0}})
      .set(cRotation2{.value = 90.0})
      .set(cScale2{.value = {1.0, 1.0}})
      .set(cSprite{.size = {2000.0, 32.0}, .texture = texture})
      .add<cPhysicsBody>()
      .set(cPhysicsBodyType::Static)
      .set(cDensity{.value = 1.0})
      .set(cPhysicsShape{.type = ShapeType::Box, .size = {100.0, 1.0}});
}

void Luxlib::frame() {

  // Logic
  float dt = (float)stm_sec(stm_laptime(&last_time));
  world.progress(dt);

  // Render
  sg_pass_action pass = {};
  pass.colors[0] = {.load_action = SG_LOADACTION_CLEAR,
                    .clear_value = {0.1f, 0.1f, 0.1f, 1.0f}};

  sg_begin_pass({.action = pass, .swapchain = sglue_swapchain()});

  render_server.set_camera_zoon(1.0);
  render_server.set_camera_position({0.0, 0.0, -0.1});
  render_server.draw_visuals();

  sg_end_pass();
  sg_commit();
}
