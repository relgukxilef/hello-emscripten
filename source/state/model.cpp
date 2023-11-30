#include "model.h"

#include <array>
#include <exception>

#include <boost/json.hpp>

template<std::integral T>
T read(std::ranges::subrange<uint8_t*> &b) {
    T v = 0;
    std::array<std::uint8_t, sizeof(T)> bytes;
    for (size_t i = 0; i < sizeof(T); i++)
        // gltf uses little-endian
        bytes[i] = b[i];
    for (size_t i = 0; i < sizeof(T); i++)
        v |= T(bytes[i]) << (i * 8);
    b = {b.begin() + sizeof(T), b.end()};
    return v;
}

struct parse_exception : public std::exception {
    const char* what() const override {
        return "parse_exception\n";
    }
};

void parse_check(bool correct) {
    if (!correct) {
        throw parse_exception();
    }
}

struct handler {
    /// The maximum number of elements allowed in an array
    static constexpr std::size_t max_array_size = -1;

    /// The maximum number of elements allowed in an object
    static constexpr std::size_t max_object_size = -1;

    /// The maximum number of characters allowed in a string
    static constexpr std::size_t max_string_size = -1;

    /// The maximum number of characters allowed in a key
    static constexpr std::size_t max_key_size = -1;

    bool on_document_begin(boost::system::error_code& ec) { return true; }

    bool on_document_end(boost::system::error_code& ec)  { return true; }

    /// Called when the beginning of an array is encountered.
    ///
    /// @return `true` on success.
    /// @param ec Set to the error, if any occurred.
    ///
    bool on_array_begin(boost::system::error_code& ec)  { return true; }

    /// Called when the end of the current array is encountered.
    ///
    /// @return `true` on success.
    /// @param n The number of elements in the array.
    /// @param ec Set to the error, if any occurred.
    ///
    bool on_array_end(std::size_t n, boost::system::error_code& ec)  {
        return true;
    }

    /// Called when the beginning of an object is encountered.
    ///
    /// @return `true` on success.
    /// @param ec Set to the error, if any occurred.
    ///
    bool on_object_begin(boost::system::error_code& ec)  { return true; }

    /// Called when the end of the current object is encountered.
    ///
    /// @return `true` on success.
    /// @param n The number of elements in the object.
    /// @param ec Set to the error, if any occurred.
    ///
    bool on_object_end(std::size_t n, boost::system::error_code& ec)  {
        return true;
    }

    /// Called with characters corresponding to part of the current string.
    ///
    /// @return `true` on success.
    /// @param s The partial characters
    /// @param n The total size of the string thus far
    /// @param ec Set to the error, if any occurred.
    ///
    bool on_string_part(
        std::string_view s, std::size_t n, boost::system::error_code& ec
    )  {
        return true;
    }

    /// Called with the last characters corresponding to the current string.
    ///
    /// @return `true` on success.
    /// @param s The remaining characters
    /// @param n The total size of the string
    /// @param ec Set to the error, if any occurred.
    ///
    bool on_string(
        std::string_view s, std::size_t n, boost::system::error_code& ec
    )  {
        return true;
    }

    /// Called with characters corresponding to part of the current key.
    ///
    /// @return `true` on success.
    /// @param s The partial characters
    /// @param n The total size of the key thus far
    /// @param ec Set to the error, if any occurred.
    ///
    bool on_key_part(
        std::string_view s, std::size_t n, boost::system::error_code& ec
    )  {
        return true;
    }

    /// Called with the last characters corresponding to the current key.
    ///
    /// @return `true` on success.
    /// @param s The remaining characters
    /// @param n The total size of the key
    /// @param ec Set to the error, if any occurred.
    ///
    bool on_key(
        std::string_view s, std::size_t n, boost::system::error_code& ec
    )  {
        return true;
    }

    /// Called with the characters corresponding to part of the current number.
    ///
    /// @return `true` on success.
    /// @param s The partial characters
    /// @param ec Set to the error, if any occurred.
    ///
    bool on_number_part(std::string_view s, boost::system::error_code& ec)  {
        return true;
    }

    /// Called when a signed integer is parsed.
    ///
    /// @return `true` on success.
    /// @param i The value
    /// @param s The remaining characters
    /// @param ec Set to the error, if any occurred.
    ///
    bool on_int64(int64_t i, std::string_view s, boost::system::error_code& ec)  {
        return true;
    }

    /// Called when an unsigend integer is parsed.
    ///
    /// @return `true` on success.
    /// @param u The value
    /// @param s The remaining characters
    /// @param ec Set to the error, if any occurred.
    ///
    bool on_uint64(uint64_t u, std::string_view s, boost::system::error_code& ec)  {
        return true;
    }

    /// Called when a double is parsed.
    ///
    /// @return `true` on success.
    /// @param d The value
    /// @param s The remaining characters
    /// @param ec Set to the error, if any occurred.
    ///
    bool on_double(double d, std::string_view s, boost::system::error_code& ec)  {
        return true;
    }

    /// Called when a boolean is parsed.
    ///
    /// @return `true` on success.
    /// @param b The value
    /// @param s The remaining characters
    /// @param ec Set to the error, if any occurred.
    ///
    bool on_bool(bool b, boost::system::error_code& ec)  {
        return true;
    }

    /// Called when a null is parsed.
    ///
    /// @return `true` on success.
    /// @param ec Set to the error, if any occurred.
    ///
    bool on_null(boost::system::error_code& ec)  {
        return true;
    }

    /// Called with characters corresponding to part of the current comment.
    ///
    /// @return `true` on success.
    /// @param s The partial characters.
    /// @param ec Set to the error, if any occurred.
    ///
    bool on_comment_part(std::string_view s, boost::system::error_code& ec)  {
        return true;
    }

    /// Called with the last characters corresponding to the current comment.
    ///
    /// @return `true` on success.
    /// @param s The remaining characters
    /// @param ec Set to the error, if any occurred.
    ///
    bool on_comment(std::string_view s, boost::system::error_code& ec)  {
        return true;
    }
};

model::model(std::ranges::subrange<uint8_t*> file) {
    auto magic = read<uint32_t>(file);
    parse_check(magic == 0x46546C67);
    auto version = read<uint32_t>(file);
    parse_check(version == 2);
    auto length = read<uint32_t>(file);
    parse_check(length >= 12);

    auto json_length = read<uint32_t>(file);
    parse_check(read<uint32_t>(file) == 0x4E4F534A); // chunk type == JSON

    boost::json::basic_parser<handler> json_parser({});
    boost::system::error_code error;
    json_parser.write_some(
        false, reinterpret_cast<char*>(file.data()), json_length, error
    );
}
