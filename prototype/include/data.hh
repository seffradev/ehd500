#ifndef DATA_HH
#define DATA_HH

#include <algorithm>
#include <format>
#include <iterator>
#include <ranges>
#include <vector>

auto hexString(std::ranges::forward_range auto& stream) {
    using namespace std::literals;
    return std::ranges::fold_left(stream | std::views::transform([](auto byte) { return std::format("{:#04x} ", static_cast<int>(byte)); }),
                                  ""s, std::plus<std::string>());
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
        internal.reserve(data.size());
        auto tToByte = [](auto t) { return static_cast<std::byte>(t); };
        std::ranges::copy(data | std::views::transform(tToByte), std::back_inserter(internal));
    }

    /// Converts the current `GenericData` into any object convertible from `GenericData`.
    template <ResizableRange T>
    [[nodiscard]]
    operator T() const {
        return materialize<T>(internal | std::views::transform([](std::byte b) { return static_cast<T::value_type>(b); }));
    }

    auto& operator[](size_t index) noexcept { return internal[index]; }

    const auto& operator[](size_t index) const noexcept { return internal[index]; }

    auto& at(size_t index) { return internal.at(index); }

    const auto& at(size_t index) const { return internal.at(index); }

    auto data() const noexcept { return internal.data(); }

    auto size() const noexcept { return internal.size(); }

    auto begin() const noexcept { return internal.begin(); }
    auto cbegin() const noexcept { return internal.cbegin(); }
    auto end() const noexcept { return internal.end(); }
    auto cend() const noexcept { return internal.cend(); }

private:
    std::vector<std::byte> internal;
};

#endif
