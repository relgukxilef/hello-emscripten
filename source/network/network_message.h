#pragma once

#include <cinttypes>
#include <span>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

struct message_header {
    struct {
        std::uint16_t size;
    } users;
};

struct message_data {
    struct {
        std::span<glm::vec3> position;
        std::span<glm::quat> orientation;
    } users;
};

std::size_t message_size(message_header header);
message_data parse_message(std::span<std::uint8_t> buffer);
