#pragma once

#include <cstdint>
#include <ranges>
#include <vector>

#include <glm/glm.hpp>

/**
 * @brief The model class stores a gltf file with all vertex data converted to
 * standard structure.
 * TOOD: allow partial and streamed parsing and conversion.
 */
struct model {
    struct node_primitive {
        // represents a pair of a node and a primitive in gltf
        uint32_t vertex_begin;
        uint32_t face_begin, face_size;
        uint32_t image_index;
        glm::mat4 world_matrix = glm::mat4(1.0);
    };

    struct image {
        uint32_t begin, size;
        uint32_t width, height;
    };

    model() = default;
    model(std::ranges::subrange<uint8_t*> file);

    // TODO Standard mesh format:
    // (right now all data is stored in gltf's format)
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
    std::vector<node_primitive> primitives;
    std::vector<image> images;
};

void parse_check(bool correct);

std::vector<uint8_t> read_png(
    std::ranges::subrange<uint8_t*> file,
    unsigned &width, unsigned &height
);
