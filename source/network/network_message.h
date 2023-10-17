#pragma once

#include <cinttypes>
#include <memory>
#include <span>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

template<class T>
struct buffer {
    buffer() = default;
    buffer(unsigned capacity) :
        values(std::make_unique<T[]>(capacity)), capacity(capacity)
    {}
    T &operator[](size_t index) {
        return values[index];
    }
    std::unique_ptr<T[]> values;
    unsigned capacity = 0;
};

struct uuid {
    std::uint64_t values[2];
};

namespace extensions {
    const uuid position {0x4e166f0d6e684580, 0x865ba55945a27e83};
    const uuid skeleton {0x3063add9a11f474c, 0xb51447791e76e2d3};
    const uuid blend_weights {0x863a5dbecfa44c04, 0x9c67927d76cad749};
    const uuid voice {0xe91bb0333e5144b6, 0xb1437c96dec30d8c};
    const uuid chat {0x2804563ac10e4fe6, 0x8d7f058b0566546f};
    const uuid avatar {0x99155c895990459c, 0xa0798a1d9b68fe1b};
    const uuid video {0x6e3ac3cdb6a8459c, 0xa2503fc0914477b9};
};

template<std::integral T, unsigned mantissa_bits>
struct fixed_precision {
    float value;
};

struct message {
    message() = default;
    message(unsigned user_capacity, unsigned audio_capacity);

    static const std::int32_t position_scale = 1 << 16;

    struct {
        std::uint16_t size;
        struct {
            buffer<fixed_precision<std::int32_t, 16>> x, y, z;
        } position;
        struct {
            buffer<fixed_precision<std::int16_t, 15>> x, y, z, w;
        } orientation;
        buffer<std::uint16_t> audio_size;
        buffer<std::uint8_t> audio;
    } users;
};

void write(message &m, std::span<std::uint8_t> b);
void read(message &m, std::span<std::uint8_t> b);
std::size_t capacity(message &m);
