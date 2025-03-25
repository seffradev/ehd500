#include <map>
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
#include "flag-set-cpp/flag_set.hpp"

using namespace pcsc_cpp;
using namespace std::literals;

struct InternalMessage {
    std::string readerName;
};

using Queue = std::queue<InternalMessage>;
using Cards = std::map<std::string, SmartCard::ptr>;

auto handleReaders(Queue& queue, Cards& cards, std::stop_token token) {
    if (token.stop_requested()) {
        return;
    }

    auto readers       = listReaders();
    auto readerChanged = [](Reader reader) { return static_cast<bool>(reader.status & Reader::Status::CHANGED); };

    for (auto reader : readers) {
        std::println("{}: {}", reader.name, reader.statusString());
    }

    for (auto reader : readers | std::views::filter(readerChanged)) {
        if (reader.isCardInserted()) {
            std::println("Card inserted into reader {}", reader.name);
            cards[reader.name] = reader.connectToCard();
            queue.emplace(reader.name);
        } else {
            std::println("Removing card from reader {}", reader.name);
            cards.erase(reader.name);
        }
    }

    std::this_thread::yield();
}

auto handleReceive(TcpServer& server, std::stop_token token) {
    if (token.stop_requested()) {
        return;
    }

    auto message = server.read<Atp>();
    std::println("Receiving {}", message);

    std::this_thread::yield();
}

auto handleQueue(TcpServer& server, Queue& queue, Cards& cards) {
    if (queue.empty()) {
        std::this_thread::yield();
        return;
    }

    auto element = queue.front();
    queue.pop();

    auto atp = Atp{
        1,
        Type::Atr,
        GenericData{cards[element.readerName]->atr()},
    };

    std::println("{}", atp);
    server.write<GenericData>(atp);
}

int main(int, char*[]) {
    auto queue       = std::queue<InternalMessage>{};
    auto cards       = std::map<std::string, SmartCard::ptr>{};
    auto stop_source = std::stop_source{};
    auto server      = TcpServer{"127.0.0.1", 9000};

    auto readerHandler = std::jthread{[&queue, &cards](std::stop_token token) {
                                          while (!token.stop_requested()) {
                                              handleReaders(queue, cards, token);
                                          }
                                      },
                                      stop_source.get_token()};

    auto receiveHandler = std::jthread{[&server](std::stop_token token) {
                                           while (!token.stop_requested()) {
                                               handleReceive(server, token);
                                           }
                                       },
                                       stop_source.get_token()};

    while (!stop_source.stop_requested()) {
        handleQueue(server, queue, cards);
    }

    // Make-shift "exit" signal to peer
    server.write<GenericData>(Atp{
        0,
        Type::Apdu,
        std::vector<std::byte>{},
    });

    return 0;
}
