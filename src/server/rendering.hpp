#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float2.hpp"
#include "glm/fwd.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "sokol_gfx.h"
#include "sokol_log.h"
#include "stb/stb_image.h"
#include <cstddef>
#include <iostream>
#include <vector>

#include "../shaders/unlit2.glsl.h"

using namespace glm;
using HandleId = uint32_t;

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

struct CameraData {
  vec3 position;
  float zoom;

  mat4 view;
  mat4 proj;

  void update_mats() {
    view = glm::translate(glm::scale(glm::mat4(1.0), {zoom, zoom, 1.0}),
                          {position});
    proj = glm::ortho(0.0, 1280.0, 0.0, 720.0, -1.0, 1.0);
  }
};

class RenderingServer {
private:
  sg_pipeline pip;
  sg_bindings bindings;
  sg_buffer vbo;
  sg_buffer ibo;
  CameraData camera;

  std::vector<GpuVertex2> vertex_buffer;
  sg_view current_texture;
  sg_image current_image;

  const int MAX_VERTICES = 10000;
  const int MAX_BATCHES = 20;

public:
  void set_camera_zoon(float zoom) {
    camera.zoom = zoom;
    camera.update_mats();
  }

  void set_camera_position(vec3 position) {
    camera.position = position;
    camera.update_mats();
  }

  void init() {
    sg_shader shader = sg_make_shader(unlit2_shader_desc(sg_query_backend()));

    // Create a dynamic vertex buffer
    sg_buffer_desc vbo_desc = {.size = MAX_VERTICES * sizeof(GpuVertex2) *
                                       MAX_BATCHES,
                               .usage = {.dynamic_update = true}};
    vbo = sg_make_buffer(&vbo_desc);

    // Create static index buffer for quads
    uint16_t indices[6000];
    for (int i = 0, v = 0; i < 6000; i += 6, v += 4) {
      indices[i + 0] = v + 0;
      indices[i + 1] = v + 1;
      indices[i + 2] = v + 2;
      indices[i + 3] = v + 0;
      indices[i + 4] = v + 2;
      indices[i + 5] = v + 3;
    }

    sg_buffer_desc buffer_desc = {.usage = {.index_buffer = true},
                                  .data = SG_RANGE(indices)};
    ibo = sg_make_buffer(&buffer_desc);

    sg_sampler_desc sampler_desc = {.min_filter = SG_FILTER_LINEAR,
                                    .mag_filter = SG_FILTER_LINEAR};
    bindings = {.vertex_buffers = {vbo},
                .index_buffer = ibo,
                .samplers = {sg_make_sampler(&sampler_desc)}};

    current_texture = sg_alloc_view();

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

  void draw_sprite(sg_image image, float x, float y, float w, float h) {
    // If texture changes or buffer full, flush to GPU
    if (image.id != current_image.id ||
        vertex_buffer.size() + 4 >= MAX_VERTICES) {
      flush();
      current_image = image;
      sg_uninit_view(current_texture);
      sg_init_view(current_texture, {.texture = {.image = current_image}});
    }

    vertex_buffer.push_back({{x, y}, {0, 0}, WHITE});
    vertex_buffer.push_back({{x + w, y}, {1, 0}, WHITE});
    vertex_buffer.push_back({{x + w, y + h}, {1, 1}, WHITE});
    vertex_buffer.push_back({{x, y + h}, {0, 1}, WHITE});
  }

  void flush() {
    if (vertex_buffer.empty())
      return;

    // Upload local RAM buffer to GPU RAM
    int offset = sg_append_buffer(
        vbo, {.ptr = vertex_buffer.data(),
              .size = vertex_buffer.size() * sizeof(GpuVertex2)});
    if (sg_query_buffer_overflow(vbo)) {
      std::cout << "buffer ovewflow on vbo" << std::endl;
      return;
    }

    sg_apply_pipeline(pip);

		bindings.vertex_buffer_offsets[0] = offset;
    bindings.views[0] = current_texture;
    sg_apply_bindings(&bindings);

    auto mvp = camera.proj * camera.view;
    vs_params_t params;
    std::memcpy(&params.mvp, glm::value_ptr(mvp), sizeof(params.mvp));
    auto uniforms = SG_RANGE(params);
    sg_apply_uniforms(0, &uniforms);

    sg_draw(0, (vertex_buffer.size() / 4) * 6, 1);
    vertex_buffer.clear();
  }
};
