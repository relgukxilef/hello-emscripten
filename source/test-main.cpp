#include <boost/process.hpp>

int main(int argc, char *argv[]) {
    boost::process::child server(
        boost::process::search_path("server"),
        "--login.secret", "TEST_SECRET_e4B0PSdwUA==",
        "--login.pepper", "TEST_SECRET_8nCGAIn0RA=="
    );
    boost::process::child client(
        boost::process::search_path("hello-vk"),
        "--server", "ws://localhost:28750/"
    );

    client.wait();

    return 0;
}
