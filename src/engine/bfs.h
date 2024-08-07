

#pragma once

#include <deque>

#include "../external_include.h"
#include "../globals.h"
#include "../vec_util.h"
#include "log.h"
#include "util.h"

namespace bfs {
const int MAX_PATH_LENGTH = 50;

static constexpr int neigh_x[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
static constexpr int neigh_y[8] = {-1, 0, 1, -1, 1, -1, 0, 1};

struct bfs {
    struct node {
        vec2 pos;
        node* parent = nullptr;
    };

    vec2 start;
    vec2 end;
    std::function<bool(vec2 pos)> is_walkable;
    std::set<vec2> visited;
    std::deque<node*> all;

    bfs(vec2 start, vec2 end, const std::function<bool(vec2 pos)>& is_walkable)
        : start(start), end(end), is_walkable(is_walkable) {}

    ~bfs() {
        for (size_t i = 0; i < all.size(); i++) {
            delete all[i];
        }
        all.clear();
    }

    std::deque<vec2> reconstruct(node* n) {
        std::deque<vec2> path;
        node* cur = n;
        while (cur) {
            path.push_back(cur->pos);
            cur = cur->parent;
        }
        return path;
    }

    std::deque<vec2> get_path() {
        visited.insert(start);

        std::deque<node*> queue;

        all.push_back(new node{.pos = start, .parent = nullptr});
        queue.push_back(all.back());

        while (!queue.empty()) {
            node* n = queue.front();
            queue.pop_front();
            if (vec::distance_sq(n->pos, end) < 4.f) {
                queue.clear();
                return reconstruct(n);
            }

            if (vec::distance_sq(n->pos, end) >
                (MAX_PATH_LENGTH * MAX_PATH_LENGTH)) {
                continue;
            }

            std::vector<vec2> neighbors;
            {
                neighbors.reserve(8);
                int i = static_cast<int>(n->pos.x);
                int j = static_cast<int>(n->pos.y);
                for (int a = 0; a < 8; a++) {
                    auto position = (vec2{(float) i + (neigh_x[a]),
                                          (float) j + (neigh_y[a])});
                    auto neighbor = vec::snap(position);
                    if (is_walkable(neighbor)) {
                        neighbors.push_back(neighbor);
                    }
                }
            }

            for (const auto neighbor : neighbors) {
                if (visited.contains(neighbor)) continue;
                visited.insert(neighbor);
                all.push_back(new node{.pos = neighbor, .parent = n});
                queue.push_back(all.back());
            }
        }

        queue.clear();
        return {};
    }
};

inline std::deque<vec2> find_path(
    vec2 start, vec2 end, const std::function<bool(vec2 pos)>& is_walkable) {
    // reverse order since we dont have directional pathing anyway and
    // then we dont have to reverse the path
    bfs d{end, start, is_walkable};
    return d.get_path();
}
}  // namespace bfs
