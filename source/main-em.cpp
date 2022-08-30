#include <memory>
#include <cstdio>

#include <emscripten/emscripten.h>
#include <emscripten/html5.h>

#include <vulkangl/vulkangl.h>

#include "hello.h"
#include "state/input.h"

EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context;

std::unique_ptr<hello> h;
::input input;
bool pointer_lock_requested = false;

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

EM_BOOL mouse_down(
    int eventType, const EmscriptenMouseEvent *mouseEvent, void *userData
) {
    if (input.prefer_pointer_locked != input.pointer_locked) {
        if (input.prefer_pointer_locked) {
            if (!pointer_lock_requested) {
                std::printf("Requesting pointer lock\n");
                // TODO: should be done in a click listener instead
                emscripten_request_pointerlock("#canvas", false);
                pointer_lock_requested = true;
            }
        } else {
            std::printf("Exiting pointer lock\n");
            emscripten_exit_pointerlock();
        }
    }
    if (!input.pointer_locked) {
        pointer_lock_requested = false;
    }
    return EM_TRUE;
}

EM_BOOL mouse_move(
    int eventType, const EmscriptenMouseEvent *mouseEvent, void *userData
) {
    if (input.pointer_locked)
        input.rotation = { mouseEvent->movementX, mouseEvent->movementY };
    else
        input.rotation = {};
    input.rotation *= 0.1f;
    return EM_TRUE;
}

EM_BOOL request_animation_frame(double time, void* userData) {
    update_canvas_size();

    EmscriptenPointerlockChangeEvent pointer_lock_status;
    emscripten_get_pointerlock_status(&pointer_lock_status);
    input.pointer_locked = pointer_lock_status.isActive;

    h->update(input);
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

    emscripten_set_mousedown_callback("#canvas", nullptr, true, mouse_down);
    emscripten_set_mousemove_callback("#canvas", nullptr, true, mouse_move);

    emscripten_request_animation_frame_loop(request_animation_frame, nullptr);

    return 0;
}
