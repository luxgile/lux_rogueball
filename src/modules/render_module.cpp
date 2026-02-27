#include "render_module.hpp"
#include "../engine_module.hpp"
#include "flecs/addons/cpp/c_types.hpp"
#include "flecs/addons/cpp/component.hpp"
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

  world.system<const sWindowSize>("Draw HUD")
      .kind(flecs::PostUpdate)
      .each([&render_server, &world](const sWindowSize &window) {
        render_server.draw_text(10, 30, "ROGUE BALL", 32.0f, WHITE);

        // Draw all entities with a cText component
        // TODO: Move this to a  separate system
        world.query<const cText>().each([&render_server](const cText &t) {
          render_server.draw_text(t.position.x, t.position.y, t.text.c_str(),
                                  t.size, t.color);
        });
      });
}
