#include "window.h"

#include <vulkan/vulkan.h>
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>

#include <vulkangl/vulkangl.h>

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

VkInstance window::get_instance() {
    return vglCreateInstanceForGL();
}

VkSurfaceKHR window::get_surface() {
    return vglCreateSurfaceForGL();
}
