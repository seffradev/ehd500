#ifndef DATA_HH
#define DATA_HH

#include <algorithm>
#include <ranges>
#include <vector>

class Data {
public:
    constexpr Data(std::ranges::range auto &&data) {
        raw.reserve(data.size());
        auto tToByte = [](auto t) { return static_cast<std::byte>(t); };
        std::ranges::copy(data | std::views::transform(tToByte), std::back_inserter(raw));
    }

    template <typename T>
    operator T() const {
        auto message = T{};
        message.reserve(raw.size());
        auto byteToChar = [](std::byte b) { return static_cast<T::value_type>(b); };
        std::ranges::copy(raw | std::views::transform(byteToChar), std::back_inserter(message));
        return message;
    }

    std::vector<std::byte> raw;
};

#endif
