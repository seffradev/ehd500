#ifndef ATP_HH
#define ATP_HH

#include <sys/socket.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <format>
#include <iterator>
#include <print>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

#include "data.hh"
#include "exception.hh"
#include "tcp.hh"

EXCEPTION(AtpException);

enum class Type {
    Apdu,
    Atr,
    UnsetCard,
};

constexpr std::string_view toString(const Type& type) noexcept {
    switch (type) {
        using enum Type;
        case Apdu:
            return "APDU";
        case Atr:
            return "ATR";
        case UnsetCard:
            return "UNSET_CARD";
    }
    return "UNREACHABLE";
}

struct Atp {
    constexpr Atp(auto version, Type type, std::ranges::contiguous_range auto&& data)
        : version(version), type(type == Type::Apdu ? 0 : 1), length(data.size()), data(std::move(data)) {}

    explicit constexpr Atp(const GenericData& data) {
        if (data.size() < minimumSize) {
            throw AtpException{"insufficient data available in packet"};
        }

        auto header = (static_cast<uint32_t>(data[0]) << 24) + (static_cast<uint32_t>(data[1]) << 16) +
                      (static_cast<uint32_t>(data[2]) << 8) + static_cast<uint32_t>(data[3]);

        version = (static_cast<uint32_t>(header) & versionBytes) >> versionOffset;
        type    = (static_cast<uint32_t>(header) & typeBytes) >> typeOffset;
        length  = (static_cast<uint32_t>(header) & lengthBytes);

        assert(header == static_cast<uint32_t>((version << versionOffset) + (type << typeOffset) + length));

        if (data.size() < length) {
            return;
        }

        this->data.reserve(length);
        std::copy(std::cbegin(data) + minimumSize, std::cbegin(data) + minimumSize + length,
                  std::back_inserter(this->data));
    }

    constexpr operator GenericData() {
        auto out = std::vector<std::byte>{};
        out.reserve(length + minimumSize);

        auto header = (static_cast<uint32_t>(version) << versionOffset) + (static_cast<uint32_t>(type) << typeOffset) +
                      static_cast<uint32_t>(length);

        out.emplace_back(static_cast<std::byte>(header >> 24));
        out.emplace_back(static_cast<std::byte>(header >> 16));
        out.emplace_back(static_cast<std::byte>(header >> 8));
        out.emplace_back(static_cast<std::byte>(header));

        std::copy(std::cbegin(data), std::cend(data), std::back_inserter(out));

        return GenericData{out};
    }

    constexpr Type getType() const noexcept {
        if (type) {
            return Type::Atr;
        }
        return Type::Apdu;
    }

    uint32_t               version  : 4;
    uint32_t               type     : 2;
    uint32_t               reserved : 9;
    uint32_t               length   : 17;
    std::vector<std::byte> data;

    static constexpr auto minimumSize = sizeof(uint32_t);

private:
    static constexpr auto versionOffset = 28;
    static constexpr auto typeOffset    = 26;
    static constexpr auto versionBytes  = 0b1111'0000'0000'0000'0000'0000'0000'0000;
    static constexpr auto typeBytes     = 0b0000'1100'0000'0000'0000'0000'0000'0000;
    static constexpr auto lengthBytes   = 0b0000'0000'0000'0001'1111'1111'1111'1111;
    static constexpr auto reservedBytes = ~(versionBytes | typeBytes | lengthBytes);
};

template <>
constexpr auto TcpSocket::read<Atp>() -> Atp {
    auto buffer = std::array<std::byte, Atp::minimumSize>{};
    buffer.fill(std::byte{0});

    if (recv(peer, buffer.data(), buffer.size(), 0) < 0) {
        throw SocketException{"failed to read header"};
    }

    auto shortAtp       = Atp{buffer};
    auto variableBuffer = std::vector<std::byte>(shortAtp.length, std::byte{0});

    if (recv(peer, variableBuffer.data(), variableBuffer.size(), 0) < 0) {
        throw SocketException{"failed to read data"};
    }

    return Atp{
        shortAtp.version,
        shortAtp.getType(),
        std::move(variableBuffer),
    };
}

template <>
struct std::formatter<Atp> {
    constexpr auto parse(std::format_parse_context& context) { return context.begin(); }

    auto format(const Atp& atp, std::format_context& context) const {
        return std::format_to(context.out(), "Version: {}, Type: {}, Length: {}, Data: {}", atp.version,
                              toString(atp.getType()), atp.length, hexString(atp.data));
    }
};

#endif
