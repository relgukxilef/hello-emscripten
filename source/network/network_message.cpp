#include "network_message.h"

// TODO: use std::apply to simplify the code here

size_t message_size(message_header header) {
    return
        sizeof(header) +
        (sizeof(glm::vec3) + sizeof(glm::quat)) * header.users.size;
}

template<typename T>
void parse(
    std::span<T>& value, std::span<uint8_t> buffer,
    unsigned size, unsigned& offset
) {
    value = {reinterpret_cast<T*>(buffer.data() + offset), size};
    offset += sizeof(T) * size;
}

message_data parse_message(std::span<uint8_t> buffer) {
    if (buffer.size() < sizeof(message_header)) {
        // invalid message
        return {};
    }
    auto header = *reinterpret_cast<message_header*>(buffer.data());
    message_data data;
    unsigned offset = sizeof(message_header);
    parse(data.users.position, buffer, header.users.size, offset);
    parse(data.users.orientation, buffer, header.users.size, offset);
    if (buffer.size() < offset) {
        // invalid message
        return {};
    }
    return data;
}
