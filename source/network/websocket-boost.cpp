#include "network_client.h"
#include "websocket.h"

#include <thread>

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/utility.hpp>

#include "../state/client.h"

bool check(boost::system::error_code error) {
    if (!error) {
        return false;
    }
    fprintf(stderr, "Error: %s\n", error.message().c_str());
    return true;
}

struct event_loop::data {
    data() : resolver(context), guard(boost::asio::make_work_guard(context)) {
        thread = std::thread([=](){ context.run(); });
    }
    boost::asio::io_context context;

    boost::asio::ip::tcp::resolver resolver;

    std::thread thread;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type>
        guard;
};

event_loop::event_loop() : d(new data()) {

}

event_loop::~event_loop() {
    d->guard.reset();
    d->thread.join();
}

struct websocket::data {
    data(boost::asio::io_context& context) : stream(context) {}
    boost::beast::websocket::stream<boost::asio::ip::tcp::socket> stream;
    boost::beast::flat_buffer buffer;
};

void read(websocket& websocket) {
    websocket.d->stream.async_read(
        websocket.d->buffer,
        [&](boost::beast::error_code error, size_t size) {
            if (check(error)) return;
            put_message(
                websocket.client,
                (const char*)websocket.d->buffer.cdata().data(), size
            );
            websocket.d->buffer.consume(websocket.d->buffer.size());
            read(websocket);
        }
    );
}

websocket::websocket(
    ::client& client, event_loop& loop, std::string_view host, unsigned port
) :
    client(client), loop(loop),
    d(new data(loop.d->context))
{
    loop.d->resolver.async_resolve(
        host.data(),
        std::string_view(std::to_string(port)),
        [=](boost::beast::error_code error, const auto results) {
            if (check(error)) return;
            boost::asio::async_connect(
                d->stream.next_layer(), results,
                [this, host](
                    boost::system::error_code error,
                    boost::asio::ip::tcp::endpoint
                ) {
                    if (check(error)) return;
                    d->stream.binary(true);
                    d->stream.async_handshake(
                        boost::string_view(host.data(), host.size()), "/",
                        [this](boost::beast::error_code error) {
                            if (check(error)) return;
                            // allow writing now
                            write_completed = true;
                            read(*this);
                        }
                    );
                }
            );
        }
    );
}

websocket::~websocket() {
    // TODO: sync with other operations
    boost::system::error_code error;
    d->stream.close(boost::beast::websocket::close_code::normal, error);
    check(error);
}

bool websocket::try_write_message(std::span<std::uint8_t> buffer) {
    if (write_completed) {
        next_message = buffer;
        write_completed = false;
        // need to post because stream is not thread safe
        loop.d->context.post([this](){
            d->stream.async_write(
                boost::asio::buffer(next_message.data(), next_message.size()),
                [this](boost::beast::error_code error, size_t) {
                    if (check(error)) return;
                    write_completed = true;
                }
            );
        });
        return true;
    }
    return false;
}

bool websocket::is_write_completed() {
    return write_completed;
}
