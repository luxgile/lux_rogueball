#include "luxlib.hpp"
#include "sokol_app.h"

void on_init() { Luxlib::instance().init(); }
void on_frame() { Luxlib::instance().frame(); }
void on_input(const sapp_event *event) { Luxlib::instance().input(event); }

sapp_desc sokol_main(int argc, char *argv[]) {
  return (sapp_desc){
      .init_cb = on_init,
      .frame_cb = on_frame,
      .event_cb = on_input,
      .width = 1280,
      .height = 720,
      .window_title = "luxlib",
  };
}
