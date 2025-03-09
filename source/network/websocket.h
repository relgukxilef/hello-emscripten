#pragma once

#include <cinttypes>
#include <memory>
#include <string>
#include <vector>

struct websocket_pipe {
    std::string url;
    std::vector<uint8_t> buffer;
    bool closed = false;
    bool readable = false;
};

struct websocket_duplex {
    std::shared_ptr<websocket_pipe> send, receive;
    std::vector<std::uint8_t> *writable();
    std::vector<std::uint8_t> *readable();
};

struct websocket {
    websocket() = default;
    websocket(websocket_duplex duplex);
    websocket(const websocket &) = delete;
    websocket(websocket &&) = default;
    websocket &operator=(const websocket &) = delete;
    websocket &operator=(websocket &&) = default;
    ~websocket();
    void close();

    websocket_duplex duplex;
};

struct websockets {
    websocket connect(std::string url);
    std::vector<websocket_duplex> websockets;
};
