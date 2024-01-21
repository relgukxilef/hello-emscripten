#include <coroutine>
#include <thread>
#include <memory>
#include <atomic>
#include <optional>
#include <chrono>

#include <boost/lexical_cast.hpp>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/smart_ptr/intrusive_ref_counter.hpp>
#include <boost/range/adaptor/indexed.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "network/network_message.h"

enum { port = 28750 };
std::chrono::milliseconds tick_time{50};

unsigned message_user_capacity = 16;
unsigned message_audio_capacity = 200;

std::span<std::uint8_t> to_span(boost::beast::flat_buffer &buffer) {
    return {
        reinterpret_cast<std::uint8_t*>(buffer.data().data()), buffer.size()
    };
}

struct session : public boost::intrusive_ref_counter<session> {
    // Need either ref counting or counting the number of outstanding operations
    session(boost::asio::ip::tcp::socket&& socket) :
        stream(std::move(socket)), m(1, message_audio_capacity)
    {}
    boost::beast::http::request<boost::beast::http::string_body> request;
    boost::beast::websocket::stream<boost::asio::ip::tcp::socket> stream;
    boost::beast::flat_buffer buffer;
    glm::vec3 position;
    glm::quat orientation;
    message m;
    // TODO: may need a queue for messages because async_write cannot be called
    // before the last async_write completed
};

struct server_t {
    server_t(boost::asio::io_context& context) :
        tick_timer(context, std::chrono::steady_clock::now()),
        m(message_user_capacity, message_audio_capacity),
        buffer(capacity(m), 0)
    {}
    // Need one state that can be written when messages arrive
    // and one that can be read to send out messages
    // TODO: how to reallocate positions when users join?
    // on message: write to session positions
    // on tick: copy from session positions to world positions
    std::atomic_int writes_pending = 0;
    std::vector<glm::vec3> tick_positions;
    std::vector<glm::quat> tick_orientations;
    std::vector<boost::intrusive_ptr<session>> sessions;
    boost::asio::steady_timer tick_timer;
    message m;
    std::vector<std::uint8_t> buffer;
};

server_t* server;

boost::asio::awaitable<void> read(boost::intrusive_ptr<session> session) {
    boost::system::error_code error;
    auto completion_token =
        boost::asio::redirect_error(boost::asio::use_awaitable, error);

    while (true) {
        size_t size = co_await session->stream.async_read(
            session->buffer, completion_token
        );

        if (error) {
            printf("Error %s.\n", error.message().c_str());
            co_return;
        }

        if (session->buffer.size() > 0) {
            read(session->m, to_span(session->buffer));
            if (session->m.users.size != 1)
                co_return;
            if (session->m.users.size == 1) {
                auto &p = session->m.users.position;
                auto &o = session->m.users.orientation;
                session->position =
                    glm::vec3{p.x.values[0], p.y.values[0], p.z.values[0]};
                session->orientation = glm::normalize(glm::quat{
                    o.w.values[0],
                    o.x.values[0],
                    o.y.values[0],
                    o.z.values[0]
                });
            }
        }

        session->buffer.consume(session->buffer.size());
    }
}

