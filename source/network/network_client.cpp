#include "network_client.h"

#include <algorithm>
#include <string>

#include "../utility/trace.h"

void put_message(client& client, const char* buffer, size_t size) {
    scope_trace trace;
    if (!client.message_in_readable) {
        client.in_buffer.resize(size);
        std::move(buffer, buffer + size, client.in_buffer.data());
        client.message_in_readable = true;

    } else {
    }
}

void set_disconnected(client& client) {

}
