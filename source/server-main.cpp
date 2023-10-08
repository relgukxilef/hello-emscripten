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

struct session : public boost::intrusive_ref_counter<session> {
    // Need either ref counting or counting the number of outstanding operations
    session(boost::asio::ip::tcp::socket&& socket) : stream(std::move(socket)) {
    }
    boost::beast::http::request<boost::beast::http::string_body> request;
    boost::beast::websocket::stream<boost::asio::ip::tcp::socket> stream;
    boost::beast::flat_buffer buffer;
    glm::vec3 position;
    glm::quat orientation;
    // TODO: may need a queue for messages because async_write cannot be called
    // before the last async_write completed
};

struct server_t {
    server_t(boost::asio::io_context& context) :
        tick_timer(context, std::chrono::steady_clock::now())
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
};

server_t* server;

void read(boost::intrusive_ptr<session> session) {
    session->stream.async_read(session->buffer, [session](
        boost::beast::error_code error, size_t size
    ) {
        printf(
            "U WS Read %s.\n",
            boost::beast::buffers_to_string(session->buffer.cdata()).c_str()
        );
        if (error) {
            printf("Error %s.\n", error.message().c_str());
            return;
        }
        // TODO
        if (size >= message_size({{1}})) {
            auto message = parse_message({
                reinterpret_cast<std::uint8_t*>(
                    session->buffer.data().data()
                ),
                session->buffer.size()
            });
            if (message.users.position.size() == 1) {
                session->position = message.users.position[0];
                session->orientation = message.users.orientation[0];
            }
        }

        session->buffer.consume(session->buffer.size());
        read(std::move(session));
    });
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

    read(session);
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

        server->writes_pending = server->sessions.size();

        for (auto& session : server->sessions) {
            // TODO: use a ConstBufferSequence containing multiple continuous
            // buffers to only send the data this client needs
            message_header header[] = {{{
                static_cast<uint16_t>(server->tick_positions.size())
            }},};
            session->stream.async_write(
                std::array<boost::asio::const_buffer, 3>{
                    boost::asio::buffer(header),
                    boost::asio::buffer(server->tick_positions),
                    boost::asio::buffer(server->tick_orientations)
                },
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
