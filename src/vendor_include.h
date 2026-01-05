#pragma once

#include "engine/graphics.h"
#include "engine/concepts.h"

namespace reasings {
// TODO decide if we wnat to use static inline
#include "reasings.h"
}  // namespace reasings

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
#include <fmt/args.h>
#include <fmt/format.h>
#include <fmt/ostream.h>
// this is needed for wstring printing
#include <fmt/xchar.h>
// Expected is now provided by afterhours
// #include <expected.hpp>
//

// We redefine the max here because the max keyboardkey is in the 300s
#undef MAGIC_ENUM_RANGE_MAX
#define MAGIC_ENUM_RANGE_MAX 400
#include <magic_enum/magic_enum.hpp>
// TODO :INFRA: cant use format yet due to no std::format yet (though not even
// sure what its needed for) #include <magic_enum/magic_enum_format.hpp>

#include <magic_enum/magic_enum_fuse.hpp>
#include <nlohmann/json.hpp>

#include "zpp_bits_include.h"

constexpr auto serialize(auto& archive, vec2& data) {
    return archive(  //
        data.x,      //
        data.y       //
    );
}

constexpr auto serialize(auto& archive, vec3& data) {
    return archive(  //
        data.x,      //
        data.y,      //
        data.z       //
    );
}

constexpr auto serialize(auto& archive, Color& data) {
    return archive(  //
        data.r,      //
        data.g,      //
        data.b,      //
        data.a       //
    );
}

constexpr auto serialize(auto& archive, Rectangle& data) {
    return archive(  //
        data.x,      //
        data.y,      //
        data.width,  //
        data.height  //
    );
}

constexpr auto serialize(auto& archive, BoundingBox& data) {
    return archive(  //
        data.min,    //
        data.max     //
    );
}

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
template<ps::concepts::BitwiseEnum T>
[[nodiscard]] constexpr T operator~(T a) noexcept {
    using ps::concepts::to_underlying;
    return static_cast<T>(~to_underlying(a));
}

template<ps::concepts::BitwiseEnum T>
[[nodiscard]] constexpr T operator|(T a, T b) noexcept {
    using ps::concepts::to_underlying;
    return static_cast<T>(to_underlying(a) | to_underlying(b));
}

template<ps::concepts::BitwiseEnum T>
[[nodiscard]] constexpr T operator&(T a, T b) noexcept {
    using ps::concepts::to_underlying;
    return static_cast<T>(to_underlying(a) & to_underlying(b));
}

template<ps::concepts::BitwiseEnum T>
[[nodiscard]] constexpr T operator^(T a, T b) noexcept {
    using ps::concepts::to_underlying;
    return static_cast<T>(to_underlying(a) ^ to_underlying(b));
}

template<ps::concepts::BitwiseEnum T>
constexpr T& operator|=(T& a, T b) noexcept {
    a = (a | b);
    return a;
}

template<ps::concepts::BitwiseEnum T>
constexpr T& operator&=(T& a, T b) noexcept {
    a = (a & b);
    return a;
}

template<ps::concepts::BitwiseEnum T>
constexpr T& operator^=(T& a, T b) noexcept {
    a = (a ^ b);
    return a;
}
