#include "main-glfw.h"

glm::vec2 deadzone(glm::vec2 stick) {
    float length = glm::length(stick);
    float deadzone = 0.25;
    stick =
        stick / length *
        glm::max(length - deadzone, 0.f) / (1 - deadzone);
    return stick;
}

void update(input& input, GLFWwindow* window, float delta) {
    glm::dvec2 cursor_position;
    glfwGetCursorPos(window, &cursor_position.x, &cursor_position.y);

    input.rotation =
        input.pointer_locked ?
        0.001f * (glm::vec2(cursor_position) - input.pointer_position) :
        glm::vec2(0);
    input.pointer_position = cursor_position;

    input.motion = glm::vec2{
        glfwGetKey(window, GLFW_KEY_A) ? -1 :
        glfwGetKey(window, GLFW_KEY_D) ? 1 : 0,
        glfwGetKey(window, GLFW_KEY_W) ? -1 :
        glfwGetKey(window, GLFW_KEY_S) ? 1 : 0,
    } * delta;

    if (glfwJoystickIsGamepad(GLFW_JOYSTICK_1)) {
        // TODO: maybe separate mouse motion from joystick motion in ::input
        GLFWgamepadstate state;

        if (glfwGetGamepadState(GLFW_JOYSTICK_1, &state)) {
            input.rotation += deadzone({
                state.axes[GLFW_GAMEPAD_AXIS_RIGHT_X],
                state.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y]
            }) * delta;
            input.motion += deadzone({
                state.axes[GLFW_GAMEPAD_AXIS_LEFT_X],
                state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y]
            }) * delta;
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
