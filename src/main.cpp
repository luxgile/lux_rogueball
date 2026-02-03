#include "flecs.h"
#include "server/rendering.hpp"
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"
#include "sokol_log.h"
#include "stb/stb_image.h"

sg_image load_rgba8_image(std::string path) {
  int width, height, channels;
  stbi_uc *pixels = stbi_load(path.c_str(), &width, &height, &channels, 4);
  sg_image_desc image_desc = {
      .width = width,
      .height = height,
      .pixel_format = SG_PIXELFORMAT_RGBA8,
      .data = {.mip_levels = {
                   {.ptr = pixels, .size = (size_t)(width * height * 4)}}}};
  return sg_make_image(image_desc);
}

class Game {
public:
  flecs::world world;
  RenderingServer render_server;
  sg_image image;
  sg_image image_circle;

  void init() {
    sg_desc desc = {};
    desc.logger.func = slog_func;
    desc.environment = sglue_environment();
    sg_setup(&desc);

    render_server.init();

    image = load_rgba8_image(
        "/mnt/6f7e372e-8cd1-4f27-980d-5342a70722c5/dev/custom_games/"
        "rogue_ball/assets/placeholder.png");
    image_circle = load_rgba8_image(
        "/mnt/6f7e372e-8cd1-4f27-980d-5342a70722c5/dev/custom_games/"
        "rogue_ball/assets/circle.png");
  }

  void frame() {

    // Logic
    world.progress();

    // Render
    sg_pass_action pass = {};
    pass.colors[0] = {.load_action = SG_LOADACTION_CLEAR,
                      .clear_value = {0.1f, 0.1f, 0.1f, 1.0f}};

    sg_begin_pass({.action = pass, .swapchain = sglue_swapchain()});

    render_server.set_camera_zoon(1.0);
    render_server.set_camera_position({0.0, 0.0, -0.1});
    render_server.draw_sprite(image, 0, 0, 256, 256);
    render_server.draw_sprite(image_circle, 512, 512, 256, 256);
    render_server.flush();

    sg_end_pass();
    sg_commit();
  }
};

static Game *g_game;
void on_init() { g_game->init(); }
void on_frame() { g_game->frame(); }

sapp_desc sokol_main(int argc, char *argv[]) {
  g_game = new Game();
  return (sapp_desc){
      .init_cb = on_init,
      .frame_cb = on_frame,
      .width = 1280,
      .height = 720,
      .window_title = "luxlib",
  };
}