boost::asio::awaitable<void> accept(
    boost::asio::io_context &context,
    boost::asio::ip::tcp::acceptor& acceptor
) {
    boost::system::error_code error;
    auto completion_token =
        boost::asio::redirect_error(boost::asio::use_awaitable, error);

    auto socket = co_await acceptor.async_accept(completion_token);

    printf(
        "T%s Accept %s.\n",
        boost::lexical_cast<std::string>(
            std::this_thread::get_id()
        ).c_str(),
        boost::lexical_cast<std::string>(
            socket.remote_endpoint()
        ).c_str()
    );

    if (error)
        co_return;

    co_spawn(context, accept(context, acceptor), boost::asio::detached);

    boost::intrusive_ptr<::session> session(
        new ::session(std::move(socket))
    );

    co_await async_read(
        session->stream.next_layer(), session->buffer, session->request,
        completion_token
    );

    printf(
        "T%s HTTP Read %s Method %s.\n",
        boost::lexical_cast<std::string>(
            std::this_thread::get_id()
        ).c_str(),
        boost::beast::buffers_to_string(
            session->buffer.cdata()
        ).c_str(),
        session->request.method_string().data()
    );

    if(error == boost::beast::http::error::end_of_stream) {
        co_return;
    } else if (error) {
        printf("Error %s.\n", error.message().c_str());
        co_return;
    }

    if (!boost::beast::websocket::is_upgrade(session->request)) {
        // TODO
        co_return;
    }

    co_await session->stream.async_accept(session->request, completion_token);

    printf(
        "T%s WS Accept.\n",
        boost::lexical_cast<std::string>(
            std::this_thread::get_id()
        ).c_str()
    );
    if (error) {
        printf(
            "Error %s.\n",
            error.message().c_str()
        );
        co_return;
    }

    session->stream.binary(true);

    server->sessions.push_back(session);

    co_await read(session);
}

void tick(boost::system::error_code error = {}) {
    if (error) return;

    if (server->writes_pending > 0) {
        printf("Tick skipped, last tick still in flight\n");
    } else {
        // TODO: instead of using remove_if use swap erase
        auto end = std::remove_if(
            server->sessions.begin(), server->sessions.end(),
            [](auto session){ return !session->stream.is_open(); }
        );

        server->sessions.erase(
            end, server->sessions.end()
        );

        server->tick_positions.resize(server->sessions.size());
        server->tick_orientations.resize(server->sessions.size());
        for (auto session : boost::adaptors::index(server->sessions)) {
            server->tick_positions[session.index()] = session.value()->position;
            server->tick_orientations[session.index()] =
                session.value()->orientation;
        }

        auto size = server->sessions.size();
        server->m.users.size = size;
        auto &p = server->m.users.position;
        for (auto s : boost::adaptors::index(server->tick_positions)) {
            p.x.values[s.index()] = s.value().x;
            p.y.values[s.index()] = s.value().y;
            p.z.values[s.index()] = s.value().z;
        }
        auto &o = server->m.users.orientation;
        for (auto s : boost::adaptors::index(server->tick_orientations)) {
            o.x.values[s.index()] = s.value().x;
            o.y.values[s.index()] = s.value().y;
            o.z.values[s.index()] = s.value().z;
            o.w.values[s.index()] = s.value().w;
        }

        if (size > 0) {
            server->m.audio_size = server->sessions[0]->m.audio_size;
            std::ranges::copy(
                server->sessions[0]->m.audio, server->m.audio.begin()
            );
        }

        write(server->m, server->buffer);

        server->writes_pending = size;

        for (auto& session : server->sessions) {
            session->stream.async_write(
                boost::asio::buffer(server->buffer),
                [](
                    boost::beast::error_code error, size_t
                ) {
                    server->writes_pending--;
                    if (error) {
                        printf("Error %s.\n", error.message().c_str());
                    }
                }
            );
        }
    }

    server->tick_timer = {
        server->tick_timer.get_executor(),
        std::max(
            server->tick_timer.expiry() + tick_time,
            std::chrono::steady_clock::now()
        )
    };
    server->tick_timer.async_wait(tick);
}

int main() {
    boost::asio::io_context context;

    server_t current_server(context);
    ::server = &current_server;

    boost::asio::ip::tcp::acceptor acceptor(
        context, boost::asio::ip::tcp::endpoint{{}, port}
    );

    acceptor.set_option(
        boost::asio::ip::tcp::acceptor::reuse_address(true)
    );

    acceptor.listen();

    co_spawn(context, accept(context, acceptor), boost::asio::detached);

    tick();

    // signal_set does not work on Windows

    printf("Running.\n");

    context.run();

    return 0;
}
