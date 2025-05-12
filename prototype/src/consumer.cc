#include <cstring>
#include <iostream>
#include <print>
#include <queue>
#include <stop_token>
#include <thread>

#include "atp.hh"
#include "data.hh"
#include "simtrace.hh"
#include "tcp.hh"

using namespace std::literals;
using namespace stream;

struct CardRemoved {
    GenericData atr;
};

struct CardInserted {
    GenericData atr;
};

struct CardData {
    GenericData apdu;
};

template <class... Ts>
struct Overloads : Ts... {
    using Ts::operator()...;
};

using InternalMessage = std::variant<CardInserted, CardRemoved, CardData>;
using Queue           = std::queue<InternalMessage>;

auto handleQueue(SimTrace2 &trace, Queue queue, std::stop_token token) {
    if (token.stop_requested()) {
        return;
    }

    if (queue.empty()) {
        return;
    }

    auto element = queue.front();
    queue.pop();

    std::visit(Overloads{[&trace](CardInserted inserted) {
                             std::println("Card inserted");
                             trace.setCard(inserted.atr);
                         },
                         [&trace](CardRemoved) {
                             std::println("Card removed");
                             trace.unsetCard();
                         },
                         [](CardData data) { std::println("Card data received: {}", hexString(data.apdu)); }},
               element);

    trace.run();
}

auto pollMessage(RwStream<Atp, error::SystemError> auto &stream, Queue &queue, std::stop_source &stop_source) {
    auto message = stream.template read<Atp>();

    if (!message) {
        std::println(std::cerr, "Could not read from socket: {}", message.error());
        return;
    }

    std::println("Received {}", *message);

    switch (message->getType()) {
        using enum Type;
        case Apdu:
            std::println("Card APDU received");
            queue.emplace(CardData{message->data});
            break;
        case Atr:
            std::println("Card ATR received");
            queue.emplace(CardInserted{message->data});
            break;
        case UnsetCard:
            std::println("Card unset");
            queue.emplace(CardRemoved{message->data});
            break;
        case Stop:
            std::println("Exit signal received, exiting");
            stop_source.request_stop();
            break;
    }

    auto result = stream.template write<GenericData>(*message);
    if (!result) {
        std::println("Failed to transmit message {}. Reason: {}", *message, result.error());
    } else {
        std::println("Transmitted {}", *message);
    }
}

int main(int, char *[]) {
    auto stop_source = std::stop_source{};
    auto queue       = Queue{};

    auto trace = SimTrace2::create();

    if (!trace) {
        std::println(std::cerr, "Failed to construct SIMtrace instance, exiting");
        stop_source.request_stop();
    }

    auto client = TcpClient{"127.0.0.1", 9000};

    while (!stop_source.stop_requested()) {
        handleQueue(*trace, queue, stop_source.get_token());
        pollMessage(client, queue, stop_source);
        std::this_thread::sleep_for(10ms);
    }

    return 0;
}
