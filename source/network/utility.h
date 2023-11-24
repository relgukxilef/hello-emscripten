#pragma once

#include <charconv>
#include <string_view>
#include <initializer_list>
#include <ranges>

void parse_form(
    std::string_view body,
    std::initializer_list<std::pair<std::string_view, std::string_view*>>
    parameters
);

struct range_stream : public std::ranges::subrange<char*> {
    range_stream(std::ranges::subrange<char*> buffer) :
        std::ranges::subrange<char*>(buffer), cursor(buffer.begin()) {}
    std::ranges::subrange<char*> right() {
        return {cursor, end()};
    }
    std::ranges::subrange<char*> left() {
        return {begin(), cursor};
    }

    char *cursor;
};

void append(std::span<char> &buffer, std::span<const char> string);

template<int N>
void append(range_stream &buffer, const char (&string)[N]) {
    append(buffer, {string, string + N - 1});
}

template<std::integral T>
void append(range_stream &buffer, T number) {
    buffer.cursor = buffer.begin() + (
        std::to_chars(
            buffer.right().data(), buffer.data() + buffer.size(), number
        ).ptr -
        buffer.data()
    );
}

template<class T1, class T2, class... R>
void append(range_stream &buffer, const T1 &t1, const T2 &t2, const R &... r) {
    append(buffer, t1);
    append(buffer, t2, r...);
}

std::uint64_t unix_time();

struct jwt {
    std::uint64_t subject, issued_at, expiration;

    char *write(
        std::ranges::subrange<const char*> secret,
        std::ranges::subrange<char*> buffer
    );

    bool read(
        std::ranges::subrange<const char*> secret,
        std::ranges::subrange<char*> buffer, uint64_t now
    );
};
