#include "network_client.h"

#include <algorithm>
#include <string>

void put_message(client& client, const char* buffer, size_t size) {
    std::string s(buffer, size);
    for (char& c : s)
        if (c == 0)
            c = '.';

    if (!client.message_in_readable) {
        client.in_buffer.resize(size);
        std::move(buffer, buffer + size, client.in_buffer.data());
        client.message_in_readable = true;

    } else {
        fprintf(stdout, "Message skipped\n");
    }
}

void set_disconnected(client& client) {

}
