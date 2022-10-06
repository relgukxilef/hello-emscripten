#pragma once

#include <glm/glm.hpp>

struct input {
    glm::vec2 pointer_position;
    glm::vec2 motion, rotation;

    bool pointer_locked;
    bool prefer_pointer_locked;
    bool pointer_lock_unavailable;

    bool fullscreen;
    bool prefer_fullscreen;

    struct touch {
        enum { size = 4, };
        int identifier[size];
        glm::vec2 position[size];
        enum : uint8_t {
            none, start, end,
        } event[size];
    } touch;
};
