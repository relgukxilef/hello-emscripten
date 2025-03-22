#include "websocket-boost.h"

using namespace std;
using namespace boost::asio;

struct url_view {
    url_view() = default;
    url_view(std::string_view url);
    std::string_view scheme, host, port, path;
};

url_view::url_view(std::string_view url) {
    // TODO: crashes when path is empty
    auto colon1 = url.find(":");
    scheme = url.substr(0, colon1);
    url = url.substr(colon1 + 1);
    if (url.starts_with("//")) {
        url = url.substr(2);
        auto authority_end = url.find_first_of("/?#");
        auto authority = url.substr(0, authority_end);
        url = url.substr(authority_end);
        auto at = authority.find_last_of("@");
        if (at != std::string_view::npos) {
            authority = authority.substr(at);
        }
        auto colon2 = authority.find(":");
        if (colon2 != std::string_view::npos) {
            port = authority.substr(colon2 + 1);
            authority = authority.substr(0, colon2);
        }
        host = authority;
    }
    path = url;
}

bool check(
    boost::system::error_code error,
    const std::source_location location = std::source_location::current()
) {
    if (!error) {
        return false;
    }
    fprintf(
        stderr, "Error %s:%u %s\n",
        location.file_name(), location.line(), error.message().c_str()
    );
    return true;
}

struct boost_insecure_websocket {
    boost_insecure_websocket(
        io_context &context, std::shared_ptr<websocket_data> data
    ) : 
        stream(context), data(data) {}
    boost::beast::websocket::stream<ip::tcp::socket> stream;
    boost::beast::flat_buffer buffer;
    std::shared_ptr<websocket_data> data;
};

boost_websockets::boost_websockets() : 
    resolver(make_shared<ip::tcp::resolver>(context)) 
{
}

boost_websockets::~boost_websockets() {
    resolver->cancel();
    for (auto &socket : sockets) {
        socket->data->up.closed = true;
        // TODO: maybe cancel socket
    }
    context.run();
}

struct {
    void operator()(std::exception_ptr e) {
        if (e) {
            std::rethrow_exception(e);
        }
    }
} checked;

awaitable<void> read(
    io_context &context,
    std::shared_ptr<boost_insecure_websocket> data,
    fence &update
) {
    while (true) {
        co_await update.async_wait(use_awaitable);
        if (data->data->up.closed) {
            co_return;
        }
        if (data->data->down.readable) {
            continue;
        }
        auto size = co_await data->stream.async_read(
            data->buffer,
            use_awaitable
        );
        data->data->down.buffer.assign(
            (const uint8_t*)data->buffer.data().data(),
            (const uint8_t*)data->buffer.data().data() + size
        );
        data->buffer.consume(size);
        data->data->down.readable = true;
    }
}

awaitable<void> write(
    io_context &context,
    std::shared_ptr<boost_insecure_websocket> data,
    fence &update
) {
    while (true) {
        co_await update.async_wait(use_awaitable);
        if (data->data->up.closed) {
            co_return;
        }
        if (!data->data->up.readable) {
            continue;
        }
        co_await data->stream.async_write(
            boost::asio::buffer(data->data->up.buffer),
            use_awaitable
        );
        data->data->up.readable = false;
    }
}

awaitable<void> connect(
    io_context &context,
    std::shared_ptr<boost_insecure_websocket> data,
    std::shared_ptr<ip::tcp::resolver> resolver,
    fence &update
) {
    auto c = use_awaitable;
    url_view url(data->data->url);
    auto results = 
        co_await resolver->async_resolve(url.host, url.port, c);
    co_await async_connect(data->stream.next_layer(), results, c);
    data->stream.binary(true);
    co_await data->stream.async_handshake(url.host, url.path, c);

    co_spawn(context, read(context, data, update), checked);
    co_await write(context, data, update);

    co_await data->stream.async_close(
        boost::beast::websocket::close_code::going_away, c
    );
}

void boost_websockets::update() {
    update_fence.signal();
    context.poll();

    for (auto &websocket : websockets.connections) {
        auto data = 
            make_shared<boost_insecure_websocket>(context, websocket);
        sockets.push_back(data);
        co_spawn(
            context, connect(context, data, resolver, update_fence), checked
        );
    }
    websockets.connections.clear();
}

websockets boost_websockets::get_websockets() {
    return websockets;
}
