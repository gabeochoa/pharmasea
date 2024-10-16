
#pragma once

#include <limits>

#include "../external_include.h"
#include "../globals.h"
#include "../vec_util.h"
#include "log.h"
#include "util.h"

namespace astar {

struct ScoreValue {
    float value;
    ScoreValue() { value = std::numeric_limits<float>::max(); }
    explicit ScoreValue(float v) : value(v) {}
    operator float() const { return value; }
};

inline float path_estimate(const vec2& a, const vec2& b) {
    return vec::distance(a, b);
}

inline float distance_between(const vec2& a, const vec2& b) {
    return vec::distance(a, b);
}

inline vec2 get_lowest_f(const std::set<vec2>& set,
                         std::map<vec2, ScoreValue>& fscore) {
    float bestscore = std::numeric_limits<float>::max();
    vec2 loc = *(set.begin());
    for (const vec2 location : set) {
        float score = fscore[location];
        if (score < bestscore) {
            bestscore = score;
            loc = location;
        }
    }
    return loc;
}

inline std::vector<vec2> get_neighbors(
    vec2 start, std::function<bool(vec2 pos)> is_walkable) {
    std::vector<vec2> output;
    int step = static_cast<int>(floor(TILESIZE));
    vec::forEachNeighbor(
        static_cast<int>(start.x), static_cast<int>(start.y),
        [&](const vec2& v) {
            auto neighbor = vec::snap(v);
            if (is_walkable(neighbor)) {
                output.push_back(neighbor);
            }
        },
        step);
    return output;
}

inline std::deque<vec2> reconstruct_path(std::deque<vec2> path,
                                         std::map<vec2, vec2>& parent_map,
                                         vec2 current) {
    if (!parent_map.contains(current)) {
        return path;
    }
    auto parent = parent_map[current];
    path.push_front(parent);
    return reconstruct_path(path, parent_map, parent);
}

inline std::deque<vec2> find_path_impl(
    vec2 start, vec2 end, const std::function<bool(vec2 pos)>& is_walkable) {
    if (!is_walkable(end)) {
        return std::deque<vec2>{};
    }

    std::set<vec2> openset;
    openset.insert(start);

    std::map<vec2, vec2> parent_map;

    std::map<vec2, ScoreValue> gscore;
    std::map<vec2, ScoreValue> fscore;

    gscore[start] = ScoreValue(0);
    fscore[start] = ScoreValue(0 + path_estimate(start, end));

    int i = 0;

    while (!openset.empty()) {
        i++;
        vec2 cur = get_lowest_f(openset, fscore);

        if (i > 10000) {
            log_warn("astar: hit interation limit and still {} away",
                     vec::distance(cur, end));
            break;
        }

        for (auto it = openset.begin(); it != openset.end();) {
            if (it->x == cur.x && it->y == cur.y)
                it = openset.erase(it);
            else
                ++it;
        }

        if (vec::distance(cur, end) < (TILESIZE / 2.f)) {
            auto path = reconstruct_path({}, parent_map, cur);
            path.push_back(end);
            return path;
        }

        std::vector<vec2> neighbors = get_neighbors(cur, is_walkable);
        for (const auto& neighbor : neighbors) {
            auto new_gscore = gscore[cur] + distance_between(cur, neighbor);
            auto neighbor_gscore = gscore[neighbor];
            if (new_gscore < neighbor_gscore) {
                parent_map[neighbor] = cur;
                gscore[neighbor] = ScoreValue(new_gscore);
                fscore[neighbor] =
                    ScoreValue(new_gscore + path_estimate(neighbor, end));
                if (!openset.contains(neighbor)) {
                    openset.insert(neighbor);
                }
            }
        }
    }

    return std::deque<vec2>{};
}

// TODO should we add a cache here?
inline std::deque<vec2> find_path(
    vec2 start, vec2 end, const std::function<bool(vec2 pos)>& is_walkable) {
    return astar::find_path_impl(start, end, is_walkable);
}

}  // namespace astar
