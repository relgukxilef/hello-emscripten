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

glm::vec2 mouse_rotation;

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
        mouse_rotation = { mouseEvent->movementX, mouseEvent->movementY };

    input.pointer_position = { mouseEvent->canvasX, mouseEvent->canvasY };
    input.rotation *= 0.1f;
    return EM_TRUE;
}

EM_BOOL touch_start(
    int eventType, const EmscriptenTouchEvent *touchEvent, void *userData
) {
    for (int i = 0; i < touchEvent->numTouches; i++) {
        auto touch = touchEvent->touches[i];
        glm::vec2 position = glm::vec2(touch.targetX, touch.targetY);

        for (int j = 0; j < input.touch.size; j++) {
            if (input.touch.identifier[j] == -1) {
                input.touch.identifier[j] = touch.identifier;
                input.touch.position[j] = position;
                input.touch.event[j] = input.touch.start;
                break;
            }
        }
    }

    return EM_TRUE;
}

EM_BOOL touch_move(
    int eventType, const EmscriptenTouchEvent *touchEvent, void *userData
) {
    for (int i = 0; i < touchEvent->numTouches; i++) {
        auto touch = touchEvent->touches[i];
        glm::vec2 position = glm::vec2(touch.targetX, touch.targetY);
        
        for (int j = 0; j < input.touch.size; j++) {
            if (input.touch.identifier[j] == touch.identifier) {
                input.touch.position[j] = position;
            }
        }
    }

    return EM_TRUE;
}

EM_BOOL touch_end(
    int eventType, const EmscriptenTouchEvent *touchEvent, void *userData
) {
    for (int i = 0; i < touchEvent->numTouches; i++) {
        auto touch = touchEvent->touches[i];

        for (int j = 0; j < input.touch.size; j++) {
            if (input.touch.identifier[j] == touch.identifier) {
                input.touch.identifier[j] = -1;
                input.touch.event[j] = input.touch.end;
                break;
            }
        }
    }

    return EM_TRUE;
}

EM_BOOL request_animation_frame(double time, void* userData) {
    update_canvas_size();

    EmscriptenPointerlockChangeEvent pointer_lock_status;
    emscripten_get_pointerlock_status(&pointer_lock_status);
    input.pointer_locked = pointer_lock_status.isActive;

    input.rotation = 0.001f * mouse_rotation;

    mouse_rotation = {};

    h->update(input);
    h->draw(vglCreateInstanceForGL(), vglCreateSurfaceForGL());

    return EM_TRUE;
}

int main() {
    for (int j = 0; j < input.touch.size; j++) {
        input.touch.identifier[j] = -1;
    }

    EmscriptenWebGLContextAttributes attr;
    emscripten_webgl_init_context_attributes(&attr);
    attr.antialias = false;

    context = emscripten_webgl_create_context("#canvas", &attr);
    emscripten_webgl_make_context_current(context);

    update_canvas_size();

    h.reset(new hello(vglCreateInstanceForGL(), vglCreateSurfaceForGL()));

    emscripten_set_mousedown_callback("#canvas", nullptr, true, mouse_down);
    emscripten_set_mousemove_callback("#canvas", nullptr, true, mouse_move);

    emscripten_set_touchstart_callback("#canvas", nullptr, true, touch_start);
    emscripten_set_touchmove_callback("#canvas", nullptr, true, touch_move);
    emscripten_set_touchend_callback("#canvas", nullptr, true, touch_end);
    emscripten_set_touchcancel_callback("#canvas", nullptr, true, touch_end);

    emscripten_request_animation_frame_loop(request_animation_frame, nullptr);

    return 0;
}
