#pragma once

#include <vector>

#include <glm/glm.hpp>

struct instance {
    struct users {
        // can be incomplete
        std::vector<glm::vec3> position;
        std::vector<glm::vec3> color;
        std::vector<unsigned> identification;
    };

    struct nodes {
        std::vector<glm::uvec4> position_depth;
        std::vector<unsigned> user_count;

    };
};