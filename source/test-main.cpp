#include <iostream>
#include <string_view>

#include <boost/process/v2.hpp>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/process/v2/windows/creation_flags.hpp>

using namespace std;
using namespace boost::asio;
using boost::process::v2::process;
using boost::filesystem::current_path;

boost::filesystem::path executable(string_view name) {
    auto path = current_path() / (string(name) + ".exe");
    if (boost::filesystem::exists(path))
        return path;
    return current_path() / name;
}

awaitable<void> run(io_context &context) {
    try {
        process server(context, executable("server"), {});
        // TODO: wait for server to be ready
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

    } catch (const exception &e) {
        cerr << "Error: " << e.what() << endl;
    }
}

int main(int argc, char *argv[]) {
    io_context context;

    // process::interrupt is sent to all processes, so this will trigger when 
    // the client exits.
    boost::asio::signal_set signals(context, SIGINT, SIGTERM);
    signals.async_wait([&](const boost::system::error_code&, int) {});

    co_spawn(context, run(context), detached);

    context.run();

    return 0;
}
