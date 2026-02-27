#include "render_module.hpp"
#include "../engine_module.hpp"
#include "flecs/addons/cpp/c_types.hpp"
#include "flecs/addons/cpp/component.hpp"
#include "flecs/addons/cpp/mixins/pipeline/decl.hpp"
#include "glm/ext/vector_float2.hpp"
#include "transform_module.hpp"

render_module::render_module(flecs::world &world) {
  auto &render_server = Luxlib::instance().render_server;

  world.module<render_module>();

  world.component<cCamera>().member<float>("zoom");

  world.component<cMainCamera>();

  world.component<cSprite>()
      .member<std::string>("path")
      .member<glm::vec2>("size")
      .add(flecs::With, world.component<cVisual2Handle>())
      .add(flecs::With, world.component<cWorldTransform2>());

  world.component<cLabel>().member<std::string>("text").member<float>("size");
  world.component<Srgba>()
      .member<float>("r")
      .member<float>("g")
      .member<float>("b")
      .member<float>("a");
  world.component<cTint>().member<Srgba>("color");

  world.component<cVisual2Handle>().member<HandleId>("id");

  world.observer<cVisual2Handle>()
      .event(flecs::OnAdd)
      .each([&render_server](cVisual2Handle &handle) {
        handle.id = render_server.new_visual2();
      });

  world.observer<cVisual2Handle>()
      .event(flecs::OnRemove)
      .each([&render_server](cVisual2Handle &handle) {
        render_server.delete_visual2(handle.id);
      });

  world.observer<const cSprite, cVisual2Handle>()
      .event(flecs::OnSet)
      .each([&render_server](const cSprite &sprite, cVisual2Handle &handle) {
        auto &visual = render_server.get_visual2(handle.id);
        visual.size = sprite.size;
        visual.texture = sprite.texture;
      });

  world
      .system<cVisual2Handle, const cSprite, const cWorldTransform2>(
          "Update Visual2")
      .kind(flecs::PreStore)
      .each([&render_server](cVisual2Handle &handle, const cSprite sprite,
                             const cWorldTransform2 &xform) {
        auto &visual = render_server.get_visual2(handle.id);
        visual.model = xform.model;
        visual.size = sprite.size;
        visual.texture = sprite.texture;
      });

  world.observer<const cCamera>()
      .with<cMainCamera>()
      .event(flecs::OnSet)
      .each([&render_server](const cCamera &camera) {
        render_server.set_camera_zoom(camera.zoom);
      });

  world.system<const cPosition2>("Sync camera position")
      .with<cCamera>()
      .with<cMainCamera>()
      .kind(flecs::PreStore)
      .each([&render_server](const cPosition2 &pos) {
        render_server.set_camera_position(glm::vec3(pos.value, 0.0f));
      });

  world.system<const cLabel, const cPosition2, cTint *>("Draw text")
      .kind(flecs::PostUpdate)
      .each([](const cLabel &label, const cPosition2 &pos, const cTint *tint) {
        auto color = tint ? tint->color : WHITE;
        Luxlib::instance().render_server.draw_text(
            pos.value.x, pos.value.y, label.text.c_str(), label.size, color);
      });
}
