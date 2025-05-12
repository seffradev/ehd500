#ifndef STREAM_HH
#define STREAM_HH

#include <concepts>
#include <expected>

#include "data.hh"
#include "exception.hh"

namespace stream {

template <typename T>
concept Writeable = std::convertible_to<T, GenericData>;

template <typename Stream, typename Data, typename Error>
concept ReadStream = requires(Stream stream, size_t size) {
    { stream.template read<Data>() } -> std::convertible_to<std::expected<Data, Error>>;
    { stream.template read<Data>(size) } -> std::convertible_to<std::expected<Data, Error>>;
} && error::Error<Error>;

template <typename Stream, typename Data, typename Error>
concept WriteStream = requires(Stream stream, const Data& data) {
    { stream.template write<Data>(data) } -> std::same_as<std::expected<size_t, Error>>;
} && Writeable<Data> && error::Error<Error>;

template <typename Stream, typename Data, typename Error>
concept RwStream = ReadStream<Stream, Data, Error> && WriteStream<Stream, Data, Error>;

template <typename Data, error::Error Error, ReadStream<Data, Error> Stream>
class read {
    static auto operator()(Stream& s) -> std::expected<Data, Error> { return s.read(); }
};

template <typename Data, error::Error Error, WriteStream<Data, Error> Stream>
class write {
    static auto operator()(Stream& s, const Data& data) -> std::expected<size_t, Error> { return s.write(data); }
};

}

#endif
