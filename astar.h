
#pragma once

#include "external_include.h"

namespace astar {

static std::vector<vec2> find_path(vec2 start, vec2 end,
                                   std::function<bool(vec2 pos)> is_walkable) {
    return astar::find_path_impl(start, end, is_walkable);
}

static std::vector<vec2> find_path_impl(
    vec2 start, vec2 end, std::function<bool(vec2 pos)> is_walkable) {
    return std::vector<vec2>{};
}

}  // namespace astar
