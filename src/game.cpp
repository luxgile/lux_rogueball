#include "game.hpp"
#include "engine_module.hpp"
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
  if (initialized) {
    spdlog::critical("more than one lib has been initialized");
    return;
  }

  initialized = true;

  sg_desc desc = {};
  desc.logger.func = slog_func;
  desc.environment = sglue_environment();
  sg_setup(&desc);

  render_server.init();

  // world.import <flecs::stats>();
  world.set<flecs::Rest>({});
  world.import <engine_module>();

  texture = load_rgba8_image("./assets/placeholder.png");
  texture_circle = load_rgba8_image("./assets/circle.png");

  world.entity()
      .add<Visual2Handle>()
      .set<Position2>({{0.0, 0.0}})
      .set(Sprite{.size = {512.0, 512.0}, .texture = texture});

  world.entity()
      .add<Visual2Handle>()
      .set<Position2>({{512.0, 0.0}})
      .set(Sprite{.size = {512.0, 512.0}, .texture = texture_circle});
}

void Luxlib::frame() {

  // Logic
  world.progress();

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
