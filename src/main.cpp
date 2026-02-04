#include "flecs.h"
#include "flecs/addons/cpp/c_types.hpp"
#include "flecs/addons/cpp/mixins/rest/decl.hpp"
#include "flecs/addons/cpp/mixins/stats/decl.hpp"
#include "glm/ext/vector_float2.hpp"
#include "server/rendering.hpp"
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"
#include "sokol_log.h"
#include "stb/stb_image.h"
#include <iterator>

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

struct Position2 {
  glm::vec2 value;
};

struct Rotation2 {
  float value;
};

struct Scale2 {
  glm::vec2 value;
};

struct Sprite {
  vec2 size;
  GpuTexture texture;
};

struct Visual2Handle {
  HandleId id;
};

class Game {
public:
  flecs::world world;
  RenderingServer render_server;
  GpuTexture texture;
  GpuTexture texture_circle;

  void init() {
    sg_desc desc = {};
    desc.logger.func = slog_func;
    desc.environment = sglue_environment();
    sg_setup(&desc);

    render_server.init();

    texture = load_rgba8_image(
        "/mnt/6f7e372e-8cd1-4f27-980d-5342a70722c5/dev/custom_games/"
        "rogue_ball/assets/placeholder.png");
    texture_circle = load_rgba8_image(
        "/mnt/6f7e372e-8cd1-4f27-980d-5342a70722c5/dev/custom_games/"
        "rogue_ball/assets/circle.png");

    // world.import <flecs::stats>();
    world.set<flecs::Rest>({});

    world.observer<Visual2Handle>()
        .event(flecs::OnAdd)
        .each([this](Visual2Handle &handle) {
          handle.id = render_server.new_visual2();
          std::cout << "added visual handle " << handle.id << std::endl;
        });

    world.observer<Visual2Handle>()
        .event(flecs::OnRemove)
        .each([this](Visual2Handle &handle) {
          render_server.delete_visual2(handle.id);
          std::cout << "removed visual handle " << handle.id << std::endl;
        });

    world.system<Visual2Handle, const Sprite, const Position2>("Update Visual2")
        .each([this](Visual2Handle &handle, const Sprite &sprite,
                     const Position2 &pos) {
          auto &visual = render_server.get_visual2(handle.id);
          visual.position = pos.value;
          visual.size = sprite.size;
          visual.texture = sprite.texture;
        });

    world.entity()
        .add<Visual2Handle>()
        .set<Position2>({{0.0, 0.0}})
        .set(Sprite{.size = {32.0, 32.0}, .texture = texture});
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
    render_server.draw_visuals();

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
