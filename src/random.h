
#pragma once

#include <random>

inline int randIn(int a, int b) { return a + (std::rand() % (b - a + 1)); }

inline int randSign() {
    int sign = randIn(0, 1);
    if (sign == 0) {
        return -1;
    } else {
        return 1;
    }
}

inline std::mt19937 make_engine(size_t seed) {
    std::mt19937 gen((unsigned int) seed);
    return gen;
}

inline int get_rand(std::mt19937 gen, int range_mn, int range_mx) {
    // Standard mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<> dis(range_mn, range_mx);
    return dis(gen);
}
