#pragma once


#include "external_include.h"
#include "globals.h"

#ifndef EPSILON
#define EPSILON 0.000001f
#endif

bool operator<(const vec2& l, const vec2& r) {
    if (l.x < r.x) return true;
    if (l.y < r.y) return true;
    return false;
}

bool operator==(const vec2& p, const vec2& q) {
    return ((fabsf(p.x - q.x)) <=
            (EPSILON * fmaxf(1.0f, fmaxf(fabsf(p.x), fabsf(q.x))))) &&
           ((fabsf(p.y - q.y)) <=
            (EPSILON * fmaxf(1.0f, fmaxf(fabsf(p.y), fabsf(q.y)))));
}

std::ostream& operator<<(std::ostream& os, const vec2& v) {
    os << "vec(" << v.x << ", " << v.y << ")";
    return os;
}

namespace vec {

float distance(vec2 a, vec2 b) {
    return sqrtf((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
}

vec2 snap(vec2 position) {
    return {TILESIZE * floor(position.x / TILESIZE),  //
            TILESIZE * floor(position.y / TILESIZE)};
}
vec3 snap(vec3 position) {
    return {TILESIZE * floor(position.x / TILESIZE),  //
            position.y,                               //
            TILESIZE * floor(position.z / TILESIZE)};
}

}  // namespace vec
