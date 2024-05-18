
#pragma once

#include <array>
#include <functional>
#include <stdexcept>
#include <utility>  // for pair

template<typename Key, typename Value, std::size_t Size>
struct CEMap {
    std::array<std::pair<Key, Value>, Size> data;
    [[nodiscard]] constexpr Value at(const Key &key) const {
        const auto itr =
            std::find_if(std::begin(data), std::end(data),
                         [&key](const auto &v) { return v.first == key; });
        if (itr != std::end(data)) {
            return itr->second;
        } else {
            throw std::range_error("Not Found");
        }
    }

    [[nodiscard]] constexpr bool contains(const Key &key) const {
        const auto itr =
            std::find_if(std::begin(data), std::end(data),
                         [&key](const auto &v) { return v.first == key; });
        return itr != std::end(data);
    }

    [[nodiscard]] constexpr auto begin() const { return data.begin(); }
    [[nodiscard]] constexpr auto end() const { return data.end(); }
    [[nodiscard]] constexpr auto size() const { return data.size(); }

    [[nodiscard]] constexpr std::pair<Key, Value> for_index(size_t i) const {
        if (i > Size) {
            throw std::range_error("index larger than size of map");
        }
        return data[i];
    }

    [[nodiscard]] constexpr Value value_for_index(size_t i) const {
        return for_index(i).second;
    }

    [[nodiscard]] constexpr Key key_for_index(size_t i) const {
        return for_index(i).first;
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
    const auto itr = std::find_if(std::begin(array), std::end(array),
                                  [&key](const auto &v) { return v == key; });
    return itr != std::end(array);
}

template<typename T, std::size_t Size>
[[nodiscard]] constexpr int index_of(const std::array<T, Size> &array,
                                     const T &key) {
    const auto itr = std::find(std::begin(array), std::end(array), key);
    if (itr != std::end(array)) {
        // Calculate the index by subtracting the beginning iterator
        // from the found iterator
        return static_cast<int>(std::distance(std::begin(array), itr));
    } else {
        // Item not found
        return -1;
    }
}

template<typename T, std::size_t Size>
[[nodiscard]] constexpr int first_matching(
    const std::array<T, Size> &array,
    const std::function<bool(const T &)> &pred) {
    const auto itr = std::find_if(std::begin(array), std::end(array), pred);
    if (itr != std::end(array)) {
        // Calculate the index by subtracting the beginning iterator
        // from the found iterator
        return static_cast<int>(std::distance(std::begin(array), itr));
    } else {
        // Item not found
        return -1;
    }
}

template<typename T, std::size_t Size>
[[nodiscard]] constexpr bool array_contains_any_value(
    const std::array<T, Size> &array) {
    const auto itr = std::find_if(std::begin(array), std::end(array),
                                  [](const auto &v) { return v > T(); });
    return itr != std::end(array);
}

template<typename T, std::size_t Size>
constexpr void array_reset(std::array<T, Size> &array) {
    std::transform(array.begin(), array.end(), array.begin(),
                   [](const auto &) { return T(); });
}
