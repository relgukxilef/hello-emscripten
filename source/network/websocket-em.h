#pragma once

#include <vector>
#include <memory>

#include "websocket.h"

struct em_websocket;

struct em_websockets {
    em_websockets();
    ~em_websockets();
    void update();

    ::websockets websockets;
    std::vector<std::unique_ptr<em_websocket>> data;
};
