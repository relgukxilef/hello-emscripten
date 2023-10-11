#include "client.h"

#include "../network/network_message.h"

client::client() {
    connection.reset(
        new websocket(*this, event_loop, "ws://localhost:28750/")
    );
    next_network_update = std::chrono::steady_clock::now();
}

void client::update(::input &input) {
    glm::vec2 touch_rotation = {};

    for (int i = 0; i < input.touch.size; i++) {
        if (input.touch.identifier[i] != -1) {
            if (input.touch.event[i] == input.touch.start) {
                // TODO: handle buttons and such
                // TODO: need to know window size
                if (rotation_touch == -1)
                    rotation_touch = i;
            } else if (input.touch.event[i] == input.touch.end) {
                if (rotation_touch == i)
                    rotation_touch = -1;
                input.touch.identifier[i] = -1; // TODO: surprising to caller
            }

            if (
                rotation_touch == i && input.touch.event[i] != input.touch.start
            ) {
                touch_rotation = 
                    input.touch.position[i] - touch_previous_position[i];
            }

            input.touch.event[i] = input.touch.none; // TODO: surprising
        }

        touch_previous_position[i] = input.touch.position[i];
    }
    
    glm::vec2 rotation = input.rotation * 2.0f + touch_rotation * -0.002f;
    user_pitch = glm::clamp(
        user_pitch + rotation.y,
        glm::radians(0.f), glm::radians(180.f)
    );
    user_yaw += rotation.x;
    user_orientation =
        glm::rotate(glm::quat{0, {1, 0, 0}}, -user_yaw, {0, 0, 1});
    user_orientation =
        glm::rotate(user_orientation, user_pitch, {1, 0, 0});

    input.motion = input.motion / glm::max(1.0f, glm::length(input.motion));

    user_position +=
        user_orientation * glm::vec3(input.motion.x, 0, input.motion.y);

    // physics
    user_position.z = glm::max(0.0f, user_position.z);

    // user interface
    input.prefer_pointer_locked = true;

    // network
    auto now = std::chrono::steady_clock::now();
    if (now > next_network_update) {
        if (connection->is_write_completed()) {
            message_header header = {{1}};
            message.resize(message_size(header));
            *reinterpret_cast<message_header*>(message.data()) = header;
            auto data = parse_message({message.data(), message.size()});
            data.users.position[0] = user_position;
            data.users.orientation[0] = user_orientation;
            connection->try_write_message({message.data(), message.size()});
            next_network_update = std::max(
                now, next_network_update + std::chrono::milliseconds{50}
            );
        }
    }

    if (message_in_readable) {
        auto header = *reinterpret_cast<message_header*>(message_in.data());
        auto data = parse_message({message_in.data(), message_in.size()});
        std::size_t user_count = header.users.size;
        if (user_count != users.position.size()) {
            users.position.resize(user_count);
            users.orientation.resize(user_count);
            update_number++;
        }
        std::move(
            data.users.position.begin(), data.users.position.end(),
            users.position.begin()
        );
        std::move(
            data.users.orientation.begin(), data.users.orientation.end(),
            users.orientation.begin()
        );
        message_in_readable = false;
    }
}
