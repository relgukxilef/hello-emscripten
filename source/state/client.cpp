#include "client.h"

#include "../network/network_message.h"

// These may differ between server and client
unsigned message_user_capacity = 16;
unsigned message_audio_capacity = 200;

client::client(std::string_view server) {
    connection.reset(
        new websocket(*this, event_loop, server)
    );
    next_network_update = std::chrono::steady_clock::now();
    in_message = message(message_user_capacity, message_audio_capacity);
    message_in_readable = false;
    in_buffer.resize(capacity(in_message));
    out_message = message(message_user_capacity, message_audio_capacity);
    out_buffer.resize(capacity(out_message));
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
            out_message.users.size = 1;

            auto &p = out_message.users.position;
            p.x.values[0] = user_position.x;
            p.y.values[0] = user_position.y;
            p.z.values[0] = user_position.z;

            auto &o = out_message.users.orientation;
            o.x.values[0] = user_orientation.x;
            o.y.values[0] = user_orientation.y;
            o.z.values[0] = user_orientation.z;
            o.w.values[0] = user_orientation.w;

            write(out_message, out_buffer);

            connection->try_write_message(out_buffer);
            next_network_update = std::max(
                now, next_network_update + std::chrono::milliseconds{50}
            );
        }
    }

    if (message_in_readable) {
        read(in_message, in_buffer);
        std::size_t user_count = in_message.users.size;

        if (user_count != users.position.size()) {
            users.position.resize(user_count);
            users.orientation.resize(user_count);
            update_number++;
        }

        for (size_t index = 0; index < user_count; index++) {
            auto &p = in_message.users.position;
            users.position[index].x = p.x.values[index];
            users.position[index].y = p.y.values[index];
            users.position[index].z = p.z.values[index];

            auto &o = in_message.users.orientation;
            users.orientation[index].x = o.x.values[index];
            users.orientation[index].y = o.y.values[index];
            users.orientation[index].z = o.z.values[index];
            users.orientation[index].w = o.w.values[index];
        }

        message_in_readable = false;
    }
}
