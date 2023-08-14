
#pragma once

#include <bitset>

#include "random.h"

namespace bitset_utils {
template<typename T>
int index_of_nth_set_bit(const T& bitset, int n) {
    int count = 0;
    for (size_t i = 0; i < bitset.size(); ++i) {
        if (bitset.test(i)) {
            if (++count == n) {
                return static_cast<int>(i);
            }
        }
    }
    return -1;  // Return -1 if the nth set bit is not found
}

template<size_t N>
int get_random_enabled_bit(const std::bitset<N>& bitset) {
    std::vector<int> enabled_indices;
    for (size_t i = 0; i < bitset.size(); ++i) {
        if (bitset.test(i)) {
            enabled_indices.push_back(static_cast<int>(i));
        }
    }

    if (enabled_indices.empty()) {
        // No bits are enabled, return -1 or handle the error as needed.
        return -1;
    }
    int random_index = randIn(0, static_cast<int>(enabled_indices.size()) - 1);
    return enabled_indices[random_index];
}

template<size_t N>
void for_each_enabled_bit(const std::bitset<N>& bitset,
                          std::function<void(size_t)> cb) {
    for (size_t i = 0; i < bitset.size(); ++i) {
        if (bitset.test(i)) {
            cb(i);
        }
    }
}

};  // namespace bitset_utils
