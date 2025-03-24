#ifndef ATP_HH
#define ATP_HH

#include <cstdint>
#include <format>
#include <iterator>
#include <print>
#include <string>
#include <string_view>
#include <vector>

#include "data.hh"
#include "exception.hh"

EXCEPTION(AtpException);

enum class Type {
    Apdu,
    Atr,
};

constexpr std::string_view to_string(const Type& type) noexcept {
    switch (type) {
        using enum Type;
        case Apdu:
            return "Apdu";
        case Atr:
            return "Atr";
    }
    return "unreachable";
}

struct Atp {
    constexpr Atp(auto version, auto type, auto length, auto data)
        : version(version), type(type), length(length), data(data) {}

    constexpr Atp(Data&& data) {
        if (data.raw.size() < minimumSize) {
            throw AtpException{"insufficient data available in packet"};
        }

        auto header = (static_cast<uint32_t>(data.raw[0]) << 24) + (static_cast<uint32_t>(data.raw[1]) << 16) +
                      (static_cast<uint32_t>(data.raw[2]) << 8) + static_cast<uint32_t>(data.raw[3]);

        version = (static_cast<uint32_t>(header) & versionBytes) >> versionOffset;
        type    = (static_cast<uint32_t>(header) & typeBytes) >> typeOffset;
        length  = (static_cast<uint32_t>(header) & lengthBytes);

        std::copy(std::cbegin(data.raw) + sizeof(uint32_t), std::cbegin(data.raw) + sizeof(uint32_t) + length,
                  std::back_inserter(this->data));
    }

    constexpr operator Data() {
        auto out = std::vector<std::byte>{};
        out.reserve(length + sizeof(uint32_t));

        auto header = (static_cast<uint32_t>(version) << versionOffset) + (static_cast<uint32_t>(type) << typeOffset) +
                      static_cast<uint32_t>(length);

        std::println("{}", header);

        out.emplace_back(static_cast<std::byte>(header >> 24));
        out.emplace_back(static_cast<std::byte>(header >> 16));
        out.emplace_back(static_cast<std::byte>(header >> 8));
        out.emplace_back(static_cast<std::byte>(header));

        std::copy(std::cbegin(data), std::cend(data), std::back_inserter(out));

        return Data{out};
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

private:
    static constexpr auto minimumSize   = 4;
    static constexpr auto versionOffset = 28;
    static constexpr auto typeOffset    = 27;
    static constexpr auto versionBytes  = 0b1111'0000'0000'0000'0000'0000'0000'0000;
    static constexpr auto typeBytes     = 0b0000'1000'0000'0000'0000'0000'0000'0000;
    static constexpr auto lengthBytes   = 0b0000'0000'0000'0001'1111'1111'1111'1111;
    static constexpr auto reservedBytes = ~(versionBytes | typeBytes | lengthBytes);
};

template <>
struct std::formatter<Atp> {
    constexpr auto parse(std::format_parse_context& context) { return context.begin(); }

    auto format(const Atp& atp, std::format_context& context) const {
        return std::format_to(context.out(), "Version: {}, Type: {}, Length: {}, Data: {}", atp.version,
                              to_string(atp.getType()), atp.length, static_cast<std::string>(Data{atp.data}));
    }
};

#endif
