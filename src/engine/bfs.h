

#pragma once

#include <deque>

#include "../external_include.h"
#include "../globals.h"
#include "../vec_util.h"
#include "log.h"
#include "util.h"

namespace bfs {
const int MAX_PATH_LENGTH = 50;

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

    std::vector<vec2> get_neighbors(vec2 pos) {
        std::vector<vec2> output;
        vec::forEachNeighbor(
            static_cast<int>(pos.x), static_cast<int>(pos.y),
            [&](const vec2& v) {
                auto neighbor = vec::snap(v);
                if (is_walkable(neighbor)) {
                    output.push_back(neighbor);
                }
            },
            1);
        return output;
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
            if (vec::distance(n->pos, end) < 2.f) {
                queue.clear();
                return reconstruct(n);
            }

            if (vec::distance(n->pos, end) > MAX_PATH_LENGTH) {
                continue;
            }

            const auto neighbors = get_neighbors(n->pos);

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
