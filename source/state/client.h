#pragma once

#include <vector>
#include <span>
#include <memory>
#include <chrono>
#include <atomic>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "input.h"
#include "../network/websocket.h"
#include "model.h"

struct client {
    client(std::string_view server);
    // TODO: maybe this function should not be in this struct
    void update(::input& input);

    glm::vec3 user_position {0, 0, 0};
    float user_pitch = glm::radians(90.f), user_yaw = 0;
    glm::quat user_orientation {0, 0, 0, 1};

    glm::vec2 touch_previous_position[input::touch::size];
    int rotation_touch = -1, movement_touch = -1;

    // no need to double/tripple buffer the state if the messages are already
    // buffered
    unsigned time;
    unsigned update_number = 0;
    struct {
        std::vector<glm::vec3> position;
        std::vector<glm::quat> orientation;
        std::vector<unsigned> avatar;
    } users;

    std::string world_path;

    model test_model, world_model;

    std::chrono::steady_clock::time_point next_network_update;

    // out-going
    std::vector<std::uint8_t> message;

    // in-comming
    // TODO: might need a queue for delta compression
    std::vector<std::uint8_t> message_in;
    std::atomic_bool message_in_readable;

    // TODO: maybe should be in another struct
    ::event_loop event_loop;

    std::unique_ptr<websocket> connection;
};
