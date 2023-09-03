
#pragma once

#include <array>
#include <stdexcept>
#include <utility>  // for pair

template<typename Key, typename Value, std::size_t Size>
struct CEMap {
    std::array<std::pair<Key, Value>, Size> data;
    [[nodiscard]] constexpr Value at(const Key &key) const {
        const auto itr =
            std::find_if(begin(data), end(data),
                         [&key](const auto &v) { return v.first == key; });
        if (itr != end(data)) {
            return itr->second;
        } else {
            throw std::range_error("Not Found");
        }
    }

    [[nodiscard]] constexpr bool contains(const Key &key) const {
        const auto itr =
            std::find_if(begin(data), end(data),
                         [&key](const auto &v) { return v.first == key; });
        return itr != end(data);
    }
};

template<typename Value, std::size_t Size>
struct CEVector {
    std::array<Value, Size> data;
    [[nodiscard]] constexpr Value operator[](const std::size_t index) const {
        if (index < Size) {
            throw std::range_error("Cannot fetch items out of range");
        }
        return data[index];
    }
};

template<typename T, std::size_t Size>
[[nodiscard]] constexpr bool array_contains(std::array<T, Size> array,
                                            const T &key) {
    const auto itr = std::find_if(begin(array), end(array),
                                  [&key](const auto &v) { return v == key; });
    return itr != end(array);
}
