#include <thread>
#include <memory>
#include <atomic>
#include <optional>

#include <boost/lexical_cast.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include "network/server.h"

enum { port = 28750 };

struct session {
    session(boost::asio::ip::tcp::socket&& socket) : stream(std::move(socket)) {
    }
    boost::beast::http::request<boost::beast::http::string_body> request;
    boost::beast::websocket::stream<boost::asio::ip::tcp::socket> stream;
    boost::beast::flat_buffer buffer;
    // TODO: may need a queue for messages because async_write cannot be called
    // before the last async_write completed
};

std::vector<glm::vec3>* positions;
std::vector<session>* sessions;

void read(unsigned session_index, unsigned user_index);

void write(unsigned session_index, unsigned user_index);

void read(unsigned session_index, unsigned user_index) {
    auto& session = (*sessions)[session_index];
    session.stream.async_read(session.buffer, [session_index, user_index](
        boost::beast::error_code error, size_t size
    ) {
        auto& session = (*sessions)[session_index];
        printf(
            "U%d WS Read %s.\n",
            user_index,
            boost::beast::buffers_to_string(session.buffer.cdata()).c_str()
        );
        if (!error) {
            // TODO
            if (size >= sizeof(glm::vec3)) {
                (*positions)[user_index] = *reinterpret_cast<const glm::vec3*>(
                    session.buffer.cdata().data()
                );
            }

            session.buffer.consume(session.buffer.size());
            write(session_index, user_index);
        } else {
            // TODO: sessions are never deleted
            printf("Error %s.\n", error.message().c_str());
        }
    });
}

void write(unsigned session_index, unsigned user_index) {
    auto& session = (*sessions)[session_index];

    session.stream.async_write(
        boost::asio::buffer(*positions), [session_index, user_index](
            boost::beast::error_code error, size_t size
        ) {
            auto& session = (*sessions)[session_index];
            printf("U%d WS Write.\n", user_index);
            if (!error) {
                read(session_index, user_index);
            } else {
                // TODO: sessions are never deleted
                printf("Error %s.\n", error.message().c_str());
            }
        }
    );
}

void accept(
    boost::asio::ip::tcp::acceptor& acceptor
) {
    acceptor.async_accept(acceptor.get_executor(), [&acceptor](
        boost::system::error_code error,
        boost::asio::ip::tcp::socket&& socket
    ) {
        printf(
            "T%s Accept %s.\n",
            boost::lexical_cast<std::string>(
                std::this_thread::get_id()
            ).c_str(),
            boost::lexical_cast<std::string>(
                socket.remote_endpoint()
            ).c_str()
        );

        if (!error) {
            unsigned session_index = sessions->size();
            sessions->push_back(std::move(socket));
            auto& session = sessions->back();
            boost::beast::http::async_read(
                session.stream.next_layer(), session.buffer, session.request,
                [session_index](
                    boost::beast::error_code error, std::size_t size
                ) {
                    auto& session = (*sessions)[session_index];
                    printf(
                        "T%s HTTP Read %s.\n",
                        boost::lexical_cast<std::string>(
                            std::this_thread::get_id()
                        ).c_str(),
                        boost::beast::buffers_to_string(
                            session.buffer.cdata()
                        ).c_str()
                    );
                    if (!error) {
                        if (
                            boost::beast::websocket::is_upgrade(session.request)
                        ) {
                            auto user_index = positions->size();
                            positions->push_back({});
                            session.stream.async_accept(
                                session.request,
                                [session_index, user_index](
                                    boost::beast::error_code error
                                ) {
                                    auto& session = (*sessions)[session_index];
                                    printf(
                                        "T%s WS Accept.\n",
                                        boost::lexical_cast<std::string>(
                                            std::this_thread::get_id()
                                        ).c_str()
                                    );
                                    if (!error) {
                                        read(session_index, user_index);
                                    } else {
                                        printf(
                                            "Error %s.\n",
                                            error.message().c_str()
                                        );
                                    }
                                }
                            );
                        } else {
                            // TODO
                        }

                    } else if(
                        error == boost::beast::http::error::end_of_stream
                    ) {
                        session.stream.next_layer().shutdown(
                            boost::asio::ip::tcp::socket::shutdown_send, error
                        );
                    } else {
                        printf("Error %s.\n", error.message().c_str());
                    }
                }
            );

            accept(acceptor);
        }
    });
}

int main() {
    boost::asio::io_context context;

    std::vector<glm::vec3> positions;
    ::positions = &positions;
    std::vector<session> sessions;
    ::sessions = &sessions;

    boost::asio::ip::tcp::acceptor acceptor(
        context, boost::asio::ip::tcp::endpoint{{}, port}
    );

    acceptor.set_option(
        boost::asio::ip::tcp::acceptor::reuse_address(true)
    );

    acceptor.listen();

    accept(acceptor);

    // signal_set does not work on Windows

    printf("Running.\n");

    context.run();

    return 0;
}
