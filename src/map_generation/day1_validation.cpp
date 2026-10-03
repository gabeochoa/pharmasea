#include "day1_validation.h"

#include <optional>
#include <vector>

#include "../components/has_waiting_queue.h"
#include "ascii_grid.h"
#include "map_generation.h"
#include "playability_spec.h"

namespace mapgen {

bool validate_day1_ascii_plus_routing(const std::vector<std::string>& lines) {
    auto report = mapgen::playability::validate_ascii_day1(lines);
    if (!report.ok()) return false;

    // Additional runtime-aligned checks:
    // - Register queue tiles (default facing FORWARD) must be floor
    // - Spawner can path to register queue-front under ASCII routing rules
    auto [h, w] = grid::dims(lines);
    std::optional<grid::Cell> spawner;
    std::optional<grid::Cell> reg;
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            if (grid::get(lines, i, j) == generation::CUST_SPAWNER)
                spawner = grid::Cell{i, j};
            if (grid::get(lines, i, j) == generation::REGISTER)
                reg = grid::Cell{i, j};
        }
    }
    if (!spawner.has_value() || !reg.has_value()) return false;

    // Ensure queue strip is clear (tiles ahead are floor).
    for (int dj = 1; dj <= HasWaitingQueue::max_queue_size; dj++) {
        if (!grid::in_bounds(lines, reg->i, reg->j + dj)) return false;
        if (grid::get(lines, reg->i, reg->j + dj) != generation::EMPTY)
            return false;
    }

    grid::Cell goal{reg->i, reg->j + 1};

    // Build a routing view: only '.' and '0' are traversable.
    std::vector<std::string> routing = lines;
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            char ch = routing[(size_t) i][(size_t) j];
            if (grid::is_walkable_for_routing(ch)) continue;
            // Allow BFS to start on spawner tile even though it's non-walkable.
            if (i == spawner->i && j == spawner->j) continue;
            routing[(size_t) i][(size_t) j] = generation::WALL;
        }
    }

    auto path = grid::bfs_path_4_neighbor(routing, *spawner, goal);
    return !path.empty();
}

}  // namespace mapgen
