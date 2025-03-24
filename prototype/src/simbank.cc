#include <pcsc-cpp/pcsc-cpp.hpp>
#include <print>
#include <string>
#include <tcp.hh>
#include <thread>

#include "atp.hh"

using namespace pcsc_cpp;
using namespace std::literals;

int main(int, char *[]) {
    std::println("hello, simbank!");

    auto readers = listReaders();

    for (auto reader : readers) {
        std::println("found a reader");
    }

    auto server = TcpServer{"127.0.0.1", 9000};

    auto read = std::jthread{[&server] {
        auto message = server.read();
        std::println("{}", static_cast<std::string>(message));
    }};

    auto data = Data{"hello"s};

    auto atp = Atp{
        1,
        1,
        data.raw.size(),
        static_cast<std::vector<std::byte>>(data),
    };

    server.write(static_cast<Data>(atp));

    return 0;
}
