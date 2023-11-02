#include "network_message.h"

#include <array>

#include "../utility/serialization.h"

template<class F>
void apply(uuid &m, F f) {
    apply(m.values[0], f);
    apply(m.values[1], f);
}

template<class F>
void apply(initial_message &m, F f) {
    apply(m.size, f);
    apply(m.size, m.extensions, f);
}

template<class F>
void apply(message &m, F f) {
    apply(m.users.size, f);
    auto &p = m.users.position;
    for (auto *v : {&p.x, &p.y, &p.z})
        apply_fixed_point<int16_t, 8, float>(m.users.size, *v, f);
    auto &o = m.users.orientation;
    for (auto *v : {&o.x, &o.y, &o.z, &o.w})
        apply_fixed_point<int16_t, 15, float>(m.users.size, *v, f);
}

initial_message::initial_message(unsigned int extension_capacity) {
    extensions = {extension_capacity};
}

message::message(unsigned user_capacity, unsigned audio_capacity) {
    auto &p = users.position;
    for (auto v : {&p.x, &p.y, &p.z}) {
        *v = {user_capacity};
    }
    auto &o = users.orientation;
    for (auto v : {&o.x, &o.y, &o.z, &o.w}) {
        *v = {user_capacity};
    }
    users.audio_size = {user_capacity};
    users.audio = {user_capacity * audio_capacity};
}

void write(initial_message &m, std::span<std::uint8_t> b) {
    apply(m, write_tag_t{b});
}

void read(initial_message &m, std::span<std::uint8_t> b) {
    apply(m, read_tag_t{b});
}

std::size_t capacity(initial_message &m) {
    size_t size = 0;
    apply(m, capacity_tag_t{size});
    return size;
}

void write(message& m, std::span<uint8_t> b) {
    apply(m, write_tag_t{b});
}

void read(message& m, std::span<uint8_t> b) {
    apply(m, read_tag_t{b});
}

size_t capacity(message& m) {
    size_t size = 0;
    apply(m, capacity_tag_t{size});
    return size;
}
