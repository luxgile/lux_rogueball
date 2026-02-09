#include "rendering.hpp"
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
void RenderingServer::set_camera_zoon(float zoom) {
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

  // if (vertex_buffer.size() + 4 >= MAX_VERTICES) {
  //   spdlog::warn("Reached max vertex render batch");
  // }

  // If texture changes or buffer full, flush to GPU
  if (visual.texture.view.id != current_view.id ||
      vertex_buffer.size() + 4 >= MAX_VERTICES) {
    flush_visuals2();
    current_view = visual.texture.view;
  }

  float w = visual.size.x / 2.0f;
  float h = visual.size.y / 2.0f;
  glm::vec3 v0 = visual.model * glm::vec3(-w, -h, 1);
  glm::vec3 v1 = visual.model * glm::vec3(w, -h, 1);
  glm::vec3 v2 = visual.model * glm::vec3(w, h, 1);
  glm::vec3 v3 = visual.model * glm::vec3(-w, h, 1);

  vertex_buffer.push_back({{v0.x, v0.y}, {0, 0}, WHITE});
  vertex_buffer.push_back({{v1.x, v1.y}, {1, 0}, WHITE});
  vertex_buffer.push_back({{v2.x, v2.y}, {1, 1}, WHITE});
  vertex_buffer.push_back({{v3.x, v3.y}, {0, 1}, WHITE});
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
