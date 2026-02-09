#pragma once

#include "../luxlib.hpp"
#include "../server/rendering.hpp"
#include "flecs.h"
#include "glm/ext/vector_float2.hpp"
#include "transform_module.hpp"

struct cSprite {
  std::string path;
  glm::vec2 size;
  GpuTexture texture;
};

struct cVisual2Handle {
  HandleId id;
};

struct render_module {
  render_module(flecs::world &world);
};
