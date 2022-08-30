#pragma once

#include <glm/glm.hpp>

struct input {
    glm::vec2 pointer_position;
    glm::vec2 motion, rotation;

    bool pointer_locked;
    bool prefer_pointer_locked;

    bool fullscreen;
    bool prefer_fullscreen;
};
