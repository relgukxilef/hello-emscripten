#include "main-glfw.h"

void update(input& input, GLFWwindow* window) {
    glm::dvec2 cursor_position;
    glfwGetCursorPos(window, &cursor_position.x, &cursor_position.y);

    input.rotation =
        input.pointer_locked ?
        0.1f * (glm::vec2(cursor_position) - input.pointer_position) :
        glm::vec2(0);
    input.pointer_position = cursor_position;

    if (glfwJoystickIsGamepad(GLFW_JOYSTICK_1)) {
        // TODO: maybe separate mouse motion from joystick motion in ::input
        GLFWgamepadstate state;

        if (glfwGetGamepadState(GLFW_JOYSTICK_1, &state)) {
            glm::vec2 stick {
                state.axes[GLFW_GAMEPAD_AXIS_RIGHT_X],
                state.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y]
            };
            float length = glm::length(stick);
            float deadzone = 0.25;
            stick =
                stick / length *
                glm::max(length - deadzone, 0.f) / (1 - deadzone);
            // TODO: multiply with delta time
            input.rotation += stick;
        }
    }

    if (input.prefer_pointer_locked != input.pointer_locked) {
        if (input.prefer_pointer_locked) {
            glfwSetInputMode(
                window, GLFW_CURSOR, GLFW_CURSOR_DISABLED
            );
        } else {
            glfwSetInputMode(
                window, GLFW_CURSOR, GLFW_CURSOR_NORMAL
            );
        }
        input.pointer_locked = input.prefer_pointer_locked;
    }
}
