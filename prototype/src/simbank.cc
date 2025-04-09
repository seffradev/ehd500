#include <map>
#include <optional>
#include <pcsc-cpp/pcsc-cpp.hpp>
#include <print>
#include <queue>
#include <ranges>
#include <stop_token>
#include <string>
#include <tcp.hh>
#include <thread>

#include "atp.hh"
#include "data.hh"

using namespace pcsc_cpp;
using namespace std::literals;

struct Exit {};

struct CardRemoved {
    std::string readerName;
};

struct CardInserted {
    std::string readerName;
};

using InternalMessage = std::variant<CardInserted, CardRemoved, Exit>;

template <class... Ts>
struct Overloads : Ts... {
    using Ts::operator()...;
};

using Queue = std::queue<InternalMessage>;
using Cards = std::map<std::string, SmartCard::ptr>;

auto handleReaders(Queue& queue, Cards& cards, std::stop_token token) {
    if (token.stop_requested()) {
        return;
    }

    auto readers             = listReaders();
    auto readerExistsInCards = [&cards](Reader reader) { return cards.find(reader.name) != cards.end(); };

    for (auto reader : readers | std::views::filter(readerExistsInCards)) {
        if (!reader.isCardInserted()) {
            std::println("Removing card from reader {}", reader.name);
            cards.erase(reader.name);
            queue.emplace(CardRemoved{reader.name});
        }
    }

    for (auto reader :
         readers | std::views::filter([&readerExistsInCards](auto reader) { return !readerExistsInCards(reader); })) {
        if (reader.isCardInserted()) {
            std::println("Card inserted into reader {}", reader.name);
            try {
                cards[reader.name] = reader.connectToCard();
                queue.emplace(CardInserted{reader.name});
            } catch (...) {
                std::println(std::cerr, "Could not connect to card in reader {}", reader.name);
            }
        }
    }
}

auto handleReceive(TcpServer& server, Queue& queue, std::stop_token token) {
    if (token.stop_requested()) {
        return;
    }

    auto message = server.read<Atp>();
    if (!message) {
        std::println(std::cerr, "Could not read from socket: {}", message.error());
        return;
    }

    std::println("Received {}", *message);

    if (message->getType() == Type::Stop) {
        queue.emplace(Exit{});
        return;
    }
}

auto handleCardInserted(TcpServer& server, Cards& cards, CardInserted state) {
    std::println("Card inserted");

    auto message = Atp{
        1,
        Type::Atr,
        GenericData{cards[state.readerName]->atr()},
    };

    auto result = server.write<GenericData>(message);
    if (!result) {
        std::println("Failed to transmit message {}. Reason: {}", message, result.error());
    } else {
        std::println("Transmitted {}", message);
    }
}

auto handleCardRemoved(TcpServer& server) {
    std::println("Card removed");

    auto message = Atp{
        1,
        Type::UnsetCard,
        GenericData{std::vector<std::byte>{}},
    };

    auto result = server.write<GenericData>(message);
    if (!result) {
        std::println("Failed to transmit message {}. Reason: {}", message, result.error());
    } else {
        std::println("Transmitted {}", message);
    }
}

auto handleExit(std::stop_source& stop_source) {
    std::println("Exit signal received, exiting");
    stop_source.request_stop();
}

auto handleQueue(TcpServer& server, Queue& queue, Cards& cards, std::stop_source& stop_source) {
    if (queue.empty()) {
        return;
    }

    auto element = queue.front();
    queue.pop();

    std::visit(Overloads{[&server, &cards](CardInserted state) { handleCardInserted(server, cards, state); },
                         [&server](CardRemoved) { handleCardRemoved(server); },
                         [&stop_source](Exit) { handleExit(stop_source); }},
               element);
}

int main(int, char*[]) {
    auto queue       = Queue{};
    auto cards       = Cards{};
    auto stop_source = std::stop_source{};

    auto server = std::optional<TcpServer>{std::nullopt};

    try {
        server = TcpServer{"127.0.0.1", 9000};
        std::println("Server ready");
    } catch (SocketException& e) {
        std::println(std::cerr, "Failed to start server: {}", e.what());
    }

    if (!server) {
        stop_source.request_stop();
    }

    // auto readerHandler = std::jthread{[&queue, &cards](std::stop_token token) {
    //                                       while (!token.stop_requested()) {
    //                                           handleReaders(queue, cards, token);
    //                                       }
    //                                   },
    //                                   stop_source.get_token()};

    // auto serverHandler = std::jthread{[&queue, &server](std::stop_token token) {
    //                                       while (!token.stop_requested()) {
    //                                           handleReceive(*server, queue, token);
    //                                       }
    //                                   },
    //                                   stop_source.get_token()};

    while (!stop_source.stop_requested()) {
        handleQueue(*server, queue, cards, stop_source);
        handleReceive(*server, queue, stop_source.get_token());
        handleReaders(queue, cards, stop_source.get_token());
        std::this_thread::sleep_for(10ms);
    }

    return 0;
}
