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

    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx = 
        emscripten_webgl_create_context("#canvas", &attr);
    emscripten_webgl_make_context_current(ctx);

    // TODO: set resize callback
    int width, height;
    emscripten_webgl_get_drawing_buffer_size(ctx, &width, &height);
    vglSetCurrentSurfaceExtent(
        { static_cast<uint32_t>(width), static_cast<uint32_t>(height) }
    );
}

window::~window() {
}

VkInstance window::get_instance() {
    return vglCreateInstanceForGL();
}

VkSurfaceKHR window::get_surface() {
    return vglCreateSurfaceForGL();
}
