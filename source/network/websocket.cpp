#include "websocket.h"
#include <vector>

using namespace std;

websocket::~websocket() {
    close();
}

websocket::websocket(websocket_duplex duplex) {
    this->duplex = std::move(duplex);
}

std::vector<std::uint8_t> *websocket_duplex::writable()
{
    if (send->readable)
        return nullptr;
    return &send->buffer;
}

std::vector<std::uint8_t> *websocket_duplex::readable() {
    if (!receive->readable)
        return nullptr;
    return &receive->buffer;
}

void websocket::close() {
    if (duplex.send)
        duplex.send->closed = true;
}

websocket websockets::connect(std::string url) {
    auto send = make_shared<websocket_pipe>(url);
    auto receive = make_shared<websocket_pipe>();
    websockets.push_back({receive, send});
    return websocket_duplex{send, receive};
}
