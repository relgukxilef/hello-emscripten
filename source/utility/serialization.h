#pragma once

#include <span>
#include <cinttypes>
#include <stdexcept>

#include "../utility/unique_span.h"

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
    if (write.b.size() < sizeof(T))
        throw std::overflow_error("message truncated");
    T v = value;
    for (size_t i = 0; i < sizeof(T); i++)
        // network byte order is big-endian
        write.b[sizeof(T) - i - 1] = uint8_t(v >> (i * 8));
    write.b = {write.b.begin() + sizeof(T), write.b.size() - sizeof(T)};
}

template<std::integral T>
void apply(T &value, read_tag_t read) {
    if (read.b.size() < sizeof(T))
        throw std::overflow_error("message truncated");
    T v = 0;
    std::array<std::uint8_t, sizeof(T)> bytes;
    for (size_t i = 0; i < sizeof(T); i++)
        // network byte order is big-endian
        bytes[i] = read.b[sizeof(T) - i - 1];
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
void apply(size_t size, unique_span<T> &b, F f) {
    for (size_t i = 0; i < size; i++)
        apply(b.values[i], f);
}

template<class T>
void apply(size_t, unique_span<T> &values, capacity_tag_t capacity) {
    for (size_t i = 0; i < values.capacity; i++)
        apply(values.values[i], capacity);
}

template<std::integral T, unsigned mantissa_bits, std::floating_point F>
void apply_fixed_point(size_t size, unique_span<F> &span, write_tag_t write) {
    for (size_t i = 0; i < size; i++) {
        T integral = T(span.values[i] * (1ul << mantissa_bits));
        apply(integral, write);
    }
}

template<std::integral T, unsigned mantissa_bits, std::floating_point F>
void apply_fixed_point(size_t size, unique_span<F> &span, read_tag_t read) {
    for (size_t i = 0; i < size; i++) {
        T integral;
        apply(integral, read);
        span.values[i] = float(integral) / (1ul << mantissa_bits);
    }
}

template<std::integral T, unsigned mantissa_bits, std::floating_point F>
void apply_fixed_point(
    size_t, unique_span<F> &values, capacity_tag_t capacity
) {
    capacity.size += values.capacity * sizeof(T);
}
