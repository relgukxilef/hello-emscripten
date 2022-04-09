#include "window.h"

#include <vulkan/vulkan.h>
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>

struct dummy_surface {} global_surface;

struct window_internal {
};

window::window() {
    EmscriptenWebGLContextAttributes attr;
    emscripten_webgl_init_context_attributes(&attr);

    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx = emscripten_webgl_create_context("#canvas", &attr);
    emscripten_webgl_make_context_current(ctx);
}

window::~window() {
}

VkSurfaceKHR window::get_surface() {
    return (VkSurfaceKHR)&global_surface;
}