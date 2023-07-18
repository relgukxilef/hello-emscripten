#include "network_client.h"
#include "websocket.h"

#include <thread>

#include <emscripten/emscripten.h>
#include <emscripten/websocket.h>

#include "../state/client.h"

struct websocket::data {
    client &c;
    EMSCRIPTEN_WEBSOCKET_T websocket;
    bool open = false;
};

EM_BOOL message_callback(
    int eventType, const EmscriptenWebSocketMessageEvent *websocketEvent, 
    void* userData
) {
    put_message(
        ((websocket*)userData)->d->c,
        (const char*)websocketEvent->data, websocketEvent->numBytes
    );
    return EM_TRUE;
}

EM_BOOL open_callback(
    int eventType, const EmscriptenWebSocketOpenEvent *websocketEvent, 
    void* userData
) {
    ((websocket*)userData)->d->open = true;
    return EM_TRUE;
}

EM_BOOL close_callback(
    int eventType, const EmscriptenWebSocketCloseEvent *websocketEvent, 
    void* userData
) {
    set_disconnected(((websocket*)userData)->d->c);
    return EM_TRUE;
}

struct event_loop::data {};

event_loop::event_loop() {
}

event_loop::~event_loop() {
}

websocket::websocket(::client& c, event_loop& loop, std::string_view url) :
    loop(loop),
    d(new data{c})
{
    EmscriptenWebSocketCreateAttributes websocket_attributes = {
        url.data(), nullptr, EM_FALSE
    };
    auto websocket = emscripten_websocket_new(&websocket_attributes);
    emscripten_websocket_set_onmessage_callback(
        websocket, this, message_callback
    );
    emscripten_websocket_set_onclose_callback(
        websocket, this, close_callback
    );
    emscripten_websocket_set_onopen_callback(
        websocket, this, open_callback
    );
    d->websocket = websocket;
}

websocket::~websocket() {
    emscripten_websocket_set_onmessage_callback(d->websocket, this, nullptr);
    emscripten_websocket_set_onclose_callback(d->websocket, this, nullptr);
}

bool websocket::try_write_message(std::span<std::uint8_t> buffer) {
    if (d->open) {
        emscripten_websocket_send_binary(
            d->websocket, buffer.data(), buffer.size()
        );
        return true;
    }
    return false;
}

bool websocket::is_write_completed() {
    return true;
}
