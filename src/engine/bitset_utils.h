
#pragma once

#include <bitset>
#include <functional>
#include <random>

#include "random_engine.h"
//
#include "log.h"

namespace bitset_utils {

template<typename T, typename E>
void set(T& bitset, E enum_value) {
    bitset.set(static_cast<int>(enum_value));
}

template<typename T, typename E>
void reset(T& bitset, E enum_value) {
    bitset.reset(static_cast<int>(enum_value));
}

template<typename T, typename E>
bool test(const T& bitset, E enum_value) {
    return bitset.test(static_cast<int>(enum_value));
}

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

// TODO :BE: combine this with the other one...
template<size_t N>
int get_random_enabled_bit(const std::bitset<N>& bitset,
                           std::mt19937& generator, size_t max_value = N) {
    std::vector<int> enabled_indices;
    for (size_t i = 0; i < max_value; ++i) {
        if (bitset.test(i)) {
            enabled_indices.push_back(static_cast<int>(i));
        }
    }

    if (enabled_indices.empty()) {
        // No bits are enabled, return -1 or handle the error as needed.
        return -1;
    }
    int random_index = generator() % (static_cast<int>(enabled_indices.size()));
    return enabled_indices[random_index];
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
    int random_index = RandomEngine::get().get_index(enabled_indices);
    return enabled_indices[random_index];
}

template<size_t N>
int get_first_enabled_bit(const std::bitset<N>& bitset) {
    for (size_t i = 0; i < bitset.size(); ++i) {
        if (bitset.test(i)) {
            return (int) i;
        }
    }
    return -1;
}

// TODO combine with the one in entity helper?
enum struct ForEachFlow {
    NormalFlow = 0,
    Continue = 1,
    Break = 2,
};

template<size_t N>
void for_each_enabled_bit(const std::bitset<N>& bitset,
                          const std::function<ForEachFlow(size_t)>& cb) {
    for (size_t i = 0; i < bitset.size(); ++i) {
        if (bitset.test(i)) {
            ForEachFlow fef = cb(i);
            switch (fef) {
                case ForEachFlow::NormalFlow:
                case ForEachFlow::Continue:
                    break;
                case ForEachFlow::Break:
                    return;
            }
        }
    }
}

};  // namespace bitset_utils
