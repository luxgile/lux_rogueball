#include "debug_module.hpp"

#include "../modules/input_module.hpp"
#include "imgui.h"

debug_module::debug_module(flecs::world &world) {
  world.module<debug_module>();

  world.system<sInputState>("Debug Input")
      .each([](flecs::iter &it, size_t, sInputState &input) {
        ImGui::Begin("Input");

        if (ImGui::CollapsingHeader("Mouse")) {
          ImGui::Text("Mouse position: %f, %f", input.mouse_viewport_position.x,
                      input.mouse_viewport_position.y);

          ImGui::Text("Scroll: %f", input.mouse_scroll);

          for (auto &[key, pressed] : input.pressed_mouse) {
            ImGui::Text("Mouse %i: %s", key, pressed ? "pressed" : "released");
          }
        }

        if (ImGui::CollapsingHeader("Keyboard")) {
          for (auto &[key, pressed] : input.pressed_keys) {
            ImGui::Text("Key %i: %s", key, pressed ? "pressed" : "released");
          }
        }

        ImGui::End();
      });
}
