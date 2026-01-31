#include "day1_required_placement.h"

#include <algorithm>
#include <cstddef>

#include "../components/has_waiting_queue.h"
#include "ascii_grid.h"
#include "map_generation.h"

namespace mapgen {

namespace {

static bool place_one(std::vector<std::string>& lines, char sym,
                      std::mt19937& rng,
                      const std::vector<std::vector<bool>>& blocked) {
    auto [h, w] = grid::dims(lines);
    std::vector<grid::Cell> candidates;
    candidates.reserve((size_t) (h * w));
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            if (!grid::in_bounds(lines, i, j)) continue;
            if (blocked[(size_t) i][(size_t) j]) continue;
            if (grid::get(lines, i, j) != generation::EMPTY) continue;
            candidates.push_back(grid::Cell{i, j});
        }
    }
    std::shuffle(candidates.begin(), candidates.end(), rng);
    if (candidates.empty()) return false;
    grid::set(lines, candidates[0].i, candidates[0].j, sym);
    return true;
}

}  // namespace

bool place_required_day1(std::vector<std::string>& lines, std::mt19937& rng) {
    auto [h, w] = grid::dims(lines);
    if (h <= 0 || w <= 0) return false;

    // Clear any pre-existing required symbols (keep structural + origin).
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            char ch = grid::get(lines, i, j);
            if (ch == generation::WALL || ch == generation::WALL2 ||
                ch == generation::EMPTY || ch == generation::ORIGIN) {
                continue;
            }
            grid::set(lines, i, j, generation::EMPTY);
        }
    }

    grid::ensure_single_origin(lines);

    // Pick a register location where queue strip (default FORWARD) is clear.
    std::vector<grid::Cell> reg_candidates;
    for (int i = 1; i < h - 1; i++) {
        for (int j = 1; j < w - (HasWaitingQueue::max_queue_size + 1); j++) {
            if (grid::get(lines, i, j) != generation::EMPTY) continue;

            bool ok = true;
            for (int dj = 1; dj <= HasWaitingQueue::max_queue_size; dj++) {
                if (grid::get(lines, i, j + dj) != generation::EMPTY) {
                    ok = false;
                    break;
                }
            }
            if (!ok) continue;

            reg_candidates.push_back(grid::Cell{i, j});
        }
    }
    std::shuffle(reg_candidates.begin(), reg_candidates.end(), rng);
    if (reg_candidates.empty()) return false;

    // Try a few register candidates until we can place a spawner with a path to
    // the queue-front tile.
    for (size_t reg_try = 0;
         reg_try < std::min<size_t>(reg_candidates.size(), 50); reg_try++) {
        grid::Cell reg = reg_candidates[reg_try];
        grid::Cell queue_front{reg.i, reg.j + 1};

        // Try spawner placements.
        std::vector<grid::Cell> spawner_candidates;
        spawner_candidates.reserve((size_t) (h * w));
        for (int i = 1; i < h - 1; i++) {
            for (int j = 1; j < w - 1; j++) {
                if (grid::get(lines, i, j) != generation::EMPTY) continue;
                // Avoid occupying the queue strip.
                if (i == reg.i && j >= reg.j &&
                    j <= reg.j + HasWaitingQueue::max_queue_size) {
                    continue;
                }
                spawner_candidates.push_back(grid::Cell{i, j});
            }
        }
        std::shuffle(spawner_candidates.begin(), spawner_candidates.end(), rng);
        if (spawner_candidates.empty()) continue;

        for (size_t sp_try = 0;
             sp_try < std::min<size_t>(spawner_candidates.size(), 150);
             sp_try++) {
            // Work on a scratch copy while we test pathability.
            std::vector<std::string> scratch = lines;
            grid::set(scratch, reg.i, reg.j, generation::REGISTER);
            grid::Cell sp = spawner_candidates[sp_try];
            grid::set(scratch, sp.i, sp.j, generation::CUST_SPAWNER);

            auto path = grid::bfs_path_4_neighbor(scratch, sp, queue_front);
            if (path.empty()) continue;

            // Commit base required placement.
            lines = std::move(scratch);

            // Protect queue strip + found path.
            std::vector<std::vector<bool>> blocked(
                (size_t) h, std::vector<bool>((size_t) w, false));
            for (int dj = 1; dj <= HasWaitingQueue::max_queue_size; dj++) {
                blocked[(size_t) reg.i][(size_t) (reg.j + dj)] = true;
            }
            for (const grid::Cell& p : path) {
                blocked[(size_t) p.i][(size_t) p.j] = true;
            }

            // Place remaining required entities away from the critical path.
            if (!place_one(lines, generation::SODA_MACHINE, rng, blocked))
                return false;
            if (!place_one(lines, generation::CUPBOARD, rng, blocked))
                return false;
            if (!place_one(lines, generation::TRASH, rng, blocked))
                return false;
            if (!place_one(lines, generation::FAST_FORWARD, rng, blocked))
                return false;
            if (!place_one(lines, generation::SOPHIE, rng, blocked))
                return false;
            if (!place_one(lines, generation::TABLE, rng, blocked))
                return false;

            return true;
        }
    }

    return false;
}

}  // namespace mapgen
