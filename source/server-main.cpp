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
#include <boost/log/trivial.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/iostreams/stream.hpp>


#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <openssl/rand.h>

#include "network/network_message.h"
#include "network/utility.h"
#include "utility/serialization.h"

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
        stream(std::move(socket))
    {
        m.reset(1, message_audio_capacity);
    }
    boost::beast::http::request<boost::beast::http::string_body> request;
    boost::beast::websocket::stream<boost::asio::ip::tcp::socket> stream;
    boost::beast::flat_buffer buffer;
    message m;
    // TODO: may need a queue for messages because async_write cannot be called
    // before the last async_write completed
};

struct server_t {
    server_t(boost::asio::io_context& context) :
        tick_timer(context, std::chrono::steady_clock::now())
    {
        m.reset(message_user_capacity, message_audio_capacity);
        buffer.resize(capacity(m));
    }
    // Need one state that can be written when messages arrive
    // and one that can be read to send out messages
    // TODO: how to reallocate positions when users join?
    // on message: write to session positions
    // on tick: copy from session positions to world positions
    std::atomic_int writes_pending = 0;
    std::vector<boost::intrusive_ptr<session>> sessions;
    boost::asio::steady_timer tick_timer;
    std::ranges::subrange<const char *> login_secret;
    std::ranges::subrange<const char *> login_pepper;
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

        if (size > 0) {
            read(session->m, to_span(session->buffer));
            if (session->m.users.size != 1)
                co_return;
            // TODO: What if the last frame hasn't been sent yet?
        }

        session->buffer.consume(session->buffer.size());
    }
}

boost::asio::awaitable<void> accept(
    boost::asio::io_context &context,
    boost::asio::ip::tcp::acceptor &acceptor
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
        auto target = session->request.target();
        auto &body = session->request.body();

        std::ranges::subrange<char*> name, password;

        parse_form({body.data(), body.data() + body.size()}, {
            {"name", &name},
            {"password", &password},
        });

        boost::beast::http::response<boost::beast::http::string_body> response {
            boost::beast::http::status::created, session->request.version()
        };
        response.set(
            boost::beast::http::field::content_type, "application/json"
        );
        char response_buffer[512];

        range_stream response_body = std::ranges::subrange(response_buffer);

        // guest login
        append(
            response_body,
            "{\"name\":\"", name,
            // TODO: should guest ids come from a separate range?
            "\",\"id\":", 1234,
            ",\"role\":\"guest\""
            ",\"hash\":\""
        );

        char hash_buffer[32];
        std::ranges::subrange<char*> hash = hash_buffer;

        hash = {
            hash.begin(),
            write_password(hash, password, server->login_pepper)
        };

        assert(is_password_valid(hash, password, server->login_pepper));
        password[0]++;
        assert(!is_password_valid(hash, password, server->login_pepper));

        response_body.cursor = base64_encode(hash, response_body.right());

        append(
            response_body,
            "\"",
            ",\"bearer\":\""
        );

        response_body.cursor = jwt{
            0, unix_time() + 500
        }.write(
            {response_body.cursor, response_body.end()}, server->login_secret
        );

        append(response_body, "\"}");

        response.body().append(response_body.begin(), response_body.cursor);

        BOOST_LOG_TRIVIAL(info) << "User sign up";

        co_await async_write(
            session->stream.next_layer(), response, completion_token
        );

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

    // TODO: don't wait until the last tick was sent to everyone.
    // This would allow clients to stall the server.
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

        server->m.clear();
        for (auto &session : server->sessions) {
            server->m.append(session->m);
        }

        write(server->m, server->buffer);

        server->writes_pending = server->sessions.size();

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

std::ranges::subrange<const char *> base64_argument(
    char *arg, std::unique_ptr<char[]> &storage
) {
    auto end = std::find(arg, arg + strlen(arg), '=');
    auto length = (end - arg) * 3 / 4;
    storage = std::make_unique<char[]>(length);
    base64_decode({arg, end}, {storage.get(), storage.get() + length});
    return {storage.get(), storage.get() + length};
}

int main(int argc, char *argv[]) {
    boost::asio::io_context context;

    server_t current_server(context);
    ::server = &current_server;

    std::unique_ptr<char[]> secret, pepper;

    for (auto arg = argv; *arg != nullptr; arg++) {
        if (strcmp(*arg, "--login.secret") == 0) {
            arg++;
            if (*arg != nullptr) {
                // TODO: the command line argument should hold a path to a file
                // containing the secret, instead of the secret
                // TODO: create a login module that stores this secret
                server->login_secret = base64_argument(*arg, secret);
            }
        } else if (strcmp(*arg, "--login.pepper") == 0) {
            arg++;
            if (*arg != nullptr) {
                server->login_pepper = base64_argument(*arg, pepper);
            }
        }
    }

    assert(!server->login_secret.empty());

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
