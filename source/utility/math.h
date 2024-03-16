#pragma once

#include <cstdint>

inline uint32_t round_up(uint32_t x, uint32_t divisor) {
    return (x + divisor - 1) / divisor * divisor;
}
