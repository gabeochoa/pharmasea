#pragma once

#include "external_include.h"
#include "globals.h"

#ifndef EPSILON
#define EPSILON 0.000001f
#endif

static constexpr int neigh_x[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
static constexpr int neigh_y[8] = {-1, 0, 1, -1, 1, -1, 0, 1};

static void forEachNeighbor(int i, int j, std::function<void(const vec2&)> cb,
                            int step = 1) {
    for (int a = 0; a < 8; a++) {
        cb(vec2{(float) i + (neigh_x[a] * step),
                (float) j + (neigh_y[a] * step)});
    }
}

static std::vector<vec2> get_neighbors(int i, int j, int step = 1) {
    std::vector<vec2> ns;
    forEachNeighbor(
        i, j, [&](const vec2& v) { ns.push_back(v); }, step);
    return ns;
}

inline float comp_max(const vec2& a) { return fmax(a.x, a.y); }

constexpr BoundingBox get_bounds(vec3 position, vec3 size) {
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

float constexpr newton_raphson(float x, float cur, float prev) {
    return cur == prev ? cur : newton_raphson(x, 0.5f * (cur + x / cur), cur);
}

float constexpr ce_sqrtf(float x) {
    return x >= 0 && x < std::numeric_limits<float>::infinity()
               ? newton_raphson(x, x, 0)
               : std::numeric_limits<float>::quiet_NaN();
}

constexpr float distance(vec2 a, vec2 b) {
    return ce_sqrtf((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
}

float dot2(const vec2& a, const vec2& b) { return (a.x * b.x + a.y * b.y); }

vec2 norm(const vec2& a) {
    float mag = dot2(a, a);
    return (a / mag);
}

constexpr vec3 to3(vec2 position) { return {position.x, 0, position.y}; }

constexpr vec2 to2(vec3 position) { return {position.x, position.z}; }

vec2 snap(vec2 position) {
    return {TILESIZE * round(position.x / TILESIZE),  //
            TILESIZE * round(position.y / TILESIZE)};
}
vec3 snap(vec3 position) {
    return {TILESIZE * round(position.x / TILESIZE),  //
            position.y,                               //
            TILESIZE * round(position.z / TILESIZE)};
}

vec2 lerp(vec2 a, vec2 b, float pct) {
    return vec2{
        (a.x * (1 - pct)) + (b.x * pct),
        (a.y * (1 - pct)) + (b.y * pct),
    };
}

}  // namespace vec
