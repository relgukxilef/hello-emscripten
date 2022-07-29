#include <memory>

#include <emscripten.h>
#include <emscripten/html5.h>

#include <vulkangl/vulkangl.h>
#include "hello.h"

EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context;

std::unique_ptr<hello> h;

EM_BOOL request_animation_frame(double time, void* userData) {
    int width, height;
    
    emscripten_webgl_get_drawing_buffer_size(context, &width, &height);
    vglSetCurrentSurfaceExtent(
        { static_cast<uint32_t>(width), static_cast<uint32_t>(height) }
    );
    h->draw();

    return EM_TRUE;
}

int main() {
    EmscriptenWebGLContextAttributes attr;
    emscripten_webgl_init_context_attributes(&attr);

    context = emscripten_webgl_create_context("#canvas", &attr);
    emscripten_webgl_make_context_current(context);

    h.reset(new hello(vglCreateInstanceForGL(), vglCreateSurfaceForGL()));

    emscripten_request_animation_frame_loop(request_animation_frame, 0);

    return 0;
}
