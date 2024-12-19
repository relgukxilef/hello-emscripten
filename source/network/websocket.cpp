#include "websocket.h"

web_sockets::web_sockets(
    unsigned connection_capacity, unsigned url_capacity,
    unsigned write_capacity, unsigned read_capacity
) {
    connections.resize(connection_capacity);
    for (auto c : connections) {
        c.url.reserve(url_capacity);
        c.read.reserve(read_capacity);
        c.write.reserve(write_capacity);
    }
}

unique_web_socket web_sockets::try_connect(
    std::string_view url, std::vector<web_socket> &client_free_list
) {
    unsigned int index;
    for (index = 0; index < connections.size(); index++) {
        if (connections[index].url.empty())
            break;
    }
    web_socket socket = {index};
    connection &connection = connections[index];
    if (url.size() > connection.url.capacity())
        return {};
    connection.url.assign(url);
    connection.semaphore++;
    return unique_web_socket(socket, {&client_free_list});
}

void web_sockets::close(std::vector<web_socket> &client_free_list) {
    for (auto socket : client_free_list) {
        auto &connection = connections[socket.index()];
        connection.url.clear();
        connection.semaphore++;
    }
    client_free_list.clear();
}

bool web_sockets::can_write(web_socket socket) {
    return socket && write_buffer(socket).empty();
}

std::vector<uint8_t> &web_sockets::write_buffer(web_socket socket) {
    return connections[socket.index()].write;
}

bool web_sockets::can_read(web_socket socket) {
    return socket && read_buffer(socket).empty();
}

std::vector<uint8_t> &web_sockets::read_buffer(web_socket socket) {
    return connections[socket.index()].read;
}
