#pragma once 

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

#include "raylib.h"

#ifdef WRITE_FILES
#include "../vendor/sago/platform_folders.h"
#endif

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

typedef Vector2 vec2;
typedef Vector3 vec3;
typedef Vector4 vec4;

#define FMT_HEADER_ONLY
#include "../vendor/fmt/format.h"
#include "../vendor/fmt/ostream.h"
// this is needed for wstring printing
#include "../vendor/fmt/xchar.h"


#include <atomic>
#include <optional>
#include <vector>
#include <memory>
#include <stdio.h>
#include <iostream>
#include <functional>
#include <map>
#include <set>
#include <limits>
#include <numeric>
#include <ostream>
#include <deque>
#include <sstream>
#include <filesystem>
#include <fstream>
#include <stack>
#include <unordered_map>
#include <array>
#include <cassert>
#include <algorithm>
#include <variant>

// For bitwise operations
template< typename T >
typename std::enable_if< std::is_enum< T >::value, T >::type
operator~ (T a) { return static_cast<T>(~static_cast<int>(a)); }

template< typename T >
typename std::enable_if< std::is_enum< T >::value, T >::type
operator| (T a, T b) { return static_cast<T>((static_cast<int>(a) | static_cast<int>(b))); }

template< typename T >
typename std::enable_if< std::is_enum< T >::value, T >::type
operator& (T a, T b) { return static_cast<T>((static_cast<int> (a) & static_cast<int>(b))); }

template< typename T >
typename std::enable_if< std::is_enum< T >::value, T >::type
operator^ (T a, T b) { return static_cast<T>((static_cast<int> (a) ^ static_cast<int>(b))); }

template< typename T >
typename std::enable_if< std::is_enum< T >::value, T >::type
operator|= (T& a, T b) { return reinterpret_cast<T&>((reinterpret_cast<int&>(a) |= static_cast<int>(b))); }

template< typename T >
typename std::enable_if< std::is_enum< T >::value, T >::type
operator&= (T& a, T b) { return reinterpret_cast<T&>((reinterpret_cast<int&>(a) &= static_cast<int>(b))); }

template< typename T >
typename std::enable_if< std::is_enum< T >::value, T >::type
operator^= (T& a, T b) { return reinterpret_cast<T&>((reinterpret_cast<int&>(a) ^= static_cast<int>(b))); }
