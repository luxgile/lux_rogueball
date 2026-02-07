#pragma once

#include "flecs.h"
#include "flecs/addons/cpp/mixins/meta/decl.hpp"
#include "glm/fwd.hpp"
#include "server/rendering.hpp"
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"
#include "sokol_log.h"
#include "stb/stb_image.h"

GpuTexture load_rgba8_image(std::string path);

class Luxlib {
private:
  bool initialized;
  uint64_t last_time = 0;

  Luxlib() : initialized(false) {}

public:
  RenderingServer render_server;
  flecs::world world;
  GpuTexture texture;
  GpuTexture texture_circle;

  Luxlib(const Luxlib &) = delete;
  void operator=(const Luxlib &) = delete;

  static Luxlib &instance() {
    static Luxlib instance;
    return instance;
  }

  void init();

  void frame();
};
