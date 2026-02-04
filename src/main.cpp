#include "luxlib.hpp"
#include "sokol_app.h"

void on_init() { Luxlib::instance().init(); }
void on_frame() { Luxlib::instance().frame(); }

sapp_desc sokol_main(int argc, char *argv[]) {
  return (sapp_desc){
      .init_cb = on_init,
      .frame_cb = on_frame,
      .width = 1280,
      .height = 720,
      .window_title = "luxlib",
  };
}
