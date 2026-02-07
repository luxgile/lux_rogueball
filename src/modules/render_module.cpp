#include "render_module.hpp"
#include "flecs/addons/cpp/component.hpp"
#include "glm/ext/vector_float2.hpp"
#include "transform_module.hpp"

render_module::render_module(flecs::world &world) {
  auto &render_server = Luxlib::instance().render_server;

  world.module<render_module>();
  world.component<cSprite>()
      .member<glm::vec2>("size")
      .add(flecs::With, world.component<cVisual2Handle>())
      .add(flecs::With, world.component<cWorldTransform2>());

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
}
