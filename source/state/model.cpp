#include "model.h"

#include <array>
#include <exception>
#include <csetjmp>
#include <bit>

#include <boost/json.hpp>
#include <boost/static_string.hpp>

#include "../utility/math.h"

template<std::integral T>
T read(std::ranges::subrange<uint8_t*> &b) {
    if (b.size() < sizeof(T))
        return 0;
    T v = 0;
    std::array<std::uint8_t, sizeof(T)> bytes;
    for (size_t i = 0; i < sizeof(T); i++)
        // gltf uses little-endian
        bytes[i] = b[i];
    for (size_t i = 0; i < sizeof(T); i++)
        v |= T(bytes[i]) << (i * 8);
    b = {b.begin() + sizeof(T), b.end()};
    return v;
}

template<std::integral T>
void write(std::ranges::subrange<uint8_t*> &b, T value) {
    if (b.size() < sizeof(T))
        return;
    T v = value;
    for (size_t i = 0; i < sizeof(T); i++)
        // assume little-endian
        b[i] = uint8_t(v >> (i * 8));
    b = {b.begin() + sizeof(T), b.end()};
}

struct parse_exception : public std::exception {
    const char* what() const noexcept override {
        return "parse_exception\n";
    }
};

void parse_check(bool correct) {
    if (!correct) {
        throw parse_exception();
    }
}

enum struct state {
    root,
    asset,
    buffers,
    buffers_n,
    buffer_views,
    buffer_views_n,
    nodes,
    nodes_n,
    nodes_n_children,
    nodes_n_rotation,
    nodes_n_scale,
    nodes_n_translation,
    nodes_n_matrix,
    scenes,
    scenes_n,
    scenes_n_nodes,
    accessors,
    accessors_n,
    accessors_n_max,
    accessors_n_min,
    accessors_n_sparse,
    accessors_n_sparse_indices,
    accessors_n_sparse_values,
    meshes,
    meshes_n,
    meshes_n_primitives,
    meshes_n_primitives_n,
    meshes_n_primitives_n_targets,
    meshes_n_primitives_n_targets_n,
    materials,
    materials_n,
    materials_n_pbr_metallic_roughness,
    materials_n_pbr_metallic_roughness_base_color_texture,
    images,
    images_n,
    textures,
    textures_n,
};

enum struct component_type {
    unsigned_byte = 5121,
    unsigned_short = 5123,
    unsigned_int = 5125,
    signed_float = 5126,
};

enum struct primitive_mode {
    points, lines, line_loop, line_strip, triangles, triangle_strip,
    triangle_fan
};

struct buffer_view {
    uint32_t offset, length, stride = 0;
};

struct accessor {
    uint32_t buffer_view, count, offset;
    component_type type;
};

struct primitive_info {
    uint32_t positions, normals, texture_coordinates, indices, material;
    primitive_mode mode;
};

struct material_info {
    uint32_t pbr_metallic_roughness_base_color_texture;
};

struct mesh_info {
    std::vector<uint32_t> primitives;
};

struct node_info {
    glm::mat4 matrix = glm::mat4(1.0);
    std::vector<uint32_t> children;
    uint32_t parent = ~0u;
    uint32_t mesh = ~0u;
};

struct handler {
    static constexpr std::size_t max_array_size = -1;
    static constexpr std::size_t max_object_size = -1;
    static constexpr std::size_t max_string_size = -1;
    static constexpr std::size_t max_key_size = -1;

    bool on_document_begin(boost::system::error_code& ec) { return true; }

    bool on_document_end(boost::system::error_code& ec) {
        return true;
    }

    bool on_array_begin(boost::system::error_code& ec) {
        if (depth == 1 && state == state::root) {
            if (key == "buffers") {
                state = state::buffers;
            } else if (key == "bufferViews") {
                state = state::buffer_views;
            } else if (key == "nodes") {
                state = state::nodes;
            } else if (key == "accessors") {
                state = state::accessors;
            } else if (key == "meshes") {
                state = state::meshes;
            } else if (key == "images") {
                state = state::images;
            } else if (key == "materials") {
                state = state::materials;
            }
        } else if (state == state::meshes_n) {
            if (key == "primitives") {
                state = state::meshes_n_primitives;
            }
        } else if (state == state::nodes_n) {
            if (key == "matrix") {
                state = state::nodes_n_matrix;
                array_index = 0;
            } else if (key == "children") {
                state = state::nodes_n_children;
            }
        }
        key.clear();
        depth++;
        return true;
    }

