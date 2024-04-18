#include "network_message.h"

#include <array>

#include "../utility/serialization.h"

template<class F>
void apply(uuid &m, F f) {
    apply(m.values[0], f);
    apply(m.values[1], f);
}

template<class A, class B, class F>
void apply(std::pair<A, B> &p, F f) {
    apply(p.first, f);
    apply(p.first, p.second, f);
}

template<class F>
void apply(initial_message &m, F f) {
    apply(m.size, f);
    apply(m.size, m.extensions, f);
}

template<class F>
void apply(message &m, F f) {
    apply(m.users.size, f);
    apply_fixed_point<int16_t, 8, float>(m.users.size * 3, m.users.position, f);
    apply_fixed_point<int16_t, 15, float>(
        m.users.size * 4, m.users.orientation, f
    );
    apply(m.users.size, m.users.voice, f);
}

initial_message::initial_message(unsigned int extension_capacity) {
    extensions = {extension_capacity};
}

message::message(unsigned user_capacity, unsigned audio_capacity) {
    users.position = {user_capacity * 3};
    users.orientation = {user_capacity * 4};
    users.voice = {user_capacity};
    for (auto &a : users.voice) {
        a.second = {audio_capacity};
    }
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
