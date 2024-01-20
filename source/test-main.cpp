#include <boost/process.hpp>

int main(int argc, char *argv[]) {
    boost::process::child server(boost::process::search_path("server"));
    boost::process::child client(
        boost::process::search_path("hello-gl"),
        "--server", "ws://localhost:80/"
    );

    client.wait();
    server.wait();

    return 0;
}
