#pragma once

// From Raylib Extras https://github.com/JeffM2501/raylibExtras
// see copyright at the bottom

#include "../external_include.h"

struct Frustum {
    enum class FrustumPlanes {
        Back = 0,
        Front = 1,
        Bottom = 2,
        Top = 3,
        Right = 4,
        Left = 5,
        MAX = 6
    };

    std::map<FrustumPlanes, vec4> Planes;

    Frustum() {
        Planes[FrustumPlanes::Right] = vec4{0};
        Planes[FrustumPlanes::Left] = vec4{0};
        Planes[FrustumPlanes::Top] = vec4{0};
        Planes[FrustumPlanes::Bottom] = vec4{0};
        Planes[FrustumPlanes::Front] = vec4{0};
        Planes[FrustumPlanes::Back] = vec4{0};
    }
    ~Frustum() {}

    void fetch_data() {
        raylib::Matrix proj = raylib::rlGetMatrixProjection();
        raylib::Matrix mv = raylib::rlGetMatrixModelview();

        raylib::Matrix planes = {0};

        planes.m0 = mv.m0 * proj.m0 + mv.m1 * proj.m4 + mv.m2 * proj.m8 +
                    mv.m3 * proj.m12;
        planes.m1 = mv.m0 * proj.m1 + mv.m1 * proj.m5 + mv.m2 * proj.m9 +
                    mv.m3 * proj.m13;
        planes.m2 = mv.m0 * proj.m2 + mv.m1 * proj.m6 + mv.m2 * proj.m10 +
                    mv.m3 * proj.m14;
        planes.m3 = mv.m0 * proj.m3 + mv.m1 * proj.m7 + mv.m2 * proj.m11 +
                    mv.m3 * proj.m15;
        planes.m4 = mv.m4 * proj.m0 + mv.m5 * proj.m4 + mv.m6 * proj.m8 +
                    mv.m7 * proj.m12;
        planes.m5 = mv.m4 * proj.m1 + mv.m5 * proj.m5 + mv.m6 * proj.m9 +
                    mv.m7 * proj.m13;
        planes.m6 = mv.m4 * proj.m2 + mv.m5 * proj.m6 + mv.m6 * proj.m10 +
                    mv.m7 * proj.m14;
        planes.m7 = mv.m4 * proj.m3 + mv.m5 * proj.m7 + mv.m6 * proj.m11 +
                    mv.m7 * proj.m15;
        planes.m8 = mv.m8 * proj.m0 + mv.m9 * proj.m4 + mv.m10 * proj.m8 +
                    mv.m11 * proj.m12;
        planes.m9 = mv.m8 * proj.m1 + mv.m9 * proj.m5 + mv.m10 * proj.m9 +
                    mv.m11 * proj.m13;
        planes.m10 = mv.m8 * proj.m2 + mv.m9 * proj.m6 + mv.m10 * proj.m10 +
                     mv.m11 * proj.m14;
        planes.m11 = mv.m8 * proj.m3 + mv.m9 * proj.m7 + mv.m10 * proj.m11 +
                     mv.m11 * proj.m15;
        planes.m12 = mv.m12 * proj.m0 + mv.m13 * proj.m4 + mv.m14 * proj.m8 +
                     mv.m15 * proj.m12;
        planes.m13 = mv.m12 * proj.m1 + mv.m13 * proj.m5 + mv.m14 * proj.m9 +
                     mv.m15 * proj.m13;
        planes.m14 = mv.m12 * proj.m2 + mv.m13 * proj.m6 + mv.m14 * proj.m10 +
                     mv.m15 * proj.m14;
        planes.m15 = mv.m12 * proj.m3 + mv.m13 * proj.m7 + mv.m14 * proj.m11 +
                     mv.m15 * proj.m15;

        Planes[FrustumPlanes::Right] = {
            planes.m3 - planes.m0, planes.m7 - planes.m4,
            planes.m11 - planes.m8, planes.m15 - planes.m12};
        normalize_plane(Planes[FrustumPlanes::Right]);

        Planes[FrustumPlanes::Left] = {
            planes.m3 + planes.m0, planes.m7 + planes.m4,
            planes.m11 + planes.m8, planes.m15 + planes.m12};
        normalize_plane(Planes[FrustumPlanes::Left]);

        Planes[FrustumPlanes::Top] = {
            planes.m3 - planes.m1, planes.m7 - planes.m5,
            planes.m11 - planes.m9, planes.m15 - planes.m13};
        normalize_plane(Planes[FrustumPlanes::Top]);

        Planes[FrustumPlanes::Bottom] = {
            planes.m3 + planes.m1, planes.m7 + planes.m5,
            planes.m11 + planes.m9, planes.m15 + planes.m13};
        normalize_plane(Planes[FrustumPlanes::Bottom]);

        Planes[FrustumPlanes::Back] = {
            planes.m3 - planes.m2, planes.m7 - planes.m6,
            planes.m11 - planes.m10, planes.m15 - planes.m14};
        normalize_plane(Planes[FrustumPlanes::Back]);

        Planes[FrustumPlanes::Front] = {
            planes.m3 + planes.m2, planes.m7 + planes.m6,
            planes.m11 + planes.m10, planes.m15 + planes.m14};
        normalize_plane(Planes[FrustumPlanes::Front]);
    }

