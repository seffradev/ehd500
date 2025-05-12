#ifndef SIMBANK_HH
#define SIMBANK_HH

#include <map>
#include <pcsc-cpp/pcsc-cpp.hpp>
#include <print>
#include <queue>
#include <stop_token>

#include "atp.hh"
#include "exception.hh"

class SimBank {
public:
    constexpr SimBank() : queue(Queue{}), cards(Cards{}) {}

    constexpr auto run(stream::RwStream<Atp, error::SystemError> auto& stream, std::stop_source& stopSource) {
        handleQueue(stream, queue, cards, stopSource);
        handleReceive(stream, queue, stopSource.get_token());
        handleReaders(queue, cards, stopSource.get_token());
    }

private:
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
    using Cards = std::map<std::string, pcsc_cpp::SmartCard::ptr>;

    Queue queue;
    Cards cards;

    static constexpr auto handleReaders(Queue& queue, Cards& cards, std::stop_token token) -> void {
        using namespace pcsc_cpp;

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

        for (auto reader : readers | std::views::filter([&readerExistsInCards](auto reader) { return !readerExistsInCards(reader); })) {
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

    static constexpr auto handleReceive(stream::ReadStream<Atp, error::SystemError> auto& server, Queue& queue, std::stop_token token)
        -> void {
        if (token.stop_requested()) {
            return;
        }

        auto message = stream::read<Atp, error::SystemError, decltype(server)>(server);
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

    static constexpr auto handleCardInserted(stream::WriteStream<Atp, error::SystemError> auto& server, Cards& cards, CardInserted state)
        -> void {
        std::println("Card inserted");

        auto message = Atp{
            1,
            Type::Atr,
            GenericData{cards[state.readerName]->atr()},
        };

        auto result = stream::write<Atp>(server, message);
        if (!result) {
            std::println("Failed to transmit message {}. Reason: {}", message, result.error());
        } else {
            std::println("Transmitted {}", message);
        }
    }

    static constexpr auto handleCardRemoved(stream::WriteStream<Atp, error::SystemError> auto& server) -> void {
        std::println("Card removed");

        auto message = Atp{
            1,
            Type::UnsetCard,
            GenericData{std::vector<std::byte>{}},
        };

        auto result = stream::write<Atp>(server, message);
        if (!result) {
            std::println("Failed to transmit message {}. Reason: {}", message, result.error());
        } else {
            std::println("Transmitted {}", message);
        }
    }

    static constexpr auto handleExit(std::stop_source& stop_source) -> void {
        std::println("Exit signal received, exiting");
        stop_source.request_stop();
    }

    static constexpr auto handleQueue(stream::WriteStream<Atp, error::SystemError> auto& server, Queue& queue, Cards& cards,
                                      std::stop_source& stopSource) -> void {
        if (queue.empty()) {
            return;
        }

        auto element = queue.front();
        queue.pop();

        std::visit(Overloads{[&server, &cards](CardInserted state) { handleCardInserted(server, cards, state); },
                             [&server](CardRemoved) { handleCardRemoved(server); }, [&stopSource](Exit) { handleExit(stopSource); }},
                   element);
    }
};

#endif
