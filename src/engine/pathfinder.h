
#pragma once

#include "astar.h"
#include "bfs.h"

namespace pathfinder {

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

inline std::deque<vec2> find_path(
    vec2 start, vec2 end, const std::function<bool(vec2 pos)>& is_walkable) {
    return bfs::find_path(start, end, is_walkable);
}

}  // namespace pathfinder
