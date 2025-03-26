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

using namespace pcsc_cpp;
using namespace std::literals;

struct Exit {};

struct ReaderStateChange {
    std::string readerName;
};

using InternalMessage = std::variant<ReaderStateChange, Exit>;

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
        }
    }

    for (auto reader :
         readers | std::views::filter([&readerExistsInCards](auto reader) { return !readerExistsInCards(reader); })) {
        if (reader.isCardInserted()) {
            std::println("Card inserted into reader {}", reader.name);
            try {
                cards[reader.name] = reader.connectToCard();
                queue.emplace(ReaderStateChange{reader.name});
            } catch (...) {
                std::println(stderr, "Could not connect to card in reader {}", reader.name);
            }
        }
    }

    std::this_thread::yield();
}

auto handleReceive(TcpServer& server, Queue& queue, std::stop_token token) {
    if (token.stop_requested()) {
        return;
    }

    auto message = server.read<Atp>();
    if (message.length <= 0) {
        queue.emplace(Exit{});
        return;
    }

    std::println("Received {}", message);

    std::this_thread::yield();
}

auto handleReaderStateChange(TcpServer& server, Cards& cards, ReaderStateChange state) {
    auto atp = Atp{
        1,
        Type::Atr,
        GenericData{cards[state.readerName]->atr()},
    };

    std::println("{}", atp);
    server.write<GenericData>(atp);
}

auto handleExit(std::stop_source& stop_source) {
    if (stop_source.stop_possible()) {
        stop_source.request_stop();
    }
}

auto handleQueue(TcpServer& server, Queue& queue, Cards& cards, std::stop_source& stop_source) {
    if (queue.empty()) {
        std::this_thread::yield();
        return;
    }

    auto element = queue.front();
    queue.pop();

    std::visit(Overloads{[&server, &cards](ReaderStateChange state) { handleReaderStateChange(server, cards, state); },
                         [&stop_source](Exit) { handleExit(stop_source); }},
               element);
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

    auto receiveHandler = std::jthread{[&queue, &server](std::stop_token token) {
                                           while (!token.stop_requested()) {
                                               handleReceive(server, queue, token);
                                           }
                                       },
                                       stop_source.get_token()};

    while (!stop_source.stop_requested()) {
        handleQueue(server, queue, cards, stop_source);
    }

    // Make-shift "exit" signal to peer
    server.write<GenericData>(Atp{
        0,
        Type::Apdu,
        std::vector<std::byte>{},
    });

    return 0;
}
