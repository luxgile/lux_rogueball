#include "engine_module.hpp"
#include "imgui.h"
#include "modules/common_module.hpp"
#include "modules/input_module.hpp"
#include "modules/physics_module.hpp"
#include "modules/render_module.hpp"
#include "modules/transform_module.hpp"

engine_module::engine_module(flecs::world &world) {
  world.module<engine_module>();

  world.component<sWindowSize>().member<int>("width").member<int>("height").add(
      flecs::Singleton);

  world.observer<sWindowSize>().event(flecs::OnSet).each([](sWindowSize &size) {
    Luxlib::instance().render_server.set_camera_resolution(
        {size.width, size.height});
  });

  world.set<sWindowSize>({1280, 720});

  world.component<sTime>()
      .member<float>("elapsed")
      .member<float>("delta")
      .member<float>("scale")
      .member<float>("real_elapsed")
      .add(flecs::Singleton);

  world.add<sTime>();

  world.system<sTime>("Update Time")
      .kind(flecs::OnStore)
      .each([](flecs::iter &it, size_t, sTime &time) {
        auto real_dt = it.delta_time();
        time.delta = real_dt * time.scale;
        time.real_delta = real_dt;

        time.elapsed += time.delta;
        time.real_elapsed += time.real_delta;

        ImGui::Begin("General");
        ImGui::Text("Delta: %f", time.delta);
        ImGui::Text("Elapsed: %f", time.elapsed);
        ImGui::Text("Real delta: %f", time.real_delta);
        ImGui::Text("Real elapsed: %f", time.real_elapsed);
        ImGui::Text("Time scale: %f", time.scale);
        ImGui::End();
      });

  world.import <common_module>();
  world.import <transform_module>();
  world.import <render_module>();
  world.import <physics_module>();
  world.import <input_module>();
}
