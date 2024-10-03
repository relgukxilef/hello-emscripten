#pragma once

#include <charconv>
#include <initializer_list>
#include <ranges>

void parse_form(
    std::ranges::subrange<char*> body,
    std::initializer_list<std::pair<
        std::string_view, std::ranges::subrange<char*>*
    >> parameters
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

char *copy(
    std::ranges::subrange<char*> buffer,
    std::ranges::subrange<const char*> secret
);

void append(range_stream &buffer, std::ranges::subrange<const char*> string);

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

char *base64_encode(
    std::ranges::subrange<const char*> input,
    std::ranges::subrange<char*> output
);

char *base64_decode(
    std::ranges::subrange<const char*> input,
    std::ranges::subrange<char*> output
);

struct jwt {
    std::uint64_t subject, expiration;

    char *write(
        std::ranges::subrange<char*> buffer,
        std::ranges::subrange<const char*> secret
    );

    bool read(
        std::ranges::subrange<const char*> secret,
        std::ranges::subrange<char*> buffer, uint64_t now
    );
};

/**
 * @brief write_password creates a new salt and writes the password settings and
 * the Argon2 hashed password into buffer.
 */
char *write_password(
    std::ranges::subrange<char*> buffer,
    std::ranges::subrange<const char*> user_input,
    std::ranges::subrange<const char*> pepper,
    unsigned m = 47104, unsigned t = 1
);

/**
 * @brief is_password_valid reads the password settings and the salt from the
 * buffer and compares it with the user input.
 */
bool is_password_valid(
    std::ranges::subrange<const char*> buffer,
    std::ranges::subrange<const char*> user_input,
    std::ranges::subrange<const char*> pepper,
    unsigned m = 47104, unsigned t = 1
);
