#include <optional>
#include <simbank.hh>
#include <stop_token>
#include <tcp.hh>
#include <thread>

using namespace std::literals;

int main(int, char*[]) {
    auto server     = std::optional<TcpServer>{std::nullopt};
    auto stopSource = std::stop_source{};

    try {
        server = TcpServer{"127.0.0.1", 9000};
        std::println("Server ready");
    } catch (SocketException& e) {
        std::println(std::cerr, "Failed to start server: {}", e.what());
    }

    if (!server) {
        stopSource.request_stop();
    }

    auto simbank = SimBank{};
    while (!stopSource.stop_requested()) {
        simbank.run(*server, stopSource);
        std::this_thread::sleep_for(10ms);
    }

    return 0;
}
