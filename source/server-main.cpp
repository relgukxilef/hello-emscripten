#include <thread>
#include <memory>
#include <atomic>
#include <optional>

#include <boost/lexical_cast.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

enum { port = 28750 };

void notify_thread(unsigned thread) {

}

template<class T>
struct read_locked {
    ~read_locked() {
        read_locks--;
    }

    T& t;
    std::atomic_uint& read_locks;
};

template<class T>
struct double_buffered {
    std::optional<read_locked<T>> try_read();
    T& write();
    void swap_or_replace();

    T buffers[2];
    std::atomic_uint read_locks = 0;
    std::atomic_bool write_buffer = 0;
};

struct session {
    session(boost::asio::ip::tcp::socket&& socket) :
        socket(std::move(socket)) {}
    boost::asio::ip::tcp::socket socket;
    boost::beast::http::request<boost::beast::http::string_body> request;
    std::optional<boost::beast::websocket::stream<boost::asio::ip::tcp::socket>>
        stream;
    boost::beast::flat_buffer buffer;
    // TODO: may need a queue for messages because async_write cannot be called
    // before the last async_write completed
};

void read(session& session) {
    session.stream->async_read(session.buffer, [&session](
        boost::beast::error_code error,
        size_t size
    ) {
        printf(
            "T%s WS Read %s.\n",
            boost::lexical_cast<std::string>(
                std::this_thread::get_id()
            ).c_str(),
            boost::beast::buffers_to_string(session.buffer.cdata()).c_str()
        );
        if (!error) {
            // TODO

            session.buffer.consume(session.buffer.size());
            read(session);
        } else {
            // TODO: sessions are never deleted
            printf("Error %s.\n", error.message().c_str());
        }
    });
}

void accept(
    boost::asio::ip::tcp::acceptor& acceptor, std::vector<session>& sessions
) {
    acceptor.async_accept(acceptor.get_executor(), [&acceptor, &sessions](
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
            sessions.push_back(session{std::move(socket)});
            auto& session = sessions.back();
            boost::beast::http::async_read(
                session.socket, session.buffer, session.request,
                [&session](
                    boost::beast::error_code error, std::size_t size
                ) {
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
                            session.stream.emplace(
                                std::move(session.socket)
                            );
                            session.stream->async_accept(
                                session.request,
                                [&session](
                                    boost::beast::error_code error
                                ) {
                                    printf(
                                        "T%s WS Accept.\n",
                                        boost::lexical_cast<std::string>(
                                            std::this_thread::get_id()
                                        ).c_str()
                                    );
                                    if (!error) {
                                        read(session);
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
                        session.socket.shutdown(
                            boost::asio::ip::tcp::socket::shutdown_send, error
                        );
                    } else {
                        printf("Error %s.\n", error.message().c_str());
                    }
                }
            );

            accept(acceptor, sessions);
        }
    });
}

int main() {
    boost::asio::io_context context;

    std::vector<session> sessions;

    boost::asio::ip::tcp::acceptor acceptor(
        context, boost::asio::ip::tcp::endpoint{{}, port}
    );

    acceptor.set_option(
        boost::asio::ip::tcp::acceptor::reuse_address(true)
    );

    acceptor.listen();

    accept(acceptor, sessions);

    // signal_set does not work on Windows

    printf("Running.\n");

    context.run();

    return 0;
}
