#include "network_client.h"
#include "websocket.h"

#include <thread>

#include <emscripten/emscripten.h>
#include <emscripten/websocket.h>

#include "../state/client.h"

EM_BOOL message_callback(
    int eventType, const EmscriptenWebSocketMessageEvent *websocketEvent, 
    void* userData
) {
    put_message(
        ((websocket*)userData)->client,
        (const char*)websocketEvent->data, websocketEvent->numBytes
    );
    return EM_TRUE;
}

EM_BOOL close_callback(
    int eventType, const EmscriptenWebSocketCloseEvent *websocketEvent, 
    void* userData
) {
    set_disconnected(((websocket*)userData)->client);
    return EM_TRUE;
}

struct event_loop::data {};

event_loop::event_loop() {
}

event_loop::~event_loop() {
}

struct websocket::data {
    EMSCRIPTEN_WEBSOCKET_T websocket;
};

websocket::websocket(
    ::client& client, event_loop& loop, std::string_view host, unsigned port
) :
    client(client), loop(loop),
    d(new data{})
{
    EmscriptenWebSocketCreateAttributes websocket_attributes = {
        host.data(), "binary", EM_FALSE
    };
    auto websocket = emscripten_websocket_new(&websocket_attributes);
    emscripten_websocket_set_onmessage_callback(
        websocket, this, message_callback
    );
    emscripten_websocket_set_onclose_callback(
        websocket, this, close_callback
    );
    d->websocket = websocket;
}

websocket::~websocket() {
    emscripten_websocket_set_onmessage_callback(d->websocket, this, nullptr);
    emscripten_websocket_set_onclose_callback(d->websocket, this, nullptr);
}

bool websocket::try_write_message(std::span<std::uint8_t> buffer) {
    return false;
}

bool websocket::is_write_completed() {
    return write_completed;
}
