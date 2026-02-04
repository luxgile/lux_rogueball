#include "render_module.hpp"
#include "transform_module.hpp"

render_module::render_module(flecs::world &world) {
  auto &render_server = Luxlib::instance().render_server;

  world.module<render_module>();
  world.observer<Visual2Handle>()
      .event(flecs::OnAdd)
      .each([&render_server](Visual2Handle &handle) {
        handle.id = render_server.new_visual2();
      });

  world.observer<Visual2Handle>()
      .event(flecs::OnRemove)
      .each([&render_server](Visual2Handle &handle) {
        render_server.delete_visual2(handle.id);
      });

  world
      .system<Visual2Handle, const Sprite, const WorldTransform2>(
          "Update Visual2")
      .kind(flecs::PreStore)
      .each([&render_server](Visual2Handle &handle, const Sprite sprite,
                             const WorldTransform2 &xform) {
        auto &visual = render_server.get_visual2(handle.id);
        visual.model = xform.model;
        visual.size = sprite.size;
        visual.texture = sprite.texture;
      });
}
