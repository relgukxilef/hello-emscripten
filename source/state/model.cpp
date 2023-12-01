#include "model.h"

#include <array>
#include <exception>

#include <boost/json.hpp>
#include <boost/static_string.hpp>

template<std::integral T>
T read(std::ranges::subrange<uint8_t*> &b) {
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

struct parse_exception : public std::exception {
    const char* what() const override {
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
    uint32_t offset, stride;
};

struct accessor {
    uint32_t buffer_view, count, offset;
    component_type type;
};

struct primitive {
    uint32_t positions, normals, indices;
    primitive_mode mode;
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
            }
        } else if (state == state::meshes_n) {
            if (key == "primitives") {
                state = state::meshes_n_primitives;
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
                state == state::meshes
            )
        ) {
            state = state::root;
        }
        if (
            depth == 3 && state == state::meshes_n_primitives
        ) {
            state = state::meshes_n;
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
        } else if (depth == 4 && state == state::meshes_n_primitives) {
            state = state::meshes_n_primitives_n;
            primitives.push_back({});
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
            }
        } else if (depth == 5) {
            if (state == state::meshes_n_primitives_n) {
                state = state::meshes_n_primitives;
            }
        }
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
        //value.append(s);

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
            }
        } else if (depth == 5 && state == state::meshes_n_primitives_n) {
            if (key == "mode") {
                primitives.back().mode = static_cast<primitive_mode>(i);
            } else if (key == "indices") {
                primitives.back().indices = i;
            }
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

    state state = state::root;
    uint32_t depth = 0;
    boost::static_string<128> key;
    boost::static_string<128> value;

    std::vector<buffer_view> buffer_views;
    std::vector<accessor> accessors;
    std::vector<primitive> primitives;
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
}
