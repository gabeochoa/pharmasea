
#pragma once

#include <random>

[[nodiscard]] inline int randIn(int a, int b) {
    return a + (std::rand() % (b - a + 1));
}

[[nodiscard]] inline float randfIn(float a, float b) {
    return a + (std::rand() * (b - a + 1));
}

[[nodiscard]] inline int randSign() { return randIn(0, 1) == 0 ? -1 : 1; }

[[nodiscard]] inline std::string randString(int length) {
    // TODO research better ways to do this
    std::string out;

    for (int i = 0; i < length; i++) {
        out += (char) (randIn('a', 'z'));
    }

    return out;
}

[[nodiscard]] inline std::mt19937 make_engine(size_t seed) {
    std::mt19937 gen((unsigned int) seed);
    return gen;
}
