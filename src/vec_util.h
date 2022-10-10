#pragma once

#include "external_include.h"
#include "globals.h"

#ifndef EPSILON
#define EPSILON 0.000001f
#endif

inline bool operator<(const vec2& l, const vec2& r) {
    return (l.x < r.x) || ((l.x == r.x) && (l.y < r.y));
}

bool operator==(const vec2& p, const vec2& q) {
    return ((fabsf(p.x - q.x)) <=
            (EPSILON * fmaxf(1.0f, fmaxf(fabsf(p.x), fabsf(q.x))))) &&
           ((fabsf(p.y - q.y)) <=
            (EPSILON * fmaxf(1.0f, fmaxf(fabsf(p.y), fabsf(q.y)))));
}

std::ostream& operator<<(std::ostream& os, const vec2& v) {
    os << "vec2(" << v.x << ", " << v.y << ")";
    return os;
}

std::ostream& operator<<(std::ostream& os, const vec3& v) {
    os << "vec3(" << v.x << ", " << v.y << ", " << v.z << ")";
    return os;
}

std::ostream& operator<<(std::ostream& os, const Rectangle& v) {
    os << "Rect(" << v.x << ", " << v.y << ", " << v.width << ", " << v.height
       << ")";
    return os;
}

vec2 operator-(vec2 lhs, const vec2& rhs) {
    lhs.x -= rhs.x;
    lhs.y -= rhs.y;
    return lhs;
}

vec3 operator-(vec3 lhs, const vec3& rhs) {
    lhs.x -= rhs.x;
    lhs.y -= rhs.y;
    lhs.z -= rhs.z;
    return lhs;
}

vec3 operator/(vec3 lhs, float divisor) {
    lhs.x /= divisor;
    lhs.y /= divisor;
    lhs.z /= divisor;
    return lhs;
}

vec2 operator/(vec2 lhs, float divisor) {
    lhs.x /= divisor;
    lhs.y /= divisor;
    return lhs;
}

vec3 operator*(vec3 lhs, float multiplier) {
    lhs.x *= multiplier;
    lhs.y *= multiplier;
    lhs.z *= multiplier;
    return lhs;
}

vec3 operator+(vec3 lhs, float offset) {
    lhs.x += offset;
    lhs.y += offset;
    lhs.z += offset;
    return lhs;
}

vec2 operator+(vec2 lhs, float offset) {
    lhs.x += offset;
    lhs.y += offset;
    return lhs;
}

vec2 operator+(const vec2& lhs, const vec2& rhs) {
    vec2 out;
    out.x = lhs.x + rhs.x;
    out.y = lhs.y + rhs.y;
    return out;
}

namespace vec {

float distance(vec2 a, vec2 b) {
    return sqrtf((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
}

vec3 to3(vec2 position) { return {position.x, 0, position.y}; }

vec2 to2(vec3 position) { return {position.x, position.z}; }

vec2 snap(vec2 position) {
    return {TILESIZE * round(position.x / TILESIZE),  //
            TILESIZE * round(position.y / TILESIZE)};
}
vec3 snap(vec3 position) {
    return {TILESIZE * round(position.x / TILESIZE),  //
            position.y,                               //
            TILESIZE * round(position.z / TILESIZE)};
}

}  // namespace vec
