#include <boost/process.hpp>

int main(int argc, char *argv[]) {
    boost::process::child server(boost::process::search_path("server"));
    boost::process::child client(
        boost::process::search_path("hello-vk"),
        "--server", "ws://localhost:28750/"
    );

    client.wait();
    server.wait();

    return 0;
}
