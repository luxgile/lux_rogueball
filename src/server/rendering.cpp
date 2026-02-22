#include "rendering.hpp"
#include "../luxlib.hpp"
#include "spdlog/spdlog.h"
#include <vector>

HandleId RenderingServer::get_next_id() {
  if (!free_ids.empty()) {
    HandleId id = free_ids.back();
    free_ids.pop_back();
    return id;
  }

  HandleId id = {next_id};
  next_id += 1;
  return id;
}
void RenderingServer::set_camera_zoom(float zoom) {
  camera.zoom = zoom;
  camera.update_mats();
}

void RenderingServer::set_camera_position(vec3 position) {
  camera.position = position;
  camera.update_mats();
}

HandleId RenderingServer::new_visual2() {
  HandleId id = get_next_id();
  Visual2 visual = {};
  visuals.insert({id, visual});
  return id;
}

Visual2 &RenderingServer::get_visual2(const HandleId &id) {
  return visuals[id];
}

void RenderingServer::delete_visual2(const HandleId &id) {
  visuals.erase(id);
  free_ids.push_back(id);
}

void RenderingServer::init() {
  sg_shader shader = sg_make_shader(unlit2_shader_desc(sg_query_backend()));

  // Create a dynamic vertex buffer
  sg_buffer_desc vbo_desc = {.size = MAX_VERTICES * sizeof(GpuVertex2) *
                                     MAX_BATCHES,
                             .usage = {.dynamic_update = true}};
  vbo = sg_make_buffer(&vbo_desc);
  current_view = sg_alloc_view();

  // Create static index buffer for quads
  const int MAX_INDICES = (MAX_VERTICES / 4) * 6;
  std::vector<uint16_t> indices(MAX_INDICES);
  for (int i = 0, v = 0; i < MAX_INDICES; i += 6, v += 4) {
    indices[i + 0] = v + 0;
    indices[i + 1] = v + 1;
    indices[i + 2] = v + 2;
    indices[i + 3] = v + 0;
    indices[i + 4] = v + 2;
    indices[i + 5] = v + 3;
  }

  sg_buffer_desc buffer_desc = {
      .usage = {.index_buffer = true},
      .data = {.ptr = indices.data(),
               .size = indices.size() * sizeof(uint16_t)}};

  ibo = sg_make_buffer(&buffer_desc);

  sg_sampler_desc sampler_desc = {.min_filter = SG_FILTER_LINEAR,
                                  .mag_filter = SG_FILTER_LINEAR};
  bindings = {.vertex_buffers = {vbo},
              .index_buffer = ibo,
              .samplers = {sg_make_sampler(&sampler_desc)}};

  // Create the Pipeline
  sg_pipeline_desc pip_desc = {.shader = shader,
                               .index_type = SG_INDEXTYPE_UINT16};
  pip_desc.layout.attrs[0].format = SG_VERTEXFORMAT_FLOAT2;
  pip_desc.layout.attrs[1].format = SG_VERTEXFORMAT_FLOAT2;
  pip_desc.layout.attrs[2].format = SG_VERTEXFORMAT_FLOAT4;
  pip_desc.colors[0].blend = {.enabled = true,
                              .src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
                              .dst_factor_rgb =
                                  SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA};
  pip = sg_make_pipeline(&pip_desc);

  camera.zoom = 1.0;
  set_camera_position({0.0, 0.0, -1.0});

  // Create white texture
  uint32_t white_pixel = 0xFFFFFFFF;
  sg_image_desc img_desc = {
      .width = 1,
      .height = 1,
      .data = {
          .mip_levels = {{.ptr = &white_pixel, .size = sizeof(white_pixel)}}}};
  white_texture.image = sg_make_image(&img_desc);
  sg_view_desc view_desc = {.texture = {.image = white_texture.image}};
  white_texture.view = sg_make_view(&view_desc);
}

void RenderingServer::draw_visuals() {
  std::map<uint32_t, std::vector<HandleId>> batch_map;
  for (auto [id, visual] : visuals) {
    batch_map[visual.texture.view.id].push_back(id);
  }

  for (auto [view, ids] : batch_map) {
    for (auto id : ids) {
      auto visual = get_visual2(id);
      queue_visual2(visual);
    }
  }
  flush_visuals2();
}

void RenderingServer::queue_visual2(Visual2 visual) {
  if (visual.texture.view.id == 0) {
    spdlog::warn("Trying to draw with an invalid visual.");
    return;
  }

  float w = visual.size.x / 2.0f;
  float h = visual.size.y / 2.0f;
  glm::vec3 v0 = visual.model * glm::vec3(-w, -h, 1);
  glm::vec3 v1 = visual.model * glm::vec3(w, -h, 1);
  glm::vec3 v2 = visual.model * glm::vec3(w, h, 1);
  glm::vec3 v3 = visual.model * glm::vec3(-w, h, 1);

  push_quad(v0, v1, v2, v3, WHITE, &visual.texture);
}

