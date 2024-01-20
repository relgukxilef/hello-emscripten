#include "network_client.h"

#include <algorithm>
#include <string>

#include "../utility/trace.h"

void put_message(client& client, const char* buffer, size_t size) {
    scope_trace trace;
    std::string s(buffer, size);
    for (char& c : s)
        if (c == 0)
            c = '.';

    if (!client.message_in_readable) {
        client.message_in.resize(size);
        std::move(buffer, buffer + size, client.message_in.data());
        client.message_in_readable = true;

    } else {
        fprintf(stdout, "Message skipped\n");
    }
}

void set_disconnected(client& client) {

}
