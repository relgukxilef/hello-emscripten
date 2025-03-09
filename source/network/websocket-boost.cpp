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
    boost_insecure_websocket(io_context &context, websocket_duplex duplex) : 
        stream(context), duplex(duplex) {}
    boost::beast::websocket::stream<ip::tcp::socket> stream;
    boost::beast::flat_buffer buffer;
    websocket_duplex duplex;
    bool reading = false, writing = false, closing = false;
};

boost_websockets::boost_websockets() : resolver(context) {
}

void boost_websockets::update() {
    context.poll();

    for (auto &websocket : websockets.websockets) {
        auto data = 
            std::make_shared<boost_insecure_websocket>(context, websocket);
        auto connect = [data, this]() -> awaitable<void> {
            auto c = use_awaitable;
            url_view url(data->duplex.receive->url);
            auto results = 
                co_await resolver.async_resolve(url.host, url.port, c);
            co_await async_connect(data->stream.next_layer(), results, c);
            data->stream.binary(true);
            co_await data->stream.async_handshake(url.host, url.path, c);
            readable.push_back(data);
            writable.push_back(data);
            closable.push_back(data);
        };
        co_spawn(context, std::move(connect), [](exception_ptr e) {
            if (e)
                rethrow_exception(e);
        });
    }
    websockets.websockets.clear();

    for (auto &data : closable) {
        if (!data->duplex.receive->closed)
            continue;

        if (data->closing)
            continue;

        data->closing = true;

        data->stream.async_close(
            boost::beast::websocket::close_code::normal,
            [data](auto ec) {
                data->duplex.send->closed = true;
                data->closing = false;
            }
        );
    }

    for (auto &data : readable) {
        auto buffer = data->duplex.writable();
        if (!buffer)
            continue;

        if (data->reading)
            continue;

        data->reading = true;

        data->stream.async_read(
            data->buffer,
            [data, this](boost::beast::error_code, std::size_t size) {
                data->duplex.send->buffer.assign(
                    (const uint8_t*)data->buffer.data().data(),
                    (const uint8_t*)data->buffer.data().data() + size
                );
                data->buffer.consume(size);
                readable.push_back(data);
                data->duplex.send->readable = true;
                data->reading = false;
            }
        );
    }

    for (auto &data : writable) {
        auto buffer = data->duplex.readable();
        if (!buffer)
            continue;

        if (data->writing)
            continue;

        data->writing = true;
        
        data->stream.async_write(
            boost::asio::buffer(*buffer),
            [data, this](boost::beast::error_code, std::size_t) {
                writable.push_back(data);
                data->duplex.receive->readable = false;
                data->writing = false;
            }
        );
    }
}

websockets boost_websockets::get_websockets() {
    return websockets;
}
