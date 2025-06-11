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

void message::reset(unsigned user_capacity, unsigned audio_capacity) {
    users.position.reset(user_capacity * 3);
    users.orientation.reset(user_capacity * 4);
    users.voice.reset(user_capacity);
    for (auto &a : users.voice) {
        a.second.reset(audio_capacity);
    }
    this->user_capacity = user_capacity;
    this->audio_capacity = audio_capacity;
}

void message::clear() {
    users.size = 0;
}

template<class T>
void append(
    unique_span<T> &d, const unique_span<T> &s, 
    size_t offset
) {
    copy(
        std::ranges::subrange(s.begin(), s.end()),
        std::ranges::subrange(d.begin() + offset, d.end())
    );
}

void message::append(const message &other) {
    ::append(users.position, other.users.position, users.size * 3);
    ::append(users.orientation, other.users.orientation, users.size * 4);
    ::append(users.voice, other.users.voice, users.size);
    users.size += other.users.size;
}

void write(initial_message &m, std::span<std::uint8_t> b) {
    apply(m, write_tag_t{b});
}

std::size_t read(initial_message &m, std::span<std::uint8_t> b) {
    auto begin = b.data();
    apply(m, read_tag_t{b});
    return b.data() - begin;
}

std::size_t capacity(initial_message &m) {
    size_t size = 0;
    apply(m, capacity_tag_t{size});
    return size;
}

std::size_t write(message& m, std::span<uint8_t> b) {
    auto begin = b.data();
    apply(m, write_tag_t{b});
    return b.data() - begin;
}

std::size_t read(message& m, std::span<uint8_t> b) {
    auto begin = b.data();
    apply(m, read_tag_t{b});
    return b.data() - begin;
}

size_t capacity(message& m) {
    size_t size = 0;
    apply(m, capacity_tag_t{size});
    return size;
}
