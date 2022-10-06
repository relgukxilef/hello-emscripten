#include "client.h"

void client::update(::input &input) {
    glm::vec2 touch_rotation = {};

    for (int i = 0; i < input.touch.size; i++) {
        if (input.touch.identifier[i] != -1) {
            if (input.touch.event[i] == input.touch.start) {
                // TODO: handle buttons and such
                // TODO: need to know window size
                if (rotation_touch == -1)
                    rotation_touch = i;
            } else if (input.touch.event[i] == input.touch.end) {
                if (rotation_touch == i)
                    rotation_touch = -1;
                input.touch.identifier[i] = -1; // TODO: surprising to caller
            }

            if (
                rotation_touch == i && input.touch.event[i] != input.touch.start
            ) {
                touch_rotation = 
                    input.touch.position[i] - touch_previous_position[i];
            }

            input.touch.event[i] = input.touch.none; // TODO: surprising
        }

        touch_previous_position[i] = input.touch.position[i];
    }
    
    glm::vec2 rotation = input.rotation * 2.0f + touch_rotation * -0.002f;
    user_pitch = glm::clamp(
        user_pitch + rotation.y,
        glm::radians(-90.f), glm::radians(90.f)
    );
    user_yaw += rotation.x;
    user_orientation =
        glm::rotate(glm::quat{0, 0, 0, 1}, user_pitch, {1, 0, 0});
    user_orientation =
        glm::rotate(user_orientation, user_yaw, {0, 0, 1});

    input.motion = input.motion / glm::max(1.0f, glm::length(input.motion));

    user_position +=
        glm::vec3(input.motion * glm::vec2(-1, 1), 0) * user_orientation;

    input.prefer_pointer_locked = true;
}
