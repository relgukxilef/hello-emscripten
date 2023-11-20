#pragma once

#include <charconv>
#include <string_view>
#include <initializer_list>
#include <span>

void parse_form(
    std::string_view body,
    std::initializer_list<std::pair<std::string_view, std::string_view*>>
    parameters
);

void append(std::span<char> &buffer, std::span<const char> string);

template<int N>
void append(std::span<char> &buffer, const char (&string)[N]) {
    append(buffer, {string, string + N - 1});
}

template<std::integral T>
void append(std::span<char> &buffer, T number) {
    buffer = {
        std::to_chars(buffer.data(), buffer.data() + buffer.size(), number).ptr,
        buffer.data() + buffer.size()
    };
}

template<class T1, class T2, class... R>
void append(
    std::span<char> &buffer, const T1 &t1, const T2 &t2, const R &... r
) {
    append(buffer, t1);
    append(buffer, t2, r...);
}

std::uint64_t unix_time();

struct jwt {
    std::uint64_t subject, issued_at, expiration;

    std::span<char> write(std::span<char> secret, std::span<char> buffer);

    bool read(
        std::span<char> secret, std::span<const char> buffer, uint64_t now
    );
};
