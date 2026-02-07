#pragma once

#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float2.hpp"
#include "glm/fwd.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "sokol_gfx.h"
#include "sokol_log.h"
#include "spdlog/spdlog.h"
#include "stb/stb_image.h"
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <map>
#include <unordered_map>
#include <vector>

#include "../shaders/unlit2.glsl.h"

using namespace glm;

struct Srgba {
  float r;
  float g;
  float b;
  float a;
};
static const Srgba WHITE = {1.0, 1.0, 1.0, 1.0};

struct GpuVertex2 {
  vec2 position;
  vec2 uv;
  Srgba color;
};

struct GpuTexture {
  sg_view view;
  sg_image image;
};

struct CameraData {
  vec3 position;
  float zoom;

  mat4 view;
  mat4 proj;

  void update_mats() {
    vec2 center = vec2{1280.0, 720.0} / 2.0f;
    view = glm::mat4(1.0);
    view = glm::translate(view, vec3{center.x, center.y, 0.0f});
    view = glm::scale(view, {zoom, zoom, 1.0f});
    view = glm::translate(view, position);
    proj = glm::ortho(0.0, 1280.0, 0.0, 720.0, -1.0, 1.0);
  }
};

typedef uint32_t HandleId;

struct Visual2 {
  mat3 model;
  vec2 size;
  GpuTexture texture;
};

class RenderingServer {
private:
  sg_pipeline pip;
  sg_bindings bindings;
  sg_buffer vbo;
  sg_buffer ibo;
  CameraData camera;

  std::map<HandleId, Visual2> visuals;
  std::vector<HandleId> free_ids;
  uint32_t next_id;

  std::vector<GpuVertex2> vertex_buffer;
  sg_view current_view;

  const int MAX_VERTICES = 10000;
  const int MAX_BATCHES = 20;

  HandleId get_next_id();

public:
  void set_camera_zoon(float zoom);

  void set_camera_position(vec3 position);

  HandleId new_visual2();

  Visual2 &get_visual2(const HandleId &id);

  void delete_visual2(const HandleId &id);

  void init();

  void draw_visuals();

  void queue_visual2(Visual2 visual);

  void flush_visuals2();
};
