#include "websocket.h"

#include <thread>

#include <emscripten/emscripten.h>
#include <emscripten/websocket.h>

#include "../state/client.h"
#include "websocket-em.h"

using namespace std;

struct em_websocket {
    EMSCRIPTEN_WEBSOCKET_T websocket;
    shared_ptr<websocket_data> data;
    em_websockets *websockets;
};

em_websockets::em_websockets() = default;

em_websockets::~em_websockets() = default;

void em_websockets::update() {
    for (auto &websocket : websockets.connections) {
        auto data = make_unique<em_websocket>();
        EmscriptenWebSocketCreateAttributes websocket_attributes = {
            websocket->url.data(), nullptr, EM_FALSE
        };
        data->websocket = emscripten_websocket_new(&websocket_attributes);
        emscripten_websocket_set_onmessage_callback(
            data->websocket, data.get(), 
            [](
                int, const EmscriptenWebSocketMessageEvent *websocketEvent, 
                void *userData
            ) {
                auto data = ((em_websocket *)userData);
                auto &down = data->data->down;
                if (down.readable) {
                    return EM_TRUE;
                }
                down.buffer.assign(
                    (const uint8_t*)websocketEvent->data,
                    (const uint8_t*)websocketEvent->data + 
                    websocketEvent->numBytes
                );
                down.readable = true;
                return EM_TRUE;
            }
        );
        emscripten_websocket_set_onclose_callback(
            data->websocket, data.get(),
            [](int, const EmscriptenWebSocketCloseEvent *, void *userData) {
                auto data = ((em_websocket *)userData);
                data->data->down.closed = true;
                return EM_TRUE;
            }
        );
        emscripten_websocket_set_onopen_callback(
            data->websocket, data.get(), 
            [](int, const EmscriptenWebSocketOpenEvent *, void *userData) {
                auto data = ((em_websocket *)userData);
                data->websockets->data.push_back(
                    unique_ptr<em_websocket>(data)
                );
                return EM_TRUE;
            }
        );
        data.release();
    }

    websockets.connections.clear();
    
    for (auto &websocket : data) {
        if (websocket->data->up.readable) {
            auto &buffer = websocket->data->up.buffer;
            emscripten_websocket_send_binary(
                websocket->websocket, buffer.data(), buffer.size()
            );
            websocket->data->up.readable = false;
        }
    }
}