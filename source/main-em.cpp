#include <memory>

#include <emscripten/emscripten.h>
#include <emscripten/html5.h>

#include <vulkangl/vulkangl.h>

#include "hello.h"

EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context;

std::unique_ptr<hello> h;

void update_canvas_size() {
    double css_width, css_height;
    double pixel_ratio = emscripten_get_device_pixel_ratio();
    emscripten_get_element_css_size("#canvas", &css_width, &css_height);
    emscripten_set_canvas_element_size(
        "#canvas", 
        (int)(css_width * pixel_ratio), (int)(css_height * pixel_ratio)
    );

    int width, height;
    emscripten_webgl_get_drawing_buffer_size(context, &width, &height);
    vglSetCurrentSurfaceExtent(
        { static_cast<uint32_t>(width), static_cast<uint32_t>(height) }
    );
}

EM_BOOL request_animation_frame(double time, void* userData) {
    update_canvas_size();
    
    h->draw(vglCreateInstanceForGL(), vglCreateSurfaceForGL());

    return EM_TRUE;
}

int main() {
    EmscriptenWebGLContextAttributes attr;
    emscripten_webgl_init_context_attributes(&attr);
    attr.antialias = false;

    context = emscripten_webgl_create_context("#canvas", &attr);
    emscripten_webgl_make_context_current(context);

    update_canvas_size();

    h.reset(new hello(vglCreateInstanceForGL(), vglCreateSurfaceForGL()));

    emscripten_request_animation_frame_loop(request_animation_frame, nullptr);

    return 0;
}