    bool on_array_end(std::size_t n, boost::system::error_code& ec) {
        if (
            depth == 2 && (
                state == state::buffers ||
                state == state::buffer_views ||
                state == state::nodes ||
                state == state::accessors ||
                state == state::meshes ||
                state == state::images ||
                state == state::materials
            )
        ) {
            state = state::root;
        }
        if (
            depth == 4 && state == state::meshes_n_primitives
        ) {
            state = state::meshes_n;
        }
        if (state == state::nodes_n_matrix) {
            state = state::nodes_n;
        } else if (state == state::nodes_n_children) {
            state = state::nodes_n;
        }
        depth--;
        return true;
    }

    bool on_object_begin(boost::system::error_code& ec) {
        if (depth == 1 && state == state::root) {
            if (key == "asset") {
                state = state::asset;
            } else if (key == "buffers") {
                state = state::buffers;
            }
        } else if (state == state::buffer_views) {
            state = state::buffer_views_n;
            buffer_views.push_back({});
        } else if (state == state::accessors) {
            state = state::accessors_n;
            accessors.push_back({});
        } else if (state == state::meshes) {
            state = state::meshes_n;
            meshes.push_back({});
        } else if (depth == 4 && state == state::meshes_n_primitives) {
            state = state::meshes_n_primitives_n;
            meshes.back().primitives.push_back(primitives.size());
            primitives.push_back({});
        } else if (state == state::images) {
            state = state::images_n;
            images.push_back(0);
        } else if (state == state::materials) {
            state = state::materials_n;
            materials.push_back({});
        } else if (state == state::materials_n) {
            if (key == "pbrMetallicRoughness")
                state = state::materials_n_pbr_metallic_roughness;
        } else if (state == state::materials_n_pbr_metallic_roughness) {
            if (key == "baseColorTexture")
                state = state::
                    materials_n_pbr_metallic_roughness_base_color_texture;
        } else if (state == state::nodes) {
            state = state::nodes_n;
            nodes.push_back({});
        }
        key.clear();
        depth++;
        return true;
    }

    bool on_object_end(std::size_t n, boost::system::error_code& ec) {
        if (
            depth == 2 &&
            state == state::asset
        ) {
            state = state::root;
        } else if (depth == 3) {
            if (state == state::buffer_views_n) {
                state = state::buffer_views;
            } else if (state == state::accessors_n) {
                state = state::accessors;
            } else if (state == state::meshes_n) {
                state = state::meshes;
            } else if (state == state::images_n) {
                state = state::images;
            } else if (state == state::materials_n) {
                state = state::materials;
            } else if (state == state::nodes_n) {
                state = state::nodes;
            }
        } else if (depth == 5) {
            if (state == state::meshes_n_primitives_n) {
                state = state::meshes_n_primitives;
            }
        }
        if (state == state::materials_n_pbr_metallic_roughness)
            state = state::materials_n;
        else if (
            state ==
            state::materials_n_pbr_metallic_roughness_base_color_texture
        )
            state = state::materials_n_pbr_metallic_roughness;
        depth--;
        return true;
    }

    bool on_string_part(
        std::string_view s, std::size_t n, boost::system::error_code& ec
    ) {
        value.append(s);
        return true;
    }

    bool on_string(
        std::string_view s, std::size_t n, boost::system::error_code& ec
    ) {
        value.append(
            s.data(), std::min(s.size(), value.capacity() - value.size())
        );

        value.clear();
        key.clear();
        return true;
    }

    bool on_key_part(
        std::string_view s, std::size_t n, boost::system::error_code& ec
    ) {
        key.append(s);
        return true;
    }

    bool on_key(
        std::string_view s, std::size_t n, boost::system::error_code& ec
    ) {
        key.append(s);
        return true;
    }

    bool on_number_part(std::string_view s, boost::system::error_code& ec) {
        return true;
    }

