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
    struct primitive {
        uint32_t vertex_begin;
        uint32_t face_begin, face_size;
        uint32_t image_index;
    };

    struct image {
        uint32_t begin, size;
        uint32_t width, height;
    };

    model() = default;
    model(std::ranges::subrange<uint8_t*> file);

    // Standard mesh format:
    // short positions.xyz*, short normals.xyz*, short texture_coordinates.uv*,
    // short joints.xyzw*, byte weights.xyzw*
    // short indices*
    // Indices are 16 bit. Meshes with more vertices than fit into that, are
    // split up.

    unsigned
        primitive_count, vertex_count, primitive_offset,
        position_offset, normal_offset, texture_coordinate_offset;

    std::vector<uint8_t> positions;
    std::vector<uint8_t> normals;
    std::vector<uint8_t> texture_coordinates;
    std::vector<uint8_t> indices;
    std::vector<uint8_t> pixels;
    std::vector<primitive> primitives;
    std::vector<image> images;
};
