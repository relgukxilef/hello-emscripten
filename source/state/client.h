#pragma once

#include <vector>
#include <memory>
#include <chrono>
#include <atomic>
#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "input.h"
#include "../network/websocket.h"
#include "../network/network_message.h"
#include "model.h"

struct client {
    client(std::string_view server);
    // TODO: maybe this function should not be in this struct
    void update(::input &input, ::web_sockets &web_sockets);

    glm::vec3 user_position {0, 0, 0};
    float user_pitch = glm::radians(90.f), user_yaw = 0;
    glm::quat user_orientation {0, 0, 0, 1};

    glm::vec2 touch_previous_position[input::touch::size];
    int rotation_touch = -1, movement_touch = -1;

    std::vector<std::uint8_t> encoded_audio_in;
    unsigned encoded_audio_in_size = 0;

    // no need to double/triple buffer the state if the messages are already
    // buffered
    unsigned time;
    unsigned update_number = 0;
    struct {
        // TODO: maybe only use the message class
        std::vector<glm::vec3> position;
        std::vector<glm::quat> orientation;
        std::vector<unsigned> avatar;
        
        std::vector<unsigned> encoded_audio_out_size;
        std::vector<std::vector<std::uint8_t>> encoded_audio_out;
    } users;

    std::string world_path;

    model test_model, world_model;
    
    std::chrono::steady_clock::time_point next_network_update;

    // out-going
    message out_message;
    std::vector<std::uint8_t> out_buffer;

    // in-coming
    // TODO: might need a queue for delta compression
    message in_message;
    std::vector<std::uint8_t> in_buffer;
    std::atomic_bool message_in_readable;

    std::vector<web_socket> socket_free_list;

    unique_web_socket connection;
};
