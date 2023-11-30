#pragma once

#include <cstdint>
#include <ranges>
#include <vector>

/**
 * @brief The model class stores a gltf file with all vertex data converted to
 * standard structure.
 * TOOD: allow partial and streamed parsing and conversion.
 */
struct model {
    model() = default;
    model(std::ranges::subrange<uint8_t*> file);

    // Standard mesh format:
    // positions.xyz*, normals.xyz*,texture_coordinates.uv*,
    // joints.xyzw*,weights.xyzw*
    // indices*

    unsigned
        primitive_count, vertex_count, primitive_offset,
        position_offset, normal_offset, texture_coordinate_offset;

    std::vector<uint8_t> content;
};
