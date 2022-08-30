#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "input.h"

struct client {
    void update(::input& input);

    glm::vec3 user_position;
    float user_pitch = 0, user_yaw = 0;
    glm::quat user_orientation {0, 0, 0, 1};
};
