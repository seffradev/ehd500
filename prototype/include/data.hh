#ifndef DATA_HH
#define DATA_HH

#include <algorithm>
#include <format>
#include <iterator>
#include <ranges>
#include <vector>

auto hexString(std::ranges::forward_range auto& stream) {
    using namespace std::literals;
    return std::ranges::fold_left(
        stream | std::views::transform([](auto byte) { return std::format("{:#04x} ", static_cast<int>(byte)); }), ""s,
        std::plus<std::string>());
}

template <typename T>
concept ResizableRange = std::ranges::sized_range<T> && requires(T t) { t.reserve(size_t{1}); };

template <ResizableRange T>
T materialize(std::ranges::range auto&& range) {
    T materialized{};

    if constexpr (std::ranges::sized_range<decltype(range)>) {
        materialized.reserve(std::ranges::size(range));
    }

    std::ranges::copy(range, std::back_inserter(materialized));
    return materialized;
}

///
/// A data wrapper enabling simplified casting and conversion to- and from a `std::vector<std::byte>`.
///
/// An example of a conversion from a `std::string` to `GenericData`, to `std::vector<int>`:
///
/// ```
/// auto d = GenericData{"hello"s};
/// auto s = static_cast<std::vector<int>>(d);
/// ```
///
class GenericData {
public:
    /// Takes any range of elements convertible to `std::byte`.
    [[nodiscard]]
    constexpr GenericData(std::ranges::sized_range auto&& data) {
        raw.reserve(data.size());
        auto tToByte = [](auto t) { return static_cast<std::byte>(t); };
        std::ranges::copy(data | std::views::transform(tToByte), std::back_inserter(raw));
    }

    /// Converts the current `GenericData` into any object convertible from `GenericData`.
    template <ResizableRange T>
    [[nodiscard]]
    operator T() const {
        return materialize<T>(raw | std::views::transform([](std::byte b) { return static_cast<T::value_type>(b); }));
    }

    auto& operator[](size_t index) noexcept { return raw[index]; }

    const auto& operator[](size_t index) const noexcept { return raw[index]; }

    auto& at(size_t index) { return raw.at(index); }

    const auto& at(size_t index) const { return raw.at(index); }

    auto data() const noexcept { return raw.data(); }

    auto size() const noexcept { return raw.size(); }

    auto begin() const noexcept { return raw.begin(); }

    auto end() const noexcept { return raw.end(); }

private:
    std::vector<std::byte> raw;
};

#endif
