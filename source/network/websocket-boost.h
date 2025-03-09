#pragma once

#include <vector>
#include <memory>

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/utility.hpp>
#include <boost/beast/ssl.hpp>

#include "websocket.h"

struct boost_insecure_websocket;

struct boost_websockets {
    boost_websockets();
    void update();
    websockets get_websockets();

    websockets websockets;
    boost::asio::io_context context;
    boost::asio::ip::tcp::resolver resolver;
    std::vector<std::shared_ptr<boost_insecure_websocket>> 
        readable, writable, closable;
};
