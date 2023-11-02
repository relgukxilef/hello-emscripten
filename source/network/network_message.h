#pragma once

#include <cinttypes>
#include <memory>
#include <span>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "../utility/unique_span.h"

struct uuid {
    std::uint64_t values[2];
};

namespace extensions {
    const uuid pose {0x4e166f0d6e684580, 0x865ba55945a27e83};
    const uuid skeleton {0x3063add9a11f474c, 0xb51447791e76e2d3};
    const uuid blend_weights {0x863a5dbecfa44c04, 0x9c67927d76cad749};
    const uuid voice {0xe91bb0333e5144b6, 0xb1437c96dec30d8c};
    const uuid chat {0x2804563ac10e4fe6, 0x8d7f058b0566546f};
    const uuid avatar {0x99155c895990459c, 0xa0798a1d9b68fe1b};
    const uuid video {0x6e3ac3cdb6a8459c, 0xa2503fc0914477b9};
};

struct initial_message {
    initial_message(unsigned extension_capacity);

    std::uint16_t size;
    unique_span<uuid> extensions;
};

struct message {
    message() = default;
    message(unsigned user_capacity, unsigned audio_capacity);

    struct {
        std::uint16_t size;
        struct {
            unique_span<float> x, y, z;
        } position;
        struct {
            unique_span<float> x, y, z, w;
        } orientation;
        unique_span<std::uint16_t> audio_size;
        unique_span<std::uint8_t> audio;
    } users;
};

void write(initial_message &m, std::span<std::uint8_t> b);
void read(initial_message &m, std::span<std::uint8_t> b);
std::size_t capacity(initial_message &m);

void write(message &m, std::span<std::uint8_t> b);
void read(message &m, std::span<std::uint8_t> b);
std::size_t capacity(message &m);
