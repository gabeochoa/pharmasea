
#pragma once

#include "astar.h"
#include "bfs.h"
#include "path_request_manager.h"

namespace pathfinder {

inline std::deque<vec2> find_path(
    vec2 start, vec2 end, const std::function<bool(vec2 pos)>& is_walkable) {
    return bfs::find_path(start, end, is_walkable);
}

}  // namespace pathfinder
