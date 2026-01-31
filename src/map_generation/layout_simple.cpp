#include "layout_simple.h"

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

#include "../engine/globals.h"
#include "../engine/random_engine.h"
#include "simple.h"

namespace mapgen {

namespace {

int num_seeds_for_archetype(BarArchetype archetype,
                            const GenerationContext& ctx,
                            const std::string& seed) {
    int base = std::max(1, static_cast<int>((ctx.rows * ctx.cols) / 100));
    size_t h = hashString(seed);
    int jitter = static_cast<int>(h % 3);
    if (archetype == BarArchetype::OpenHall)
        return std::max(1, base - 1 + jitter);
    if (archetype == BarArchetype::MultiRoom) return base + 2 + jitter;
    if (archetype == BarArchetype::BackRoom) return base + 1 + jitter;
    return base + 3 + jitter;
}

std::vector<std::string> grid_to_lines(const std::vector<char>& grid, int rows,
                                       int cols) {
    std::vector<std::string> lines;
    lines.reserve(static_cast<size_t>(rows));
    for (int i = 0; i < rows; i++) {
        std::string line;
        line.reserve(static_cast<size_t>(cols));
        for (int j = 0; j < cols; j++) {
            line.push_back(grid[static_cast<size_t>(i * cols + j)]);
        }
        lines.push_back(std::move(line));
    }
    return lines;
}

}  // namespace

std::vector<std::string> generate_layout_simple(const std::string& seed,
                                                const GenerationContext& ctx,
                                                BarArchetype archetype,
                                                int attempt_index) {
    int num_seeds = num_seeds_for_archetype(archetype, ctx, seed);
    // Deterministic per attempt.
    RandomEngine::set_seed(fmt::format("{}:layout:{}", seed, attempt_index));
    std::vector<char> grid = something(ctx.cols, ctx.rows, num_seeds, false);
    return grid_to_lines(grid, ctx.rows, ctx.cols);
}

}  // namespace mapgen
