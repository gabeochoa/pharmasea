#pragma once

#include <type_traits>

namespace ps::concepts {

template<typename T>
concept Enum = std::is_enum_v<T>;

// We only support bitwise operators on enums whose underlying type is integral.
// (This matches how the code currently treats enums: cast to an integer type,
// do the bit op, cast back.)
template<typename T>
concept BitwiseEnum = Enum<T> && std::is_integral_v<std::underlying_type_t<T>>;

template<BitwiseEnum E>
[[nodiscard]] constexpr std::underlying_type_t<E> to_underlying(E e) noexcept {
    return static_cast<std::underlying_type_t<E>>(e);
}

}  // namespace ps::concepts
