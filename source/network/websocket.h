#pragma once

#include <cinttypes>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <atomic>

struct client;

struct web_socket {
    web_socket() = default;
    web_socket(nullptr) {}
    web_socket(unsigned index) : id(index + 1) {}
    unsigned index() { return id - 1; }
    operator bool() const { return id != 0; }
    bool operator==(web_socket l, web_socket r) = default;
    unsigned id = 0;
};

typedef std::unique_ptr<web_socket, struct web_socket_deleter> 
    unique_web_socket;

struct web_sockets {
    web_sockets(
        unsigned connection_capacity, unsigned url_capacity,
        unsigned write_capacity, unsigned read_capacity
    );
    
    unique_web_socket try_connect(
        std::string_view url_string, std::vector<web_socket> &free_list
    );
    void close(std::vector<web_socket> &free_list);
    
    // need to pass around objects that store an array, a size and a capacity
    // and vector is the simplest for now
    // Leaves buffer empty if the last write isn't done yet
    void try_write(web_socket, std::vector<std::uint8_t> &buffer);
    std::vector<std::uint8_t> &try_get_read_buffer(web_socket);

    struct connection {
        std::vector<std::uint8_t> read;
        std::vector<std::uint8_t> write;
        std::string url;
        unsigned semaphore = 0;
    };
    std::vector<connection> connections;
    std::vector<web_socket> free_list;
};

struct web_socket_deleter {
    typedef web_socket pointer;
    void operator()(web_socket s) noexcept {
        free_list->push_back(s);
    }

    std::vector<web_socket> *free_list;
};
