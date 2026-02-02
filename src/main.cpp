#include "flecs.h"
#include "server/rendering.hpp"
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"
#include "sokol_log.h"
#include "stb/stb_image.h"


class Game {
public:
  flecs::world world;
  RenderingServer render_server;
  sg_image image;

  void init() {
    sg_desc desc = {};
    desc.logger.func = slog_func;
    desc.environment = sglue_environment();
    sg_setup(&desc);

    render_server.init();

    // sg_image_desc image_desc = {.width = 4, .height = 4, .pixel_format =
    // SG_PIXELFORMAT_RGBA8}; image = sg_make_image(image_desc);
  }

  void frame() {

    // Logic
    world.progress();

    // Render
    sg_pass_action pass = {};
    pass.colors[0] = {.load_action = SG_LOADACTION_CLEAR,
                      .clear_value = {0.1f, 0.1f, 0.1f, 1.0f}};

    sg_begin_pass({.action = pass, .swapchain = sglue_swapchain()});

    render_server.draw_sprite(0, 0, 0.2, 0.2);
    render_server.draw_sprite(-0.3, 0, 0.2, 0.2);
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
      .window_title = "C++ Sokol + Flecs",
  };
}