    [[nodiscard]] bool point_inside(float x, float y, float z) const {
        for (auto& plane : Planes) {
            if (distance_to_plane(plane.second, x, y, z) <=
                0)  // point is behind plane
                return false;
        }

        return true;
    }

    [[nodiscard]] bool point_inside(vec3 pos) const {
        return point_inside(pos.x, pos.y, pos.z);
    }

    [[nodiscard]] bool sphere_inside(const vec3& position, float radius) const {
        for (auto& plane : Planes) {
            if (distance_to_plane(plane.second, position) <
                -radius)  // center is behind plane by more than the radius
                return false;
        }

        return true;
    }

    [[nodiscard]] bool AABBoxIn(const vec3& min, const vec3& max) const {
        // if any point is in and we are good
        if (point_inside(min.x, min.y, min.z)) return true;
        if (point_inside(min.x, max.y, min.z)) return true;
        if (point_inside(max.x, max.y, min.z)) return true;
        if (point_inside(max.x, min.y, min.z)) return true;
        if (point_inside(min.x, min.y, max.z)) return true;
        if (point_inside(min.x, max.y, max.z)) return true;
        if (point_inside(max.x, max.y, max.z)) return true;
        if (point_inside(max.x, min.y, max.z)) return true;

        // check to see if all points are outside of any one plane, if so the
        // entire box is outside
        for (auto& plane : Planes) {
            bool oneInside = false;

            if (distance_to_plane(plane.second, min.x, min.y, min.z) >= 0)
                oneInside = true;

            if (distance_to_plane(plane.second, max.x, min.y, min.z) >= 0)
                oneInside = true;

            if (distance_to_plane(plane.second, max.x, max.y, min.z) >= 0)
                oneInside = true;

            if (distance_to_plane(plane.second, min.x, max.y, min.z) >= 0)
                oneInside = true;

            if (distance_to_plane(plane.second, min.x, min.y, max.z) >= 0)
                oneInside = true;

            if (distance_to_plane(plane.second, max.x, min.y, max.z) >= 0)
                oneInside = true;

            if (distance_to_plane(plane.second, max.x, max.y, max.z) >= 0)
                oneInside = true;

            if (distance_to_plane(plane.second, min.x, max.y, max.z) >= 0)
                oneInside = true;

            if (!oneInside) return false;
        }

        // the box extends outside the frustum but crosses it
        return true;
    }

   private:
    void normalize_plane(vec4& plane) {
        float magnitude =
            sqrtf(plane.x * plane.x + plane.y * plane.y + plane.z * plane.z);

        plane.x /= magnitude;
        plane.y /= magnitude;
        plane.z /= magnitude;
        plane.w /= magnitude;
    }

    [[nodiscard]] float distance_to_plane(const vec4& plane, float x, float y,
                                          float z) const {
        return (plane.x * x + plane.y * y + plane.z * z + plane.w);
    }

    [[nodiscard]] float distance_to_plane(const vec4& plane,
                                          const vec3& pos) const {
        return distance_to_plane(plane, pos.x, pos.y, pos.z);
    }
};

/**********************************************************************************************
 *
 *   raylibExtras * Utilities and Shared Components for Raylib
 *
 *   RLAssets * Simple Asset Managment System for Raylib
 *
 *   LICENSE: MIT
 *
 *   Copyright (c) 2020 Jeffery Myers
 *
 *   Permission is hereby granted, free of charge, to any person obtaining a
 *copy of this software and associated documentation files (the "Software"), to
 *deal in the Software without restriction, including without limitation the
 *rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 *sell copies of the Software, and to permit persons to whom the Software is
 *   furnished to do so, subject to the following conditions:
 *
 *   The above copyright notice and this permission notice shall be included in
 *all copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 *IN THE SOFTWARE.
 *
 **********************************************************************************************/
