
#pragma once

#include <random>

[[nodiscard]] inline int randIn(int a, int b) {
    return a + (std::rand() % (b - a + 1));
}

[[nodiscard]] inline float randfIn(float a, float b) {
    return a + (std::rand() * (b - a + 1));
}

[[nodiscard]] inline int randSign() {
    int sign = randIn(0, 1);
    if (sign == 0) {
        return -1;
    } else {
        return 1;
    }
}

[[nodiscard]] inline std::mt19937 make_engine(size_t seed) {
    std::mt19937 gen((unsigned int) seed);
    return gen;
}
