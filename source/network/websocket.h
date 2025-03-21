#pragma once

#include <cinttypes>
#include <memory>
#include <string>
#include <vector>

struct websocket_pipe {
    std::vector<uint8_t> buffer;
    bool readable = false, closed = false;
};

struct websocket_data {
    std::string url;
    websocket_pipe up, down;
};

struct websocket {
    websocket() = default;
    websocket(std::shared_ptr<websocket_data> data);
    websocket(const websocket &) = delete;
    websocket(websocket &&) = default;
    websocket &operator=(const websocket &) = delete;
    websocket &operator=(websocket &&) = default;
    ~websocket();
    
    void close();
    std::vector<std::uint8_t> *writable();
    std::vector<std::uint8_t> *readable();

    std::shared_ptr<websocket_data> data;
};

struct websockets {
    websocket connect(std::string url);
    std::vector<std::shared_ptr<websocket_data>> connections;
};
