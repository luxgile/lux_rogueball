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
  vec2 size;

  mat4 view;
  mat4 proj;

  void update_mats() {
    vec2 center = vec2{size.x, size.y} / 2.0f;
    view = glm::mat4(1.0f);
    view = glm::translate(view, vec3{center.x, center.y, 0.0f});
    view = glm::scale(view, {zoom, zoom, 1.0f});
    view = glm::translate(view, position);
    proj = glm::ortho(0.0f, (float)size.x, 0.0f, (float)size.y, -1.0f, 1.0f);
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
  GpuTexture white_texture;

  std::map<HandleId, Visual2> visuals;
  std::vector<HandleId> free_ids;
  uint32_t next_id;

  std::vector<GpuVertex2> vertex_buffer;
  sg_view current_view;

  const int MAX_VERTICES = 10000;
  const int MAX_BATCHES = 20;

  HandleId get_next_id();

  void push_quad(vec2 v0, vec2 v1, vec2 v2, vec2 v3, Srgba color,
                 GpuTexture *texture) {
    GpuTexture *t = texture ? texture : &white_texture;
    if (t->view.id != current_view.id ||
        vertex_buffer.size() + 4 >= MAX_VERTICES) {
      flush_visuals2();
      current_view = t->view;
    }

    vertex_buffer.push_back({v0, {0, 0}, color});
    vertex_buffer.push_back({v1, {1, 0}, color});
    vertex_buffer.push_back({v2, {1, 1}, color});
    vertex_buffer.push_back({v3, {0, 1}, color});
  }

public:
  auto set_camera_zoom(float zoom) -> void;
  auto get_camera_zoom() const -> float;

  void set_camera_position(vec3 position);
  auto get_camera_position() const -> vec3;

  void set_camera_resolution(vec2 size);
  auto get_camera_resolution() const -> vec2;

  HandleId new_visual2();

  Visual2 &get_visual2(const HandleId &id);

  void delete_visual2(const HandleId &id);

  void init();

  void draw_visuals();

  void queue_visual2(Visual2 visual);

  void flush_visuals2();
  void draw_line(vec2 p1, vec2 p2, Srgba color, float thickness = 1.0f);
  void draw_point(vec2 p, Srgba color, float size = 1.0f);
  void draw_rect(vec2 p, vec2 size, Srgba color, bool filled = false);
  void draw_circle(vec2 center, float radius, Srgba color, int segments = 32);
};
