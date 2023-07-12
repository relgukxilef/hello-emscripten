#pragma once

#include <cinttypes>
#include <memory>
#include <span>
#include <string_view>
#include <atomic>

struct client;

struct event_loop {
    struct data;

    event_loop();
    ~event_loop();

    std::unique_ptr<data> d;
};

struct websocket {
    struct data;

    websocket(client& client, event_loop& loop, std::string_view url);
    ~websocket();

    // buffer needs to stay valid until is_write_completed returns true
    bool try_write_message(std::span<std::uint8_t> buffer);
    bool is_write_completed();

    event_loop& loop;
    std::unique_ptr<data> d;

    std::span<std::uint8_t> next_message;
};
