#include "utility.h"

#include <charconv>
#include <chrono>
#include <string_view>
#include <algorithm>
#include <cassert>

#include <boost/archive/iterators/transform_width.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/params.h>
#include <openssl/kdf.h>
#include <openssl/rand.h>
#include <openssl/core_names.h>

void parse_form(
    std::ranges::subrange<char*> body,
    std::initializer_list<std::pair<
        std::string_view, std::ranges::subrange<char*>*
    >> parameters
) {
    auto c = body.begin();
    while (c != body.end()) {
        auto c2 = c;
        while (c != body.end() && *c++ != '=');
        std::ranges::subrange<char*> key{c2, c - 1};

        c2 = c;
        while (c != body.end() && *c++ != '&');
        std::ranges::subrange<char*> value{c2, c - 1};

        for (auto parameter : parameters) {
            if (std::ranges::equal(parameter.first, key)) {
                *parameter.second = value;
            }
        }
    }
}

void append(range_stream &buffer, std::ranges::subrange<const char*> string) {
    auto in = string.begin();
    while (in != string.end() && buffer.cursor != buffer.end()) {
        *buffer.cursor++ = *in++;
    }
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

char *base64_encode(
    std::ranges::subrange<const char *> input,
    std::ranges::subrange<char*> output
) {
    typedef boost::archive::iterators::transform_width<
        const char *, 6, 8
    > base64_iterator;

    auto
        in_begin = base64_iterator(input.begin()),
        in_end = base64_iterator(input.end());
    auto out_begin = output.begin(), out_end = output.end();

    while (in_begin != in_end && out_begin != out_end) {
        *out_begin++ = to_base64(*in_begin++);
    }
    return out_begin;
}

char *base64_decode(
    std::ranges::subrange<const char*> input,
    std::ranges::subrange<char*> output
) {
    typedef boost::archive::iterators::transform_width<
        boost::transform_iterator<from_base64, const char*>, 8, 6
    > base64_iterator;

    auto in_begin = base64_iterator(input.data());
    auto in_end = base64_iterator(input.data() + input.size());
    auto out_begin = output.begin();

    while (in_begin != in_end && out_begin != output.end()) {
        *out_begin++ = *in_begin++;
    }
    return out_begin;
}

char *hmac_sha256(
    std::ranges::subrange<const char*> key,
    std::ranges::subrange<const char*> message,
    std::ranges::subrange<char*> destination
) {
    unsigned int size = 0;
    HMAC(
        EVP_sha256(),
        key.data(), key.size(),
        (unsigned char*)message.data(), message.size(),
        (unsigned char*)destination.data(), &size
    );
    return destination.begin() + size;
}

char *jwt::write(
    std::ranges::subrange<char*> buffer,
    std::ranges::subrange<const char*> secret
) {
    char header[] = "{\"alg\":\"HS256\"}";

    range_stream destination = buffer;

    destination.cursor = base64_encode(
        {std::begin(header), std::end(header) - 1},
        destination.right()
    );
    append(destination, ".");

    char payload_buffer[128];
    range_stream payload(payload_buffer);

    append(
        payload,
        "{\"sub\":", subject,
        ",\"exp\":", expiration, "}"
    );
    // Putting roles/groups/entitlements in the token is good if they differ per
    // token. E.g. some rights are only available to a user, if they logged in
    // using a certain method, even if the user in general has those rights.

    destination.cursor = base64_encode(payload.left(), destination.right());

    payload.cursor = payload.begin();
    payload.cursor = hmac_sha256(secret, destination.left(), payload_buffer);

    append(destination, ".");
    destination.cursor = base64_encode(payload.left(), destination.right());

    return destination.cursor;
}

bool jwt::read(
    std::ranges::subrange<const char*> secret,
    std::ranges::subrange<char*> buffer,
    uint64_t now
) {
    char payload_buffer[128];
    auto buffer_end = buffer.data() + buffer.size();

    auto dot1 = std::find(buffer.data(), buffer_end, '.');
    auto dot2 = std::find(dot1 + 1, buffer_end, '.');

    std::ranges::subrange<char*> payload = std::views::all(payload_buffer);

    char signature_buffer[32];
    std::ranges::subrange<char*> signature = {
        std::begin(signature_buffer),
        hmac_sha256(secret, {buffer.data(), dot2}, signature_buffer)
    };

    payload = {
        payload.begin(),
        base64_decode({dot2 + 1, buffer_end}, payload_buffer)
    };
    if (
        payload.size() != signature.size() ||
        !std::equal(signature.begin(), signature.end(), payload.begin())
    ) {
        return false;
    }
    puts("signature checked");

    payload = {
        payload.begin(),
        base64_decode({dot1 + 1, dot2}, payload_buffer)
    };


    auto result = std::ranges::search(payload, "\"sub\":");
    if (!result)
        return false;
    std::from_chars(result.begin(), result.end(), subject);

    result = std::ranges::search(payload, "\"exp\":");
    if (result)
        return false;
    std::from_chars(result.begin(), result.end(), expiration);

    if (now >= expiration) {
        return false;
    }

    return true;
}

char *copy(
    std::ranges::subrange<char*> destination,
    std::ranges::subrange<const char*> source
) {
    return std::copy(
        source.begin(),
        source.begin() + std::min(source.size(), destination.size()),
        destination.begin()
    );
}

char* write_password(
    std::ranges::subrange<char*> buffer,
    std::ranges::subrange<const char*> user_input,
    std::ranges::subrange<const char *> pepper, unsigned int m, unsigned int t
) {
    // TODO: normalize user_input and check for invalid characters

    EVP_KDF *function = EVP_KDF_fetch(nullptr, "ARGON2ID", nullptr);
    assert(function);
    EVP_KDF_CTX *context = EVP_KDF_CTX_new(function);
    assert(context);

    unsigned char salt[16];
    RAND_bytes(salt, std::size(salt));
    uint32_t lanes = 1;

    unsigned char hash[128 / 8];

    if (buffer.size() < std::size(hash) * 2)
        return buffer.end();

    OSSL_PARAM parameters[] = {
        OSSL_PARAM_construct_uint32(OSSL_KDF_PARAM_ARGON2_MEMCOST, &m),
        OSSL_PARAM_construct_uint32(OSSL_KDF_PARAM_ARGON2_LANES, &lanes),
        OSSL_PARAM_construct_uint32(OSSL_KDF_PARAM_ITER, &t),
        OSSL_PARAM_construct_octet_string(
            OSSL_KDF_PARAM_SALT, salt, std::size(salt)
        ),
        OSSL_PARAM_construct_octet_string(
            OSSL_KDF_PARAM_SECRET,
            (void *)pepper.data(),
            pepper.size()
        ),
        OSSL_PARAM_construct_octet_string(
            OSSL_KDF_PARAM_PASSWORD,
            (void *)user_input.data(),
            user_input.size()
        ),
        OSSL_PARAM_construct_end()
    };

    assert(EVP_KDF_derive(
        context, hash, std::size(hash), parameters
    ) == 1);

    size_t hex_length = 0;
    OPENSSL_buf2hexstr_ex(
        buffer.begin(), buffer.size(), &hex_length, salt, std::size(salt), 0
    );
    buffer.advance(hex_length - 1);
    OPENSSL_buf2hexstr_ex(
        buffer.begin(), buffer.size(), &hex_length, hash, std::size(hash), 0
    );
    buffer.advance(hex_length - 1);

    EVP_KDF_CTX_free(context);
    EVP_KDF_free(function);

    return buffer.begin();
}

bool is_password_valid(
    std::ranges::subrange<const char*> buffer,
    std::ranges::subrange<const char*> user_input,
    std::ranges::subrange<const char*> pepper,
    unsigned m, unsigned t
) {
    char hex_salt[16 + 1] = {0};
    std::copy(buffer.begin(), buffer.begin() + 16, hex_salt);
    char hex_hash[32 + 1] = {0};
    std::copy(buffer.begin() + 16, buffer.begin() + 16 + 32, hex_hash);

    unsigned char salt[8 + 1] = {0};
    unsigned char hash_a[128 / 8 + 1] = {0};

    if (buffer.size() < (std::size(salt) + std::size(hash_a)) * 2)
        return buffer.end();

    OPENSSL_hexstr2buf_ex(salt, std::size(salt), nullptr, hex_salt, 0);
    OPENSSL_hexstr2buf_ex(hash_a, std::size(hash_a), nullptr, hex_hash, 0);

    // TODO: normalize user_input and check for invalid characters
    // TODO: read parameters from buffer

    EVP_KDF *function = EVP_KDF_fetch(nullptr, "ARGON2D", nullptr);
    EVP_KDF_CTX *context = EVP_KDF_CTX_new(function);

    unsigned char hash_b[128 / 8];

    uint32_t lanes = 1;

    OSSL_PARAM parameters[] = {
        OSSL_PARAM_construct_uint32(OSSL_KDF_PARAM_ARGON2_MEMCOST, &m),
        OSSL_PARAM_construct_uint32(OSSL_KDF_PARAM_ARGON2_LANES, &lanes),
        OSSL_PARAM_construct_uint32(OSSL_KDF_PARAM_ITER, &t),
        OSSL_PARAM_construct_octet_string(
            OSSL_KDF_PARAM_SALT, salt, std::size(salt)
        ),
        OSSL_PARAM_construct_octet_string(
            OSSL_KDF_PARAM_SECRET,
            (void *)pepper.data(),
            pepper.size()
        ),
        OSSL_PARAM_construct_octet_string(
            OSSL_KDF_PARAM_PASSWORD,
            (void *)user_input.data(),
            user_input.size()
        ),
        OSSL_PARAM_construct_end()
    };

    EVP_KDF_derive(
        context, hash_b, std::size(hash_b), parameters
    );

    return OPENSSL_strncasecmp(
        (const char *)hash_a, (const char *)hash_b, std::size(hash_a)
    );
}
