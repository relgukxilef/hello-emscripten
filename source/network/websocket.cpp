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
    free_list.resize(connection_capacity);
    for (auto i = 0u; i < connection_capacity; i++) {
        free_list[i] = i;
    }
}

unique_web_socket web_sockets::try_connect(
    std::string_view url, std::vector<web_socket> &client_free_list
) {
    if (free_list.empty())
        return {};
    web_socket socket = {free_list.back()};
    connection &connection = connections[socket.index()];
    if (url.size() > connection.url.capacity())
        return {};
    connection.url.assign(url);
    connection.semaphore++;
    return unique_web_socket(socket, {&client_free_list});
}

void web_sockets::close(std::vector<web_socket> &client_free_list) {
    for (auto socket : client_free_list) {
        free_list.push_back(socket);
        auto &connection = connections[socket.index()];
        connection.url.clear();
        connection.semaphore++;
    }
    client_free_list.clear();
}

std::vector<std::uint8_t> web_sockets::try_prepare(web_socket socket) {
    return std::move(connections[socket.index()].write);
}

void web_sockets::write(web_socket socket, std::vector<std::uint8_t> buffer) {
    std::swap(connections[socket.index()].write, buffer);
}

std::vector<std::uint8_t> web_sockets::try_consume(web_socket socket) {
    return std::move(connections[socket.index()].read);
}

void web_sockets::read(web_socket socket, std::vector<std::uint8_t> buffer) {
    std::swap(connections[socket.index()].write, buffer);
}