    bool on_int64(
        int64_t i, std::string_view s, boost::system::error_code& ec
    ) {
        if (state == state::buffer_views_n) {
            if (key == "byteOffset") {
                buffer_views.back().offset = i;
            } else if (key == "byteStride") {
                buffer_views.back().stride = i;
            } else if (key == "byteLength") {
                buffer_views.back().length = i;
            }
        } else if (state == state::accessors_n) {
            if (key == "bufferView") {
                accessors.back().buffer_view = i;
            } else if (key == "byteOffset") {
                accessors.back().offset = i;
            } else if (key == "componentType") {
                accessors.back().type = static_cast<component_type>(i);
            } else if (key == "count") {
                accessors.back().count = i;
            }
        } else if (depth == 6 && state == state::meshes_n_primitives_n) {
            if (key == "POSITION") {
                primitives.back().positions = i;
            } else if (key == "NORMAL") {
                primitives.back().normals = i;
            } else if (key == "TEXCOORD_0") {
                primitives.back().texture_coordinates = i;
            }
        } else if (depth == 5 && state == state::meshes_n_primitives_n) {
            if (key == "mode") {
                primitives.back().mode = static_cast<primitive_mode>(i);
            } else if (key == "indices") {
                primitives.back().indices = i;
            } else if (key == "material") {
                primitives.back().material = i;
            }
        } else if (state == state::images_n) {
            if (key == "bufferView") {
                images.back() = i;
            }
        } else if (
            state ==
            state::materials_n_pbr_metallic_roughness_base_color_texture
        ) {
            if (key == "index") {
                materials.back().pbr_metallic_roughness_base_color_texture = i;
            }
        } else if (state == state::nodes_n && key == "mesh") {
            nodes.back().mesh = i;
        } else if (state == state::nodes_n_children) {
            nodes.back().children.push_back(i);
        }
        key.clear();
        return true;
    }

    bool on_uint64(
        uint64_t u, std::string_view s, boost::system::error_code& ec
    ) {
        key.clear();
        return true;
    }

    bool on_double(
        double d, std::string_view s, boost::system::error_code& ec
    ) {
        if (state == state::nodes_n_matrix && array_index < 16) {
            nodes.back().matrix[array_index / 4][array_index % 4] = d;
            array_index++;
        }
        key.clear();
        return true;
    }

    bool on_bool(bool b, boost::system::error_code& ec) {
        key.clear();
        return true;
    }

    bool on_null(boost::system::error_code& ec) {
        key.clear();
        return true;
    }

    bool on_comment_part(std::string_view s, boost::system::error_code& ec) {
        return true;
    }

    bool on_comment(std::string_view s, boost::system::error_code& ec) {
        return true;
    }

    ::state state = ::state::root;
    uint32_t depth = 0;
    std::string key;
    std::string value;
    uint32_t array_index = 0;

    std::vector<buffer_view> buffer_views;
    std::vector<accessor> accessors;
    std::vector<primitive_info> primitives;
    std::vector<mesh_info> meshes;
    std::vector<material_info> materials;
    std::vector<node_info> nodes;
    std::vector<unsigned> images;
};

