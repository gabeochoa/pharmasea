#pragma once

// Cursor/Linux build shim:
// Provide minimal arithmetic operators for raylib C structs.
//
// On the primary macOS toolchain this functionality is expected to already exist
// via the local raylib setup; in this container it does not, so we define it
// here without touching production headers.

#include "engine/graphics.h"

namespace raylib {

inline bool operator==(const Vector2& a, const Vector2& b) {
    return a.x == b.x && a.y == b.y;
}
inline bool operator!=(const Vector2& a, const Vector2& b) { return !(a == b); }

inline Vector2 operator+(const Vector2& a, const Vector2& b) {
    return Vector2{a.x + b.x, a.y + b.y};
}
inline Vector2 operator-(const Vector2& a, const Vector2& b) {
    return Vector2{a.x - b.x, a.y - b.y};
}
inline Vector2 operator*(const Vector2& a, float s) { return Vector2{a.x * s, a.y * s}; }
inline Vector2 operator/(const Vector2& a, float s) { return Vector2{a.x / s, a.y / s}; }

inline bool operator==(const Vector3& a, const Vector3& b) {
    return a.x == b.x && a.y == b.y && a.z == b.z;
}
inline bool operator!=(const Vector3& a, const Vector3& b) { return !(a == b); }

inline Vector3 operator+(const Vector3& a, const Vector3& b) {
    return Vector3{a.x + b.x, a.y + b.y, a.z + b.z};
}
inline Vector3 operator-(const Vector3& a, const Vector3& b) {
    return Vector3{a.x - b.x, a.y - b.y, a.z - b.z};
}
inline Vector3 operator*(const Vector3& a, float s) {
    return Vector3{a.x * s, a.y * s, a.z * s};
}
inline Vector3 operator/(const Vector3& a, float s) {
    return Vector3{a.x / s, a.y / s, a.z / s};
}

}  // namespace raylib

