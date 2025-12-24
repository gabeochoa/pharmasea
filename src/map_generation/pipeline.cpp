#include "pipeline.h"

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

#include "../engine/globals.h"
#include "map_generation.h"
#include "simple.h"

namespace mapgen {

namespace {

BarArchetype pick_archetype_from_seed(const std::string& seed) {
    size_t h = hashString(seed);
    int bucket = static_cast<int>(h % 100);
    if (bucket < 40) return BarArchetype::OpenHall;
    if (bucket < 75) return BarArchetype::MultiRoom;
    if (bucket < 90) return BarArchetype::BackRoom;
    return BarArchetype::LoopRing;
}

int num_seeds_for_archetype(BarArchetype archetype, const GenerationContext& ctx,
                            const std::string& seed) {
    int base = std::max(1, static_cast<int>((ctx.rows * ctx.cols) / 100));
    size_t h = hashString(seed);
    int jitter = static_cast<int>(h % 3);
    if (archetype == BarArchetype::OpenHall) return std::max(1, base - 1 + jitter);
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

std::vector<std::string> stage_layout(const std::string& seed,
                                      const GenerationContext& ctx,
                                      BarArchetype archetype) {
    int num_seeds = num_seeds_for_archetype(archetype, ctx, seed);
    std::vector<char> grid = something(ctx.cols, ctx.rows, num_seeds, false);
    return grid_to_lines(grid, ctx.rows, ctx.cols);
}

std::vector<std::string> stage_required_placement(std::vector<std::string> lines,
                                                  int rows) {
    std::vector<char> required = {{
        generation::CUST_SPAWNER,
        generation::SODA_MACHINE,
        generation::TRASH,
        generation::REGISTER,
        generation::ORIGIN,
        generation::SOPHIE,
        generation::FAST_FORWARD,
        generation::CUPBOARD,
        generation::TABLE,
        generation::TABLE,
        generation::TABLE,
        generation::TABLE,
    }};

    std::string tmp;
    tmp.reserve(static_cast<size_t>(rows));
    for (char c : required) {
        tmp.push_back(c);
        if (tmp.size() == static_cast<size_t>(rows)) {
            lines.push_back(tmp);
            tmp.clear();
        }
    }
    if (!tmp.empty()) lines.push_back(tmp);
    return lines;
}

std::vector<std::string> stage_decoration(std::vector<std::string> lines) {
    return lines;
}

}  // namespace

GeneratedAscii generate_ascii(const std::string& seed,
                             const GenerationContext& context) {
    BarArchetype archetype = pick_archetype_from_seed(seed);

    std::vector<std::string> lines = stage_layout(seed, context, archetype);
    lines = stage_required_placement(std::move(lines), context.rows);
    lines = stage_decoration(std::move(lines));

    GeneratedAscii out;
    out.lines = std::move(lines);
    out.archetype = archetype;
    return out;
}

}  // namespace mapgen

