#pragma once

#include "external_include.h"
//
#define _USE_MATH_DEFINES  // for C++
#include <math.h>

#include <cmath>
#include <codecvt>
#include <locale>

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

float deg2rad(float deg) { return deg * static_cast<float>(M_PI) / 180.0f; }

float rad2deg(float rad) { return rad * (180.f / static_cast<float>(M_PI)); }

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

static constexpr int x[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
static constexpr int y[8] = {-1, 0, 1, -1, 1, -1, 0, 1};

void forEachNeighbor(int i, int j, std::function<void(const vec2&)> cb,
                     int step = 1) {
    for (int a = 0; a < 8; a++) {
        cb(vec2{(float) i + (x[a] * step), (float) j + (y[a] * step)});
    }
}

std::vector<vec2> get_neighbors(int i, int j, int step = 1) {
    std::vector<vec2> ns;
    forEachNeighbor(
        i, j, [&](const vec2& v) { ns.push_back(v); }, step);
    return ns;
}

using convert_t = std::codecvt_utf8<wchar_t>;
static std::wstring_convert<convert_t, wchar_t> strconverter;

inline std::string to_string(std::wstring wstr) {
    try {
        return strconverter.to_bytes(wstr);
    } catch (...) {
        return std::string();
    }
}

inline std::wstring to_wstring(std::string str) {
    try {
        return strconverter.from_bytes(str);
    } catch (...) {
        return std::wstring();
    }
}

inline std::string escaped_wstring(const std::wstring& content) {
    std::string sstr;
    // Reserve memory in 1 hit to avoid lots of copying for long strings.
    static size_t const nchars_per_code = 6;
    sstr.reserve(content.size() * nchars_per_code);
    char code[nchars_per_code];
    code[0] = '\\';
    code[1] = 'u';
    static char const* const hexlut = "0123456789abcdef";
    std::wstring::const_iterator i = content.begin();
    std::wstring::const_iterator e = content.end();
    for (; i != e; ++i) {
        unsigned wc = *i;
        code[2] = (hexlut[(wc >> 12) & 0xF]);
        code[3] = (hexlut[(wc >> 8) & 0xF]);
        code[4] = (hexlut[(wc >> 4) & 0xF]);
        code[5] = (hexlut[(wc) &0xF]);
        sstr.append(code, code + nchars_per_code);
    }
    return sstr;
}

}  // namespace util
