#define SOKOL_IMPL
#define SOKOL_DEBUG

#if defined(_WIN32)
#define SOKOL_D3D11
#elif defined(__APPLE__)
#define SOKOL_METAL
#else
#define SOKOL_GLCORE
#endif

#include "imgui.h"
#include "sokol_app.h"
#include "sokol_gfx.h"
#include "sokol_glue.h"
#include "sokol_imgui.h"
#include "sokol_log.h"
#include "sokol_time.h"

#define SOKOL_GL_IMPL
#include "sokol_gl.h"

#define FONTSTASH_IMPLEMENTATION
#if defined(_MSC_VER )
#pragma warning(disable:4996) // strncpy use in fontstash.h
#endif
extern "C" {
    #include "fontstash.h"
}
#define SOKOL_FONTSTASH_IMPL
#include "sokol_fontstash.h"
