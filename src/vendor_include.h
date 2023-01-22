#pragma once

#include "engine/graphics.h"

#ifdef __APPLE__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#pragma clang diagnostic ignored "-Wdeprecated-volatile"
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#endif

#ifdef WIN32
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif

#define FMT_HEADER_ONLY
#include <fmt/format.h>
#include <fmt/ostream.h>
// this is needed for wstring printing
#include <fmt/xchar.h>
//
#include <expected.hpp>
//
#include <magic_enum/magic_enum.hpp>
// TODO cant use format yet due to no std::format yet (though not even sure what
// its needed for) #include <magic_enum/magic_enum_format.hpp>
#include <magic_enum/magic_enum_fuse.hpp>

//
#include <bitsery/adapter/buffer.h>
#include <bitsery/bitsery.h>
#include <bitsery/ext/inheritance.h>
#include <bitsery/ext/pointer.h>
#include <bitsery/ext/std_bitset.h>
#include <bitsery/ext/std_map.h>
#include <bitsery/ext/std_optional.h>
#include <bitsery/ext/std_smart_ptr.h>
#include <bitsery/ext/std_tuple.h>
#include <bitsery/ext/std_variant.h>
#include <bitsery/traits/array.h>
#include <bitsery/traits/deque.h>
#include <bitsery/traits/string.h>
#include <bitsery/traits/vector.h>

namespace bitsery {
template<typename S>
void serialize(S& s, vec2& data) {
    s.value4b(data.x);
    s.value4b(data.y);
}

template<typename S>
void serialize(S& s, vec3& data) {
    s.value4b(data.x);
    s.value4b(data.y);
    s.value4b(data.z);
}

template<typename S>
void serialize(S& s, Color& data) {
    s.value1b(data.r);
    s.value1b(data.g);
    s.value1b(data.b);
    s.value1b(data.a);
}
}  // namespace bitsery

#include "engine/tracy.h"

#ifdef __APPLE__
#pragma clang diagnostic pop
#else
#pragma enable_warn
#endif

#ifdef WIN32
#pragma GCC diagnostic pop
#endif

#ifdef _MSC_VER
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

// For bitwise operations
template<typename T>
typename std::enable_if<std::is_enum<T>::value, T>::type operator~(T a) {
    return static_cast<T>(~static_cast<int>(a));
}

template<typename T>
typename std::enable_if<std::is_enum<T>::value, T>::type operator|(T a, T b) {
    return static_cast<T>((static_cast<int>(a) | static_cast<int>(b)));
}

template<typename T>
typename std::enable_if<std::is_enum<T>::value, T>::type operator&(T a, T b) {
    return static_cast<T>((static_cast<int>(a) & static_cast<int>(b)));
}

template<typename T>
typename std::enable_if<std::is_enum<T>::value, T>::type operator^(T a, T b) {
    return static_cast<T>((static_cast<int>(a) ^ static_cast<int>(b)));
}

template<typename T>
typename std::enable_if<std::is_enum<T>::value, T>::type operator|=(T& a, T b) {
    return reinterpret_cast<T&>(
        (reinterpret_cast<int&>(a) |= static_cast<int>(b)));
}

template<typename T>
typename std::enable_if<std::is_enum<T>::value, T>::type operator&=(T& a, T b) {
    return reinterpret_cast<T&>(
        (reinterpret_cast<int&>(a) &= static_cast<int>(b)));
}

template<typename T>
typename std::enable_if<std::is_enum<T>::value, T>::type operator^=(T& a, T b) {
    return reinterpret_cast<T&>(
        (reinterpret_cast<int&>(a) ^= static_cast<int>(b)));
}
