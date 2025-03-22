#include <iostream>
#include <string_view>
#include <exception>

#include <boost/process/v2.hpp>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>

using namespace std;
using namespace boost::asio;
using boost::process::v2::process;
using boost::process::v2::process_stdio;
using boost::filesystem::current_path;

boost::filesystem::path executable(string_view name) {
    auto path = current_path() / (string(name) + ".exe");
    if (boost::filesystem::exists(path))
        return path;
    return current_path() / name;
}

awaitable<void> forward_output(boost::asio::readable_pipe pipe) {
    while (true) {
        char c;
        co_await async_read(pipe, buffer(&c, 1), use_awaitable);
        // For reasons unknown to me, the server output seems to not be flushed
        cout << c << flush;
    }
}

awaitable<void> run(io_context &context) {
    boost::asio::readable_pipe pipe(context);
    process server(
        context, executable("server"), {}, process_stdio{.out = pipe}
    );
    
    string line;
    do {
        line.clear();
        char c;
        do {
            co_await async_read(pipe, buffer(&c, 1), use_awaitable);
            line += c;
        } while (c != '\n');
        cout << line << flush;
    } while (!line.starts_with("Running."));

    co_spawn(context, forward_output(move(pipe)), rethrow_exception);

    process client(
        context, executable("hello-vk"), 
        {"--server", "ws://localhost:28750/"}
    );

    auto result = co_await client.async_wait(use_awaitable);
    cout << "Client exited with " << result << endl;

    // request_exit is not supported on Windows
    server.interrupt();
    result = co_await server.async_wait(use_awaitable);
    cout << "Server exited with " << result << endl;
}

int main(int argc, char *argv[]) {
    io_context context;

    // process::interrupt is sent to all processes, so this will trigger when 
    // the client exits.
    boost::asio::signal_set signals(context, SIGINT, SIGTERM);
    signals.async_wait([&](const boost::system::error_code&, int) {
        signals.clear();
    });

    co_spawn(context, run(context), rethrow_exception);

    try {
        context.run();
    } catch (const exception &e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}
