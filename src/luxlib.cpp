#include "luxlib.hpp"
#include "engine_module.hpp"
#include "flecs/addons/cpp/entity.hpp"
#include "flecs/addons/cpp/mixins/pipeline/decl.hpp"
#include "game/game_module.hpp"
#include "glm/ext/vector_float2.hpp"
#include "imgui.h"
#include "modules/physics_module.hpp"
#include "modules/render_module.hpp"
#include "modules/transform_module.hpp"
#include "server/rendering.hpp"
#include "sokol_imgui.h"
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

  // Imgui setup
  simgui_desc_t simgui_desc = {};
  simgui_desc.logger.func = slog_func;
  simgui_setup(&simgui_desc);
  ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

  render_server.init();

  // TODO: For some reason this crashes in debug mode
  // world.import <flecs::stats>();
  world.set<flecs::Rest>({});
  world.import <engine_module>();
  world.import <game_module>();

  world.system<cSprite>().kind(flecs::OnLoad).each([](cSprite &sprite) {
    if (sprite.texture.view.id != 0)
      return;

    sprite.texture = load_rgba8_image(sprite.path);
  });
}

void Luxlib::frame() {

  // Logic
  float dt = (float)stm_sec(stm_laptime(&last_time));

  auto size = world.get<sWindowSize>();
  simgui_new_frame({size.width, size.height, dt, sapp_dpi_scale()});
  world.progress(dt);

  // Render
  sg_pass_action pass = {};
  pass.colors[0] = {.load_action = SG_LOADACTION_CLEAR,
                    .clear_value = {0.1f, 0.1f, 0.1f, 1.0f}};

  sg_begin_pass({.action = pass, .swapchain = sglue_swapchain()});

  render_server.draw_visuals();

  simgui_render();

  sg_end_pass();
  sg_commit();
}

void Luxlib::input(const sapp_event *event) {
  simgui_handle_event(event);

  if (event->type == SAPP_EVENTTYPE_KEY_DOWN) {
    if (event->key_code == SAPP_KEYCODE_ESCAPE) {
      sapp_request_quit();
    }
  }

  if (event->type == SAPP_EVENTTYPE_RESIZED) {
    world.set(sWindowSize{.width = event->window_width,
                          .height = event->window_height});
  }

  auto input = world.try_get_mut<sInputState>();
  if (!input)
    return;

  // Mouse
  if (event->type == SAPP_EVENTTYPE_MOUSE_MOVE) {
    input->mouse_viewport_position = {event->mouse_x, event->mouse_y};
  }

  if (event->type == SAPP_EVENTTYPE_MOUSE_DOWN) {
    input->pressed_mouse[event->mouse_button] = true;
  }

  if (event->type == SAPP_EVENTTYPE_MOUSE_UP) {
    input->pressed_mouse[event->mouse_button] = false;
  }

  // Keyboard
  if (event->type == SAPP_EVENTTYPE_KEY_DOWN) {
    input->pressed_keys.insert({event->key_code, true});
  }

  if (event->type == SAPP_EVENTTYPE_KEY_UP) {
    input->pressed_keys.insert({event->key_code, false});
  }
}
