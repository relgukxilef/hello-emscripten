#include "client.h"

#include "../utility/file.h"
#include "../network/network_message.h"
#include "../utility/serialization.h"

#include "../test/testing.h"

// These may differ between server and client
// TODO: read from a configuration file or command line
unsigned message_user_capacity = 16;
unsigned message_audio_capacity = 200;

client::client(std::string_view server) {
    this->server = server;
    //auto test_file = read_file("test_files/AvatarSample_B.vrm");
    //test_model = model({test_file.data(), test_file.data() + test_file.size()});
    //auto world_file = read_file("test_files/white_modern_living_room.glb");
    //world_model = 
    //    model({world_file.data(), world_file.data() + world_file.size()});

    next_network_update = std::chrono::steady_clock::now();
    in_message.reset(message_user_capacity, message_audio_capacity);
    out_message.reset(message_user_capacity, message_audio_capacity);
    encoded_audio_in.resize(message_audio_capacity);
    users.encoded_audio_out_size.resize(message_user_capacity);
    users.encoded_audio_out.resize(
        message_user_capacity
    );
    for (auto &audio : users.encoded_audio_out) {
        audio.resize(message_audio_capacity);
    }
}

void client::update(::input &input, ::web_sockets &web_sockets) {
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
    //user_position.z = glm::max(0.0f, user_position.z);

    // user interface
    input.prefer_pointer_locked = true;

    // network
    if (!connection) {
        connection = web_sockets.try_connect(server, socket_free_list);
    }

    // TODO: read time from function arguments
    auto now = std::chrono::steady_clock::now();
    if (now > next_network_update) {
        if (web_sockets.can_write(connection.get())) {
            out_message.users.size = 1;

            auto &p = out_message.users.position;
            p[0] = user_position.x;
            p[1] = user_position.y;
            p[2] = user_position.z;

            auto &o = out_message.users.orientation;
            o[0] = user_orientation.x;
            o[1] = user_orientation.y;
            o[2] = user_orientation.z;
            o[3] = user_orientation.w;

            out_message.users.voice[0].first = encoded_audio_in_size;
            copy(
                encoded_audio_in, 
                std::views::all(out_message.users.voice[0].second)
            );
            encoded_audio_in_size = 0;

            // TODO: how to communicate desired buffer capacity to caller?
            auto &buffer = web_sockets.write_buffer(connection.get());
            buffer.resize(buffer.capacity());
            buffer.resize(write(out_message, buffer));

            next_network_update = std::max(
                now, next_network_update + std::chrono::milliseconds{50}
            );
        }
    }

    if (web_sockets.can_read(connection.get())) {
        auto &buffer = web_sockets.read_buffer(connection.get());
        try {
            read(in_message, buffer);
            buffer.clear();
            std::size_t user_count = in_message.users.size;

            if (user_count != users.position.size()) {
                users.position.resize(user_count);
                users.orientation.resize(user_count);
                update_number++;
            }

            for (size_t index = 0; index < user_count; index++) {
                auto &p = in_message.users.position;
                users.position[index].x = p[index * 3 + 0];
                users.position[index].y = p[index * 3 + 1];
                users.position[index].z = p[index * 3 + 2];

                auto &o = in_message.users.orientation;
                users.orientation[index].x = o[index * 4 + 0];
                users.orientation[index].y = o[index * 4 + 1];
                users.orientation[index].z = o[index * 4 + 2];
                users.orientation[index].w = o[index * 4 + 3];

                users.encoded_audio_out_size[index] = 
                    in_message.users.voice[index].first;
                copy(
                    in_message.users.voice[index].second, 
                    std::views::all(users.encoded_audio_out[index])
                );
            }
        } catch(...) {}
    }
}

using namespace testing;

test client_test([] {
    auto url = "wss://example.org";
    client c(url);

    ::input input;
    ::web_sockets web_sockets(4, 64, 4096, 4096);

    while (any<bool>()) {
        for (auto &connection : web_sockets.connections) {
            auto &read = connection.read;
            read.resize(std::min(read.capacity(), any<size_t>()));
            for (auto &byte : read)
                byte = any<uint8_t>();
        }

        c.update(input, web_sockets);
    }
});
