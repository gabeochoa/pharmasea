#pragma once

//
#include "log.h"
#define _USE_MATH_DEFINES  // for C++
#include <math.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>  // For using the system function
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#define CONCAT_IMPL(s1, s2) s1##s2
#define CONCAT(s1, s2) CONCAT_IMPL(s1, s2)
#define ANONYMOUS(x) CONCAT(x, __COUNTER__)
// usage: int ANONYMOUS(myvar); => int myvar_1;

namespace util {

#ifdef __APPLE__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif
// Legit what it says
inline void force_segfault() { *(int*) 0 = 0; }

#ifdef __APPLE__
#pragma clang diagnostic pop
#else
#pragma enable_warn
#endif

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

static bool in_range(int a, int b, int val) { return val >= a && val <= b; }

static std::string toLowerCase(const std::string_view& str) {
    std::string result;
    for (char c : str) {
        result +=
            static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return result;
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

inline std::vector<std::string> split_re(const std::string& input,
                                         const std::string& regex) {
    // passing -1 as the submatch index parameter performs splitting
    const static std::regex re(regex);
    std::sregex_token_iterator first{input.begin(), input.end(), re, -1};
    std::sregex_token_iterator last;
    return std::vector<std::string>{first, last};
}

inline std::string snake_case(const std::string_view& input) {
    std::string result;

    for (size_t i = 0; i < input.length(); ++i) {
        char ch = input[i];

        if (std::isupper(ch)) {
            if (i > 0) result += '_';
            ch = static_cast<char>(std::tolower(ch));
        }

        result += ch;
    }

    return result;
}

template<typename T>
inline std::string convertToSnakeCase(T type) {
    return snake_case(magic_enum::enum_name(type));
}

static float clamp(float a, float mn, float mx) {
    return std::min(std::max(a, mn), mx);
}

static float round_up(float value, int decimal_places) {
    const float multiplier = (float) std::pow(10.0, decimal_places);
    return std::ceil(value * multiplier) / multiplier;
}

static float trunc(float value, int decimal_places) {
    const float multiplier = (float) std::pow(10.0, decimal_places);
    return std::trunc(value * multiplier) / multiplier;
}

static float lerp(float a, float b, float pct) {
    return (a * (1 - pct)) + (b * pct);
}

template<template<typename...> class Container, typename K, typename V,
         typename... Ts>
inline V map_get_or_default(Container<K, V, Ts...> map, K key, V def_value) {
    if (map.contains(key)) {
        return map.at(key);
    }
    return def_value;
}

// note: delimiter cannot contain NUL characters
template<typename Range, typename Value = typename Range::value_type>
[[nodiscard]] inline std::string join(Range const& elements,
                                      const char* const delimiter) {
    std::ostringstream os;
    auto b = begin(elements), e = end(elements);
    if (b != e) {
        std::copy(b, prev(e), std::ostream_iterator<Value>(os, delimiter));
        b = prev(e);
    }
    if (b != e) {
        os << *b;
    }
    return os.str();
}

[[nodiscard]] inline int words_in_string(const std::string& str) {
    const int count = (int) std::count_if(str.begin(), str.end(),
                                          [](char i) { return i == ' '; });
    return count;
}

inline void open_url(const std::string& url) {
#ifdef _WIN32
    // For Windows
    std::string command = "start " + url;
#elif __APPLE__
    std::string command = "open " + url;
#else
    // For Unix-based systems (e.g., Linux)
    std::string command = "xdg-open " + url;
#endif

    int result = std::system(command.c_str());
    if (result != 0) {
        log_warn("Failed to open url: {}", result);
    }
}

}  // namespace util
