
#pragma once

#include <limits>

#include "external_include.h"
#include "globals.h"
#include "util.h"
#include "vec_util.h"

namespace astar {

struct ScoreValue {
    float value;
    ScoreValue() { value = std::numeric_limits<float>::max(); }
    ScoreValue(float v) : value(v) {}
    operator float() const { return value; }
};

float path_estimate(const raylib::vec2& a, const raylib::vec2& b) {
    return vec::distance(a, b);
}

float distance_between(const raylib::vec2& a, const raylib::vec2& b) {
    return vec::distance(a, b);
}

raylib::vec2 get_lowest_f(const std::set<raylib::vec2>& set,
                  std::map<raylib::vec2, ScoreValue>& fscore) {
    // TODO
    float bestscore = std::numeric_limits<float>::max();
    raylib::vec2 loc = *(set.begin());
    for (const raylib::vec2 location : set) {
        float score = fscore[location];
        if (score < bestscore) {
            bestscore = score;
            loc = location;
        }
    }
    return loc;
}

std::vector<raylib::vec2> get_neighbors(raylib::vec2 start,
                                std::function<bool(raylib::vec2 pos)> is_walkable) {
    std::vector<raylib::vec2> output;
    int step = static_cast<int>(floor(TILESIZE));
    forEachNeighbor(
        static_cast<int>(start.x), static_cast<int>(start.y),
        [&](const raylib::vec2& v) {
            auto neighbor = vec::snap(v);
            if (is_walkable(neighbor)) {
                output.push_back(neighbor);
            }
        },
        step);
    return output;
}

std::deque<raylib::vec2> reconstruct_path(std::deque<raylib::vec2> path,
                                  std::map<raylib::vec2, raylib::vec2>& parent_map,
                                  raylib::vec2 current) {
    if (!parent_map.contains(current)) {
        return path;
    }
    auto parent = parent_map[current];
    path.push_front(parent);
    return reconstruct_path(path, parent_map, parent);
}

std::deque<raylib::vec2> find_path_impl(raylib::vec2 start, raylib::vec2 end,
                                std::function<bool(raylib::vec2 pos)> is_walkable) {
    if (!is_walkable(end)) {
        return std::deque<raylib::vec2>{};
    }

    std::set<raylib::vec2> openset;
    openset.insert(start);

    std::map<raylib::vec2, raylib::vec2> parent_map;

    std::map<raylib::vec2, ScoreValue> gscore;
    std::map<raylib::vec2, ScoreValue> fscore;

    gscore[start] = 0;
    fscore[start] = 0 + path_estimate(start, end);

    int i = 0;

    while (!openset.empty()) {
        i++;
        if (i > 1000) {
            // std::cout << "astar: hit interation limit" << std::endl;
            break;
        }
        raylib::vec2 cur = get_lowest_f(openset, fscore);

        for (auto it = openset.begin(); it != openset.end();) {
            if (it->x == cur.x && it->y == cur.y)
                it = openset.erase(it);
            else
                ++it;
        }

        // TODO add utils for comparing these
        if (cur.x == end.x && cur.y == end.y) {
            // TODO
            auto path = reconstruct_path({}, parent_map, cur);
            path.push_back(end);
            return path;
        }

        std::vector<raylib::vec2> neighbors = get_neighbors(cur, is_walkable);
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

    return std::deque<raylib::vec2>{};
}

std::deque<raylib::vec2> find_path(raylib::vec2 start, raylib::vec2 end,
                           std::function<bool(raylib::vec2 pos)> is_walkable) {
    return astar::find_path_impl(start, end, is_walkable);
}

}  // namespace astar
