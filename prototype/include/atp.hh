#ifndef ATP_HH
#define ATP_HH

#include <algorithm>
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
};

constexpr std::string_view to_string(const Type& type) noexcept {
    switch (type) {
        using enum Type;
        case Apdu:
            return "APDU";
        case Atr:
            return "ATR";
    }
    return "UNREACHABLE";
}

struct Atp {
    constexpr Atp(auto version, Type type, auto&& data)
        : version(version), type(type == Type::Apdu ? 0 : 1), length(data.size()), data(std::move(data)) {}

    constexpr Atp(GenericData&& data) {
        if (data.size() < minimumSize) {
            throw AtpException{"insufficient data available in packet"};
        }

        auto header = (static_cast<uint32_t>(data[0]) << 24) + (static_cast<uint32_t>(data[1]) << 16) +
                      (static_cast<uint32_t>(data[2]) << 8) + static_cast<uint32_t>(data[3]);

        version = (static_cast<uint32_t>(header) & versionBytes) >> versionOffset;
        type    = (static_cast<uint32_t>(header) & typeBytes) >> typeOffset;
        length  = (static_cast<uint32_t>(header) & lengthBytes);

        std::copy(std::cbegin(data) + sizeof(uint32_t), std::cbegin(data) + sizeof(uint32_t) + length,
                  std::back_inserter(this->data));
    }

    constexpr operator GenericData() {
        auto out = std::vector<std::byte>{};
        out.reserve(length + sizeof(uint32_t));

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
    uint32_t               type     : 1;
    uint32_t               reserved : 10;
    uint32_t               length   : 17;
    std::vector<std::byte> data;

    static constexpr auto minimumSize = 4;

private:
    static constexpr auto versionOffset = 28;
    static constexpr auto typeOffset    = 27;
    static constexpr auto versionBytes  = 0b1111'0000'0000'0000'0000'0000'0000'0000;
    static constexpr auto typeBytes     = 0b0000'1000'0000'0000'0000'0000'0000'0000;
    static constexpr auto lengthBytes   = 0b0000'0000'0000'0001'1111'1111'1111'1111;
    static constexpr auto reservedBytes = ~(versionBytes | typeBytes | lengthBytes);
};

template <>
constexpr auto TcpSocket::read<Atp>() -> Atp {
    auto buffer = std::array<std::byte, Atp::minimumSize>{};
    {  // Placeholder variables are a C++2c extension
        auto _ = recv(peer, buffer.data(), buffer.size(), 0);
    }

    auto short_atp       = Atp{buffer};
    auto variable_buffer = std::vector<std::byte>(short_atp.length, std::byte{0});

    {  // Placeholder variables are a C++2c extension
        auto _ = recv(peer, variable_buffer.data(), variable_buffer.capacity(), 0);
    }

    return Atp{
        short_atp.version,
        short_atp.getType(),
        variable_buffer,
    };
}

template <>
struct std::formatter<Atp> {
    constexpr auto parse(std::format_parse_context& context) { return context.begin(); }

    auto format(const Atp& atp, std::format_context& context) const {
        auto hexStream = std::ranges::fold_left(
            atp.data | std::views::transform([](auto byte) { return std::format("{:#04x} ", static_cast<int>(byte)); }),
            ""s, std::plus<std::string>());

        return std::format_to(context.out(), "Version: {}, Type: {}, Length: {}, Data: {}", atp.version,
                              to_string(atp.getType()), atp.length, hexStream);
    }
};

#endif
