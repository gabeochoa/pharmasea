#pragma once

//
#define _USE_MATH_DEFINES  // for C++
#include <math.h>

#include <cmath>
#include <string>
#include <vector>

#define CONCAT_IMPL(s1, s2) s1##s2
#define CONCAT(s1, s2) CONCAT_IMPL(s1, s2)
#define ANONYMOUS(x) CONCAT(x, __COUNTER__)
// usage: int ANONYMOUS(myvar); => int myvar_1;

namespace util {

template<class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
// explicit deduction guide (not needed as of C++20)
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

template<typename T>
int sgn(T val) {
    return (T(0) < val) - (val < T(0));
}

static float deg2rad(float deg) {
    return deg * static_cast<float>(M_PI) / 180.0f;
}

static float rad2deg(float rad) {
    return rad * (180.f / static_cast<float>(M_PI));
}

static std::vector<std::string> split_string(const std::string& str,
                                             const std::string& delimiter) {
    std::vector<std::string> strings;

    std::string::size_type pos = 0;
    std::string::size_type prev = 0;
    while ((pos = str.find(delimiter, prev)) != std::string::npos) {
        strings.push_back(str.substr(prev, pos - prev));
        prev = pos + delimiter.size();
    }

    // To get the last substring (or only, if delimiter is not found)
    strings.push_back(str.substr(prev));

    return strings;
}

}  // namespace util