void RenderingServer::flush_visuals2() {
  if (vertex_buffer.empty())
    return;

  // Upload local RAM buffer to GPU RAM
  int offset = sg_append_buffer(
      vbo, {.ptr = vertex_buffer.data(),
            .size = vertex_buffer.size() * sizeof(GpuVertex2)});
  if (sg_query_buffer_overflow(vbo)) {
    return;
  }

  if (sg_query_buffer_overflow(vbo)) {
    spdlog::error("sokol buffer overflow! increase vbo size.");
    vertex_buffer.clear();
    return;
  }

  sg_apply_pipeline(pip);

  bindings.vertex_buffer_offsets[0] = offset;
  bindings.views[0] = current_view;
  sg_apply_bindings(&bindings);

  auto mvp = camera.proj * camera.view;
  vs_params_t params;
  std::memcpy(&params.mvp, glm::value_ptr(mvp), sizeof(params.mvp));
  auto uniforms = SG_RANGE(params);
  sg_apply_uniforms(0, &uniforms);

  // spdlog::info("vertex buffer size: {}", vertex_buffer.size());
  sg_draw(0, (int)(vertex_buffer.size() / 4) * 6, 1);
  vertex_buffer.clear();
}

void RenderingServer::set_camera_resolution(vec2 size) {
  camera.size = size;
  camera.update_mats();
}

void RenderingServer::draw_line(vec2 p1, vec2 p2, Srgba color,
                                float thickness) {
  vec2 dir = p2 - p1;
  float length = glm::length(dir);
  if (length < 0.0001f)
    return;
  vec2 normal = vec2(-dir.y, dir.x) / length;
  vec2 offset = normal * (thickness * 0.5f);

  vec2 v0 = p1 - offset;
  vec2 v1 = p2 - offset;
  vec2 v2 = p2 + offset;
  vec2 v3 = p1 + offset;

  push_quad(v0, v1, v2, v3, color, nullptr);
}

void RenderingServer::draw_point(vec2 p, Srgba color, float size) {
  draw_rect(p, 0.0f, vec2(size, size), color, true);
}

void RenderingServer::draw_rect(vec2 p, float r, vec2 size, Srgba color,
                                bool filled) {
  vec2 half_size = size * 0.5f;

  // Corner offsets before rotation
  vec2 corners[4] = {
      vec2(-half_size.x, -half_size.y),
      vec2(half_size.x, -half_size.y),
      vec2(half_size.x, half_size.y),
      vec2(-half_size.x, half_size.y),
  };

  // Rotate and translate each corner
  float c = cos(r);
  float s = sin(r);
  for (vec2 &v : corners) {
    v = p + vec2(c * v.x - s * v.y, s * v.x + c * v.y);
  }

  if (filled) {
    push_quad(corners[0], corners[1], corners[2], corners[3], color, nullptr);
  } else {
    draw_line(corners[0], corners[1], color);
    draw_line(corners[1], corners[2], color);
    draw_line(corners[2], corners[3], color);
    draw_line(corners[3], corners[0], color);
  }
}

void RenderingServer::draw_circle(vec2 center, float radius, Srgba color,
                                  int segments) {
  float angle_step = 2.0f * M_PI / segments;
  for (int i = 0; i < segments; i++) {
    float angle1 = i * angle_step;
    float angle2 = (i + 1) * angle_step;
    vec2 p1 = center + vec2(cos(angle1), sin(angle1)) * radius;
    vec2 p2 = center + vec2(cos(angle2), sin(angle2)) * radius;
    draw_line(p1, p2, color);
  }
}
auto RenderingServer::get_camera_zoom() const -> float { return camera.zoom; }

auto RenderingServer::get_camera_resolution() const -> vec2 {
  return camera.size;
}

auto RenderingServer::get_camera_position() const -> vec3 {
  return camera.position;
}
auto RenderingServer::draw_quad(vec2 p1, vec2 p2, vec2 p3, vec2 p4,
                                vec2 position, float rotation, Srgba color,
                                bool filled) -> void {
  float c = cos(rotation);
  float s = sin(rotation);
  p1 = position + vec2(c * p1.x - s * p1.y, s * p1.x + c * p1.y);
  p2 = position + vec2(c * p2.x - s * p2.y, s * p2.x + c * p2.y);
  p3 = position + vec2(c * p3.x - s * p3.y, s * p3.x + c * p3.y);
  p4 = position + vec2(c * p4.x - s * p4.y, s * p4.x + c * p4.y);

  if (filled) {
    push_quad(p1, p2, p3, p4, color, &white_texture);
  } else {
    draw_line(p1, p2, color);
    draw_line(p2, p3, color);
    draw_line(p3, p4, color);
    draw_line(p4, p1, color);
  }
}
