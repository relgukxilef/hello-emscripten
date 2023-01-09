#include "network_client.h"
#include "websocket.h"

#include <thread>

#include "../state/client.h"

struct event_loop::data {
    data() {
        
    }
};

event_loop::event_loop() : d(new data()) {

}

event_loop::~event_loop() {
}

struct websocket::data {
    data() {}
};

void read(websocket& websocket) {
    
}

websocket::websocket(
    ::client& client, event_loop& loop, std::string_view host, unsigned port
) :
    client(client), loop(loop),
    d(new data)
{
    
}

websocket::~websocket() {
    // TODO: sync with other operations
    
}

bool websocket::try_write_message(std::span<std::uint8_t> buffer) {
    
}

bool websocket::is_write_completed() {
    return write_completed;
}
