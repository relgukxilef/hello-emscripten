#include "network_message.h"
#include <array>

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

struct write_tag_t {
    std::span<uint8_t> &b;
};

struct read_tag_t {
    std::span<uint8_t> &b;
};

struct capacity_tag_t {
    size_t &size;
};

template<std::integral T>
void apply(T &value, write_tag_t write) {
    assert(write.b.size() > 0);
    T v = value;
    for (size_t i = 0; i < sizeof(T); i++)
        write.b[i] = v >> (i * 8);
    write.b = {write.b.begin() + sizeof(T), write.b.size() - sizeof(T)};
}

template<std::integral T>
void apply(T &value, read_tag_t read) {
    assert(read.b.size() > 0);
    T v = 0;
    std::array<std::uint8_t, sizeof(T)> bytes;
    for (size_t i = 0; i < sizeof(T); i++)
        bytes[i] = read.b[i];
    for (size_t i = 0; i < sizeof(T); i++)
        v |= T(bytes[i]) << (i * 8);
    value = v;
    read.b = {read.b.begin() + sizeof(T), read.b.size() - sizeof(T)};
}

template<std::integral T>
void apply(T &, capacity_tag_t size) {
    size.size += sizeof(T);
}

template<class T, class F>
void apply(size_t size, buffer<T> &b, F f) {
    for (size_t i = 0; i < size; i++)
        apply(b.values[i], f);
}

template<class T>
void apply(size_t, buffer<T> &values, capacity_tag_t capacity) {
    capacity.size += values.capacity * sizeof(T);
}

template<std::integral T, unsigned mantissa_bits>
void apply(fixed_precision<T, mantissa_bits> &value, write_tag_t write) {
    T integral = value.value / (1 << mantissa_bits);
    apply(integral, write);
}

template<std::integral T, unsigned mantissa_bits>
void apply(fixed_precision<T, mantissa_bits> &value, read_tag_t read) {
    T integral;
    apply(integral, read);
    value.value = float(integral) * (1 << mantissa_bits);
}

template<std::integral T, unsigned mantissa_bits>
void apply(fixed_precision<T, mantissa_bits> &value, capacity_tag_t capacity) {
    T integral;
    apply(integral, capacity);
}

template<class F>
void apply(message &m, F f) {
    apply(m.users.size, f);
    auto &p = m.users.position;
    for (auto *v : {&p.x, &p.y, &p.z})
        apply(m.users.size, *v, f);
    auto &o = m.users.orientation;
    for (auto *v : {&o.x, &o.y, &o.z, &o.w})
        apply(m.users.size, *v, f);
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
