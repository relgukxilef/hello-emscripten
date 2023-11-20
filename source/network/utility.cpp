#include "utility.h"

#include <charconv>
#include <chrono>
#include <span>

#include <boost/archive/iterators/transform_width.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <openssl/evp.h>
#include <openssl/hmac.h>

void parse_form(
    std::string_view body,
    std::initializer_list<std::pair<std::string_view, std::string_view*>>
        parameters
) {
    auto c = body.begin();
    while (c != body.end()) {
        auto c2 = c;
        while (c != body.end() && *c++ != '=');
        std::string_view key{c2, c - 1};

        c2 = c;
        while (c != body.end() && *c++ != '&');
        std::string_view value{c2, c - 1};

        for (auto parameter : parameters) {
            if (parameter.first == key) {
                *parameter.second = value;
            }
        }
    }
}

void skip(std::span<char> &buffer, std::span<const char> string) {
    buffer = {buffer.begin() + string.size(), buffer.end()};
}

void append(std::span<char> &buffer, std::span<const char> string) {
    auto out_begin = buffer.begin(), out_end = buffer.end();
    auto in_begin = string.begin(), in_end = string.end();
    while (in_begin != in_end && out_begin != out_end) {
        *out_begin++ = *in_begin++;
    }
    buffer = {out_begin, out_end};
}

char to_base64(char six_bit) {
    char lookup_table[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789"
        "-_";
    return lookup_table[(unsigned)six_bit];
}

struct from_base64 {
    typedef char result_type;
    char operator()(char six_bit) const {
        char lookup_table[] = {
             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,62, 0, 0,
            52,53,54,55,56,57,58,59,60,61, 0, 0, 0, 0, 0, 0, // render '=' as 0
             0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,
            15,16,17,18,19,20,21,22,23,24,25, 0, 0, 0, 0,63,
             0,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
            41,42,43,44,45,46,47,48,49,50,51, 0, 0, 0, 0, 0
        };
        return lookup_table[std::min<unsigned>(six_bit, 127)];
    }
};

std::uint64_t unix_time() {
    auto now = std::chrono::system_clock::now();
    auto epoch = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::seconds>(epoch).count();
}

std::span<char> base64_encode(
    std::string_view input, std::span<char> output
) {
    typedef boost::archive::iterators::transform_width<char*, 6, 8>
        base64_iterator;

    auto in_begin = base64_iterator(
        input.data()), in_end = base64_iterator(input.data() + input.size()
    );
    auto out_begin = output.begin(), out_end = output.end();

    while (in_begin != in_end && out_begin != out_end) {
        *out_begin++ = to_base64(*in_begin++);
    }
    return {output.begin(), out_begin};
}

std::span<char> base64_decode(
    std::string_view input, std::span<char> output
    ) {
    typedef boost::archive::iterators::transform_width<
        boost::transform_iterator<from_base64, const char*>, 8, 6
    > base64_iterator;

    auto in_begin = base64_iterator(input.data());
    auto in_end = base64_iterator(input.data() + input.size());
    auto out_begin = output.begin(), out_end = output.end();

    while (in_begin != in_end && out_begin != out_end) {
        *out_begin++ = *in_begin++;
    }
    return {output.begin(), out_begin};
}

std::span<char> hmac_sha256(
    std::span<const char> key, std::span<const char> message,
    std::span<char> destination
    ) {
    unsigned int size = 0;
    HMAC(
        EVP_sha256(),
        key.data(), key.size(),
        (unsigned char*)message.data(), message.size(),
        (unsigned char*)destination.data(), &size
    );
    return {destination.data(), size};
}

std::span<char> jwt::write(std::span<char> secret, std::span<char> buffer) {
    char header[] = "{\"alg\":\"HS256\",\"typ\":\"JWT\"}";

    auto span = buffer;

    skip(span, base64_encode(header, span));
    append(span, ".");

    char payload_buffer[128];
    std::span<char> payload(payload_buffer);

    append(
        payload,
        "{\"sub\":", subject, ",\"iat\":", issued_at,
        ",\"exp\":", expiration, "}"
    );

    skip(span, base64_encode({payload_buffer, payload.data()}, span));

    skip(payload, hmac_sha256(
        secret, {buffer.begin(), buffer.size() - span.size()}, payload_buffer
    ));

    append(span, ".");
    skip(span, base64_encode({payload_buffer, payload.data()}, span));

    return {buffer.begin(), buffer.size() - span.size()};
}

bool jwt::read(
    std::span<char> secret, std::span<const char> buffer, uint64_t now
) {
    char payload_buffer[128];
    auto buffer_end = buffer.data() + buffer.size();

    auto dot1 = std::find(buffer.data(), buffer_end, '.');
    auto dot2 = std::find(dot1 + 1, buffer_end, '.');

    std::span<char> payload = payload_buffer;

    char signature_buffer[32];
    auto signature =
        hmac_sha256(secret, {buffer.data(), dot2}, signature_buffer);

    payload = base64_decode({dot2 + 1, buffer_end}, payload_buffer);
    if (
        payload.size() != signature.size() ||
        !std::equal(signature.begin(), signature.end(), payload.begin())
        ) {
        return false;
    }
    puts("signature checked");

    payload = base64_decode({dot1 + 1, dot2}, payload_buffer);


    auto end = payload.data() + payload.size();

    {
        char query[] = "\"sub\":";
        auto iterator = std::search(
            payload.data(), end, std::begin(query), std::end(query) - 1
            );
        if (iterator == end)
            return false;
        std::from_chars(iterator + std::size(query) - 1, end, subject);
    }
    {
        char query[] = "\"iat\":";
        auto iterator = std::search(
            payload.data(), end, std::begin(query), std::end(query) - 1
            );
        if (iterator == end)
            return false;
        std::from_chars(iterator + std::size(query) - 1, end, issued_at);
    }
    {
        char query[] = "\"exp\":";
        auto iterator = std::search(
            payload.data(), end, std::begin(query), std::end(query) - 1
            );
        if (iterator == end)
            return false;
        std::from_chars(iterator + std::size(query) - 1, end, expiration);
    }

    if (issued_at > now || now >= expiration) {
        return false;
    }

    return true;
}
