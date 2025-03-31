#include <cstring>
#include <exception>
#include <iostream>
#include <print>
#include <stop_token>
#include <thread>

#include "atp.hh"
#include "data.hh"
#include "simtrace.hh"
#include "tcp.hh"

using namespace std::literals;

auto handleTrace(SimTrace2 &trace, std::stop_token token) {
    if (token.stop_requested()) {
        return;
    }

    trace.run();
}

int main(int, char *[]) {
    auto stop_source = std::stop_source{};
    auto trace       = SimTrace2{};

    try {
        trace.connect();
    } catch (std::exception &e) {
        std::println(std::cerr, "Failed to construct SIMtrace instance, exiting. Cause: {}", e.what());
        return 1;
    }

    auto client = TcpClient{"127.0.0.1", 9000};

    auto traceHandler = std::jthread{[&trace](std::stop_token token) {
                                         while (!token.stop_requested()) {
                                             handleTrace(trace, token);
                                         }
                                     },
                                     stop_source.get_token()};

    while (!stop_source.stop_requested()) {
        auto message = client.read<Atp>();
        std::println("Received {}", message);

        if (message.length <= 0 && stop_source.stop_possible()) {
            stop_source.request_stop();
            continue;
        }

        switch (message.getType()) {
            using enum Type;
            case Apdu:
                break;
            case Atr:
                trace.setCard(message.data);
                break;
            case UnsetCard:
                trace.unsetCard();
                break;
        }

        client.write<GenericData>(message);
        std::println("Transmitted {}", message);
    }

    return 0;
}
