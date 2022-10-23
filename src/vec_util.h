#pragma once

#include "external_include.h"
#include "globals.h"

#ifndef EPSILON
#define EPSILON 0.000001f
#endif

static constexpr int neighbor_x[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
static constexpr int neighbor_y[8] = {-1, 0, 1, -1, 1, -1, 0, 1};

static void forEachNeighbor(int i, int j, std::function<void(const vec2&)> cb,
                     int step = 1) {
    for (int a = 0; a < 8; a++) {
        cb(vec2{(float) i + (neighbor_x[a] * step), (float) j + (neighbor_y[a] * step)});
    }
}

static std::vector<vec2> get_neighbors(int i, int j, int step = 1) {
    std::vector<vec2> ns;
    forEachNeighbor(
        i, j, [&](const vec2& v) { ns.push_back(v); }, step);
    return ns;
}

inline float comp_max(const vec2& a){
    return fmax(a.x, a.y);
}

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

vec2 operator*(vec2 lhs, float multiplier) {
    lhs.x *= multiplier;
    lhs.y *= multiplier;
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

vec3 operator+(vec3 lhs, vec3 rhs) {
    lhs.x += rhs.x;
    lhs.y += rhs.y;
    lhs.z += rhs.z;
    return lhs;
}

vec2 operator+(vec2 lhs, float offset) {
    lhs.x += offset;
    lhs.y += offset;
    return lhs;
}

vec2 operator+=(vec2 lhs, float offset) {
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

// TODO this wasnt working, so im disabling it until we can figure out why
// vec2 operator+=(const vec2& lhs, const vec2& rhs) {
// vec2 out;
// out.x = lhs.x + rhs.x;
// out.y = lhs.y + rhs.y;
// return out;
// }

BoundingBox get_bounds(vec3 position, vec3 size) {
    return {(vec3){
                position.x - size.x / 2,
                position.y - size.y / 2,
                position.z - size.z / 2,
            },
            (vec3){
                position.x + size.x / 2,
                position.y + size.y / 2,
                position.z + size.z / 2,
            }};
}

namespace vec {

float distance(vec2 a, vec2 b) {
    return sqrtf((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
}
float dot2(const vec2& a, const vec2& b) {
    float result = (a.x * b.x + a.y * b.y);
    return result;
}

vec2 norm(const vec2& a) {
    float mag = dot2(a, a);
    return (a / mag);
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
