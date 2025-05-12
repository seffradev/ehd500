#include <gtest/gtest.h>

#include <exception.hh>
#include <expected>
#include <print>
#include <simbank.hh>
#include <stop_token>

// template <typename T>
// concept Writeable = std::convertible_to<T, GenericData>;
//
// template <typename Stream, typename Data, typename Error>
// concept ReadStream = requires(Stream stream, size_t size) {
//     { stream.template read<Data>() } -> std::convertible_to<std::expected<Data, Error>>;
//     { stream.template read<Data>(size) } -> std::convertible_to<std::expected<Data, Error>>;
// } && error::Error<Error>;
//
// template <typename Stream, typename Data, typename Error>
// concept WriteStream = requires(Stream stream, const Data data) {
//     { stream.template write<Data>(data) } -> std::same_as<std::expected<size_t, Error>>;
// } && Writeable<Data> && error::Error<Error>;
//
// template <typename Stream, typename Data, typename Error>
// concept RwStream = ReadStream<Stream, Data, Error> && WriteStream<Stream, Data, Error>;

class MockStream {
    constexpr auto write(const Atp& atp) -> std::expected<size_t, error::SystemError> {
        std::println("Written data: {}", atp);
        return 0;
    }

    template <typename Data = Atp>
    constexpr auto read() -> std::expected<Data, error::SystemError> {
        return Atp{1, Type::Atr, std::vector<std::byte>{}};
    }

    template <typename Data = Atp>
    constexpr auto read(size_t size) -> std::expected<Data, error::SystemError> {
        return Atp{1, Type::Atr, std::vector<std::byte>{}};
    }
};

static_assert(stream::RwStream<MockStream, Atp, error::SystemError>);

TEST(NoTcp, WithoutTcp) {
    auto mock       = MockStream{};
    auto stopSource = std::stop_source{};

    auto simbank = SimBank{};
    simbank.run(mock, stopSource);
}
