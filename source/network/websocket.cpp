#include "websocket.h"
#include <vector>

using namespace std;

websocket::~websocket() {
    close();
}

websocket::websocket(std::shared_ptr<websocket_data> data) {
    this->data = std::move(data);
}

std::vector<std::uint8_t> *websocket::writable() {
    if (data->up.readable)
        return nullptr;
    data->up.readable = true;
    return &data->up.buffer;
}

std::vector<std::uint8_t> *websocket::readable() {
    if (!data->down.readable)
        return nullptr;
    data->down.readable = false;
    return &data->down.buffer;
}

void websocket::close() {
    if (data)
        data->up.closed = true;
}

websocket websockets::connect(std::string url) {
    auto socket = make_shared<websocket_data>(url);
    connections.push_back(socket);
    return socket;
}
