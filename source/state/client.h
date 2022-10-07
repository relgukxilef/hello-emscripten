#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "input.h"

struct client {
    // TODO: maybe this function should not be in this struct
    void update(::input& input);

    glm::vec3 user_position {0, 0, 0};
    float user_pitch = 0, user_yaw = 0;
    glm::quat user_orientation {0, 0, 0, 1};

    glm::vec2 touch_previous_position[input::touch::size];
    int rotation_touch = -1, movement_touch = -1;
};
