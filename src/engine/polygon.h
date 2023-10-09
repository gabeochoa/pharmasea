

#pragma once

#include <cstddef>
#include <vector>

#include "../external_include.h"
#include "../vec_util.h"
#include "log.h"

// grahamScan
// https://www.geeksforgeeks.org/dynamic-convex-hull-adding-points-existing-convex-hull/?ref=rp

// testing points
// https://www.nayuki.io/page/convex-hull-algorithm

struct Polygon {
    std::vector<vec2> points;
    std::vector<vec2> hull;

    Polygon() {}

    explicit Polygon(Rectangle rect) {
        this->add(vec2{rect.x, rect.y});
        this->add(vec2{rect.x + rect.width, rect.y});
        this->add(vec2{rect.x, rect.y + rect.height});
    }

    std::vector<vec3> as_3d() {
        std::vector<vec3> three_d;
        three_d.reserve(hull.size());

        for (auto p : hull) {
            three_d.push_back(vec::to3(p));
        }
        return three_d;
    }

    // checks whether the point crosses the convex hull
    // or not
    int orientation(vec2 a, vec2 b, vec2 c) const {
        float res_f = (b.y - a.y) * (c.x - b.x) - (c.y - b.y) * (b.x - a.x);
        int res = (int) floor(res_f);
        if (res == 0) return 0;
        if (res > 0) return 1;
        return -1;
    }

    // Returns the square of distance between two input points
    float sqDist(vec2 p1, vec2 p2) const {
        return (p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y);
    }

    vec2 centroid() const {
        vec2 mid = {0, 0};
        for (size_t i = 0; i < hull.size(); i++) {
            mid.x += hull[i].x;
            mid.y += hull[i].y;
        }
        return mid;
    }

    float maxradius() const {
        vec2 mid = centroid();
        float maxdst = 0.f;

        for (size_t i = 0; i < hull.size(); i++) {
            float dst = sqDist(mid, hull[i]);
            maxdst = fmax(maxdst, dst);
        }

        return sqrt(maxdst);
    }

    // Checks whether the point is inside the convex hull or not
    bool inside(vec2 p) const {
        // Initialize the centroid of the convex hull
        vec2 mid = {0, 0};

        if (hull.size() < 3) return false;

        std::vector<vec2> hullcopy(hull);

        int n = static_cast<int>(hullcopy.size());

        // Multiplying with n to avoid floating point
        // arithmetic.
        p.x *= n;
        p.y *= n;
        for (int i = 0; i < n; i++) {
            mid.x += hullcopy[i].x;
            mid.y += hullcopy[i].y;
            hullcopy[i].x *= n;
            hullcopy[i].y *= n;
        }

        // if the mid and the given point lies always
        // on the same side w.r.t every edge of the
        // convex hullcopy, then the point lies inside
        // the convex hullcopy
        for (int i = 0, j; i < n; i++) {
            j = (i + 1) % n;
            auto x1 = hullcopy[i].x, x2 = hullcopy[j].x;
            auto y1 = hullcopy[i].y, y2 = hullcopy[j].y;
            auto a1 = y1 - y2;
            auto b1 = x2 - x1;
            auto c1 = x1 * y2 - y1 * x2;
            auto for_mid = a1 * mid.x + b1 * mid.y + c1;
            auto for_p = a1 * p.x + b1 * p.y + c1;
            if (for_mid * for_p < 0) return false;
        }

        return true;
    }

    // TODO split work into add / compute
    // so that we can add 4 points at a time (at least)

    // Adds a point p to given convex hull a[]
    void add(vec2 p) {
        points.push_back(p);
        if (hull.size() <= 3) {
            hull.push_back(p);
            return;
        }

        andrew_chain();
        // grahamScan(p);
    }

    float cross(vec2 o, vec2 a, vec2 b) const {
        return (a.x - o.x) * (b.y - o.y) - (a.y - o.y) * (b.x - o.x);
    }

    struct CompareVec {
        bool operator()(const vec2& a, const vec2& b) const {
            return a.x < b.x || (a.x == b.x && a.y < b.y);
        }
    };

    void remove(vec2 p) {
        auto hasp = std::find(points.begin(), points.end(), p);
        if (hasp == points.end()) {
            if (inside(p)) {
                log_trace(
                    "Trying to remove a point that is inside the nav mesh");
            } else {
                log_warn(
                    "Trying to remove a point that isnt in the original nav "
                    "set mesh");
            }
            return;
        }
        points.erase(hasp);
        andrew_chain();
    }

    // https://en.wikibooks.org/wiki/Algorithm_Implementation/Geometry/Convex_hull/Monotone_chain#C++
    void andrew_chain() {
        int n = static_cast<int>(points.size());
        if (n <= 3) {
            hull = points;
            return;
        }
        int k = 0;

        std::vector<vec2> h(2 * n);
        std::sort(points.begin(), points.end(), CompareVec());

        for (int i = 0; i < n; i++) {
            while (k >= 2 && cross(h[k - 2], h[k - 1], points[i]) <= 0) k--;
            h[k++] = points[i];
        }

        for (int i = n - 1, t = k + 1; i > 0; i--) {
            while (k >= t && cross(h[k - 2], h[k - 1], points[i - 1]) <= 0) k--;
            h[k++] = points[i - 1];
        }

        h.resize(k - 1);
        hull = h;
    }

    void grahamScan(vec2 p) {
        // If point is inside p
        if (inside(p)) return;

        // point having minimum distance from the point p
        int ind = 0;
        int n = (int) hull.size();
        for (int i = 1; i < n; i++)
            if (sqDist(p, hull[i]) < sqDist(p, hull[ind])) ind = i;

        // Find the upper tangent
        int up = ind;
        while (orientation(p, hull[up], hull[(up + 1) % n]) >= 0)
            up = (up + 1) % n;

        // Find the lower tangent
        int low = ind;
        while (orientation(p, hull[low], hull[(n + low - 1) % n]) <= 0)
            low = (n + low - 1) % n;

        // Initialize result
        std::vector<vec2> ret;

        // making the final hull by traversing points
        // from up to low of given convex hull.
        int curr = up;
        ret.push_back(hull[curr]);
        while (curr != low) {
            curr = (curr + 1) % n;
            ret.push_back(hull[curr]);
        }

        // Modify the original vector
        ret.push_back(p);
        hull.clear();
        for (size_t i = 0; i < ret.size(); i++) hull.push_back(ret[i]);
    }
};
