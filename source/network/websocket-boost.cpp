#include "network_client.h"
#include "websocket.h"

#include <source_location>
#include <thread>

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/utility.hpp>
#include <boost/beast/ssl.hpp>

#include "../state/client.h"

struct url_view
{
    url_view() = default;
    url_view(std::string_view url);
    std::string_view scheme, host, port, path;
};

struct websocket::data {
    data(client &c) : c(c) {}
    virtual ~data() = default;
    virtual void read() = 0;
    virtual void try_write_message(websocket &s) = 0;
    std::atomic_bool write_completed = false;
    client &c;
};

struct insecure_websocket : public websocket::data {
    void read() override;
    void try_write_message(websocket& s) override;

    insecure_websocket(client &c, event_loop &loop, url_view url);
    ~insecure_websocket();
    boost::beast::websocket::stream<boost::asio::ip::tcp::socket> stream;
    boost::beast::flat_buffer buffer;
    boost::asio::io_context& context;
};

struct secure_websocket : public websocket::data {
    void read() override;
    void try_write_message(websocket& s) override;

    secure_websocket(client& c, event_loop& loop, url_view url);
    ~secure_websocket();
    boost::asio::ssl::context ssl_context;
    boost::beast::websocket::stream<
        boost::beast::ssl_stream<boost::asio::ip::tcp::socket>
    > stream;
    boost::beast::flat_buffer buffer;
    boost::asio::io_context& context;
};

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

void read(websocket& websocket) {
    websocket.d->read();
}

websocket::websocket(::client& c, event_loop& loop, std::string_view url) :
    loop(loop)
{
    if (url.starts_with("ws:"))
        d.reset(new insecure_websocket(c, loop, url));
    else if (url.starts_with("wss:"))
        d.reset(new secure_websocket(c, loop, url));
    else
        throw std::runtime_error(
            "Unsupported url " + std::string(url)
        );
}

websocket::~websocket() {
    // empty destructor needs to exist because ~data is unknown outside of this
    // translation unit
}

bool websocket::try_write_message(std::span<std::uint8_t> buffer) {
    if (d->write_completed) {
        next_message = buffer;
        d->write_completed = false;
        // need to post because stream is not thread safe
        loop.d->context.post([this](){
            d->try_write_message(*this);
        });
        return true;
    }
    return false;
}

bool websocket::is_write_completed() {
    return d->write_completed;
}

url_view::url_view(std::string_view url) {
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

void insecure_websocket::read() {
    stream.async_read(
        buffer,
        [&](boost::beast::error_code error, size_t size) {
            if (check(error)) return;
            put_message(
                c,
                (const char*)buffer.cdata().data(), size
            );
            buffer.consume(buffer.size());
            read();
        }
    );
}

void insecure_websocket::try_write_message(websocket& s) {
    stream.async_write(
        boost::asio::buffer(s.next_message.data(), s.next_message.size()),
        [&s](boost::beast::error_code error, size_t) {
            if (check(error)) return;
            s.d->write_completed = true;
        }
    );
}

insecure_websocket::insecure_websocket(
    client& c, event_loop& loop, url_view url
) :
    websocket::data(c), stream(loop.d->context), context(loop.d->context)
{
    // post because resolver is not thread-safe
    context.post([url, &loop, this] () {
        loop.d->resolver.async_resolve(
            url.host,
            url.port,
            [=](boost::beast::error_code error, const auto results) {
                if (check(error)) return;
                boost::asio::async_connect(
                    stream.next_layer(), results,
                    [this, url](
                        boost::system::error_code error,
                        boost::asio::ip::tcp::endpoint
                    ) {
                        if (check(error)) return;
                        stream.binary(true);
                        stream.async_handshake(
                            boost::string_view(
                                url.host.data(), url.host.size()
                            ),
                            boost::string_view(
                                url.path.data(), url.path.size()
                            ),
                            [this](boost::beast::error_code error) {
                                if (check(error)) return;
                                // allow writing now
                                write_completed = true;
                                read();
                            }
                        );
                    }
                );
            }
        );
    });
}

insecure_websocket::~insecure_websocket() {
    std::promise<void> promise;
    auto future = promise.get_future();
    boost::asio::post(stream.get_executor(), [this, &promise] () {
        stream.async_close(
            boost::beast::websocket::close_code::normal,
            [&promise](const boost::system::error_code& error) {
                check(error);
                promise.set_value();
            }
        );
    });
    future.wait();
}

void secure_websocket::read() {
    stream.async_read(
        buffer,
        [&](boost::beast::error_code error, size_t size) {
            if (check(error)) return;
            put_message(
                c,
                (const char*)buffer.cdata().data(), size
            );
            buffer.consume(buffer.size());
            read();
        }
    );
}

void secure_websocket::try_write_message(websocket& s) {
    stream.async_write(
        boost::asio::buffer(s.next_message.data(), s.next_message.size()),
        [&s](boost::beast::error_code error, size_t) {
            if (check(error)) return;
            s.d->write_completed = true;
        }
    );
}

secure_websocket::secure_websocket(
    client& c, event_loop& loop, url_view url
) :
    websocket::data(c), ssl_context{boost::asio::ssl::context::tlsv12_client},
    stream(loop.d->context, ssl_context), context(loop.d->context)
{
    // post because resolver is not thread-safe
    context.post([url, &loop, this] () {
        loop.d->resolver.async_resolve(
            url.host,
            url.port,
            [=](boost::beast::error_code error, const auto results) {
                if (check(error)) return;
                boost::asio::async_connect(
                    get_lowest_layer(stream), results,
                    [this, url](
                        boost::system::error_code error,
                        boost::asio::ip::tcp::endpoint
                    ) {
                        if (check(error)) return;
                        stream.binary(true);
                        stream.async_handshake(
                            boost::string_view(
                                url.host.data(), url.host.size()
                            ),
                            boost::string_view(
                                url.path.data(), url.path.size()
                            ),
                            [this](boost::beast::error_code error) {
                                if (check(error)) return;
                                // allow writing now
                                write_completed = true;
                                read();
                            }
                        );
                    }
                );
            }
        );
    });
}

secure_websocket::~secure_websocket() {
    // TODO: sync with other operations
    std::promise<void> promise;
    auto future = promise.get_future();
    boost::asio::post(stream.get_executor(), [this, &promise] () {
        stream.async_close(
            boost::beast::websocket::close_code::normal,
            [&promise](const boost::system::error_code& error) {
                check(error);
                promise.set_value();
            }
        );
    });
    future.wait();
}
