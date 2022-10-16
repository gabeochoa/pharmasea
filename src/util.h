#pragma once

#include "external_include.h"
//
#include <cmath>

namespace util {

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
// explicit deduction guide (not needed as of C++20)
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

template<typename T>
int sgn(T val) {
    return (T(0) < val) - (val < T(0));

}

float deg2rad(float deg) {
    return deg * M_PI / 180.0;
}

float rad2deg(float rad) {
    return rad * (180.f / M_PI);
}

std::vector<std::string> split_string(const std::string& str,
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
