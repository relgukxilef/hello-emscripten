#include "client.h"

void client::update(::input &input) {
    // TODO: apply i.rotation to user_orientation
    input.rotation *= 0.02f;
    user_pitch = glm::clamp(
        user_pitch + input.rotation.y,
        glm::radians(-90.f), glm::radians(90.f)
    );
    user_yaw += input.rotation.x;
    user_orientation =
        glm::rotate(glm::quat{0, 0, 0, 1}, user_pitch, {1, 0, 0});
    user_orientation =
        glm::rotate(user_orientation, user_yaw, {0, 0, 1});

    input.prefer_pointer_locked = true;
}