model::model(std::ranges::subrange<uint8_t*> file) {
    auto magic = read<uint32_t>(file);
    parse_check(magic == 0x46546C67);
    auto version = read<uint32_t>(file);
    parse_check(version == 2);
    auto length = read<uint32_t>(file);
    parse_check(length >= 12);

    auto json_length = read<uint32_t>(file);
    parse_check(read<uint32_t>(file) == 0x4E4F534A); // chunk type == JSON

    boost::json::basic_parser<handler> json_parser({});
    boost::system::error_code error;
    json_parser.write_some(
        false, reinterpret_cast<char*>(file.data()), json_length, error
    );
    auto &h = json_parser.handler();

    for (auto node_index = 0; node_index < h.nodes.size(); node_index++) {
        auto& node = h.nodes[node_index];
        for (auto child_index : node.children) {
            h.nodes[child_index].parent = node_index;
        }
    }

    file = {file.begin() + round_up(json_length, 4), file.end()};

    if (file.empty())
        // TODO: external data
        return;

    auto binary_length = read<uint32_t>(file);
    parse_check(read<uint32_t>(file) == 0x004E4942); // chunk type == BIN


    for (node_info& node : h.nodes) {
        // TODO: don't duplicate data per node and primitive
        if (node.mesh == ~0u)
            continue;

        glm::mat4 matrix = node.matrix;
        auto parent = node.parent;
        while (parent != ~0u) {
            matrix = h.nodes[parent].matrix * matrix;
            parent = h.nodes[parent].parent;
        }

        for (auto p : h.meshes[node.mesh].primitives) {
            auto start_index = positions.size() / 12;

            primitives.push_back({
                static_cast<uint32_t>(start_index),
                static_cast<uint32_t>(indices.size() / 4),
                h.accessors[h.primitives[p].indices].count,
                h.materials[h.primitives[p].material].
                pbr_metallic_roughness_base_color_texture
            });

            auto a = h.primitives[p].positions;
            auto v = h.accessors[a].buffer_view;
            auto offset = h.accessors[a].offset + h.buffer_views[v].offset;
            parse_check(
                h.buffer_views[v].stride == 0 || h.buffer_views[v].stride == 12
            );
            std::ranges::subrange<uint8_t*> file_range = {
                file.data() + offset, 
                file.begin() + offset + h.accessors[a].count * 12
            };
            auto begin = positions.insert(
                positions.end(),
                file.begin() + offset,
                file.begin() + offset + h.accessors[a].count * 12
            );
            std::ranges::subrange position_range = {
                std::to_address(begin), std::to_address(positions.end())
            };
            for (unsigned i = 0; i < h.accessors[a].count; i++) {
                glm::vec3 position;
                position.x = std::bit_cast<float>(read<uint32_t>(file_range));
                position.y = std::bit_cast<float>(read<uint32_t>(file_range));
                position.z = std::bit_cast<float>(read<uint32_t>(file_range));
                position = matrix * glm::vec4(position, 1.0);
                write(position_range, std::bit_cast<uint32_t>(position.x));
                write(position_range, std::bit_cast<uint32_t>(position.y));
                write(position_range, std::bit_cast<uint32_t>(position.z));
            }

            a = h.primitives[p].normals;
            v = h.accessors[a].buffer_view;
            offset = h.accessors[a].offset + h.buffer_views[v].offset;
            parse_check(
                h.buffer_views[v].stride == 0 || h.buffer_views[v].stride == 12
            );
            normals.insert(
                normals.end(),
                file.begin() + offset,
                file.begin() + offset + h.accessors[a].count * 12
            );

            a = h.primitives[p].texture_coordinates;
            v = h.accessors[a].buffer_view;
            offset = h.accessors[a].offset + h.buffer_views[v].offset;
            parse_check(
                h.buffer_views[v].stride == 0 || h.buffer_views[v].stride == 8
            );
            texture_coordinates.insert(
                texture_coordinates.end(),
                file.begin() + offset,
                file.begin() + offset + h.accessors[a].count * 8
            );

            a = h.primitives[p].indices;
            v = h.accessors[a].buffer_view;
            offset = h.accessors[a].offset + h.buffer_views[v].offset;

            parse_check(h.accessors[a].type == component_type::unsigned_int);
            auto new_indices_begin = indices.size();
            indices.resize(indices.size() + h.accessors[a].count * 4);
            std::ranges::subrange<uint8_t*> new_indices(
                indices.data() + new_indices_begin, 
                indices.data() + indices.size()
            );
            std::ranges::subrange<uint8_t*> old_indices(
                file.data() + offset, file.data() + offset + new_indices.size()
            );

            while (!old_indices.empty()) {
                write<uint32_t>(new_indices, read<uint32_t>(old_indices));
            }
        }
    }

    for (auto i = 0ul; i < h.images.size(); i++) {
        auto v = h.images[i];
        auto offset = h.buffer_views[v].offset;
        auto length = h.buffer_views[v].length;
        uint32_t width, height;
        auto image = read_png(
            {file.data() + offset, file.data() + offset + length},
            width, height
        );
        images.push_back({
            static_cast<uint32_t>(pixels.size()),
            static_cast<uint32_t>(image.size()),
            width, height
        });
        pixels.insert(pixels.end(), image.begin(), image.end());
    }
}
