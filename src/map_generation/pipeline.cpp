#include "pipeline.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <optional>
#include <random>
#include <string>
#include <string_view>
#include <vector>

#include "../engine/globals.h"
#include "../components/has_waiting_queue.h"
#include "map_generation.h"
#include "playability_spec.h"
#include "simple.h"
#include "wave_collapse.h"

namespace mapgen {

namespace {

struct Point {
    int i = 0;
    int j = 0;
};

static std::string_view lstrip_prefix(std::string_view s,
                                      std::string_view prefix) {
    if (s.size() < prefix.size()) return s;
    if (s.substr(0, prefix.size()) != prefix) return s;
    return s.substr(prefix.size());
}

static bool is_walkable_for_routing(char ch) {
    // Treat only floor/origin as walkable for pathing.
    return ch == generation::EMPTY || ch == generation::ORIGIN;
}

static bool in_bounds(const std::vector<std::string>& lines, int i, int j) {
    if (i < 0 || j < 0) return false;
    if (i >= (int) lines.size()) return false;
    if (j >= (int) lines[i].size()) return false;
    return true;
}

static char get(const std::vector<std::string>& lines, int i, int j) {
    if (!in_bounds(lines, i, j)) return generation::WALL;
    return lines[i][j];
}

static void set(std::vector<std::string>& lines, int i, int j, char ch) {
    if (!in_bounds(lines, i, j)) return;
    lines[i][j] = ch;
}

static std::pair<int, int> dims(const std::vector<std::string>& lines) {
    int h = (int) lines.size();
    int w = 0;
    for (const auto& l : lines) w = std::max(w, (int) l.size());
    return {h, w};
}

static void normalize_dims(std::vector<std::string>& lines, int target_rows,
                           int target_cols) {
    // Crop/pad number of rows.
    if ((int) lines.size() > target_rows) {
        lines.resize((size_t) target_rows);
    } else if ((int) lines.size() < target_rows) {
        lines.resize((size_t) target_rows, std::string{});
    }

    // Crop/pad each row to target_cols, padding with floor.
    for (auto& l : lines) {
        if ((int) l.size() > target_cols) {
            l.resize((size_t) target_cols);
        } else if ((int) l.size() < target_cols) {
            l.append((size_t) (target_cols - (int) l.size()),
                     generation::EMPTY);
        }
    }
}

static void ensure_single_origin(std::vector<std::string>& lines) {
    auto [h, w] = dims(lines);
    if (h <= 0 || w <= 0) return;

    // If multiple origins, keep the first and clear the rest.
    bool found = false;
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            if (lines[i][j] != generation::ORIGIN) continue;
            if (!found) {
                found = true;
            } else {
                lines[i][j] = generation::EMPTY;
            }
        }
    }

    if (found) return;

    // Place at center-ish if none exists, preferring a floor tile.
    int ci = h / 2;
    int cj = w / 2;
    if (get(lines, ci, cj) == generation::WALL ||
        get(lines, ci, cj) == generation::WALL2) {
        // Find any floor tile.
        for (int i = 0; i < h; i++) {
            for (int j = 0; j < w; j++) {
                if (get(lines, i, j) == generation::EMPTY) {
                    set(lines, i, j, generation::ORIGIN);
                    return;
                }
            }
        }
        // Worst-case: place anyway.
    }
    set(lines, ci, cj, generation::ORIGIN);
}

static std::vector<Point> bfs_path_4_neighbor(
    const std::vector<std::string>& lines, Point start, Point goal) {
    auto [h, w] = dims(lines);
    if (h <= 0 || w <= 0) return {};
    if (!in_bounds(lines, start.i, start.j)) return {};
    if (!in_bounds(lines, goal.i, goal.j)) return {};

    std::vector<std::vector<bool>> visited(
        (size_t) h, std::vector<bool>((size_t) w, false));
    std::vector<std::vector<Point>> parent(
        (size_t) h, std::vector<Point>((size_t) w, Point{-1, -1}));

    std::vector<Point> q;
    q.reserve((size_t) (h * w));
    q.push_back(start);
    visited[(size_t) start.i][(size_t) start.j] = true;

    const int di[4] = {-1, 1, 0, 0};
    const int dj[4] = {0, 0, -1, 1};

    for (size_t qi = 0; qi < q.size(); qi++) {
        Point cur = q[qi];
        if (cur.i == goal.i && cur.j == goal.j) break;

        for (int k = 0; k < 4; k++) {
            int ni = cur.i + di[k];
            int nj = cur.j + dj[k];
            if (ni < 0 || nj < 0 || ni >= h || nj >= w) continue;
            if (visited[(size_t) ni][(size_t) nj]) continue;

            char ch = get(lines, ni, nj);
            if (!is_walkable_for_routing(ch)) continue;

            visited[(size_t) ni][(size_t) nj] = true;
            parent[(size_t) ni][(size_t) nj] = cur;
            q.push_back(Point{ni, nj});
        }
    }

    if (!visited[(size_t) goal.i][(size_t) goal.j]) return {};

    // Reconstruct path.
    std::vector<Point> path;
    Point cur = goal;
    while (!(cur.i == start.i && cur.j == start.j)) {
        path.push_back(cur);
        Point p = parent[(size_t) cur.i][(size_t) cur.j];
        if (p.i == -1 && p.j == -1) break;
        cur = p;
    }
    path.push_back(start);
    std::reverse(path.begin(), path.end());
    return path;
}

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

std::vector<std::string> stage_layout_simple(const std::string& seed,
                                             const GenerationContext& ctx,
                                             BarArchetype archetype,
                                             int attempt_index) {
    int num_seeds = num_seeds_for_archetype(archetype, ctx, seed);
    // Deterministic per attempt.
    RandomEngine::set_seed(fmt::format("{}:layout:{}",
                                       seed,
                                       attempt_index));
    std::vector<char> grid = something(ctx.cols, ctx.rows, num_seeds, false);
    return grid_to_lines(grid, ctx.rows, ctx.cols);
}

static std::vector<std::string> wfc_lines_no_log(const wfc::WaveCollapse& wc) {
    bool add_extra_space = false;
    std::vector<std::string> lines;
    std::stringstream temp;

    // NOTE: WaveCollapse indexes grid_options as [x * rows + y] throughout.
    // The current config uses square grids (rows == cols), so keep this
    // convention here for consistency.
    for (int r = 0; r < wc.rows; r++) {
        for (size_t i = 0; i < wc.patterns[0].pat.size(); i++) {
            for (int c = 0; c < wc.cols; c++) {
                int idx = r * wc.rows + c;
                int bit = bitset_utils::get_first_enabled_bit(
                    wc.grid_options[(size_t) idx]);
                if (bit == -1) {
                    temp << std::string(wc.patterns[0].pat.size(), '.');
                } else if (wc.grid_options[(size_t) idx].count() > 1) {
                    temp << std::string(wc.patterns[0].pat.size(), '?');
                } else {
                    temp << (wc.patterns[(size_t) bit].pat)[i];
                }
                if (add_extra_space) temp << " ";
            }
            lines.push_back(std::string(temp.str()));
            temp.str(std::string());
        }
        if (add_extra_space) lines.push_back(" ");
    }
    return lines;
}

static void scrub_to_layout_only(std::vector<std::string>& lines) {
    for (auto& line : lines) {
        for (char& ch : line) {
            if (ch == generation::WALL || ch == generation::WALL2 ||
                ch == generation::EMPTY || ch == generation::ORIGIN) {
                continue;
            }
            if (ch == '?' || std::isspace((unsigned char) ch)) {
                ch = generation::EMPTY;
                continue;
            }
            // Any non-structural token becomes floor for layout-only output.
            ch = generation::EMPTY;
        }
    }
}

std::vector<std::string> stage_layout_wfc(const std::string& seed,
                                          const GenerationContext& ctx,
                                          int attempt_index) {
    wfc::ensure_map_generation_info_loaded();

    unsigned int wfc_seed = static_cast<unsigned int>(
        hashString(fmt::format("{}:wfc_layout:{}", seed, attempt_index)));
    wfc::WaveCollapse wc(wfc_seed);
    wc.run();

    std::vector<std::string> lines = wfc_lines_no_log(wc);
    normalize_dims(lines, ctx.rows, ctx.cols);
    scrub_to_layout_only(lines);
    return lines;
}

static bool place_one(std::vector<std::string>& lines, char sym,
                      std::mt19937& rng,
                      const std::vector<std::vector<bool>>& blocked) {
    auto [h, w] = dims(lines);
    std::vector<Point> candidates;
    candidates.reserve((size_t) (h * w));
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            if (!in_bounds(lines, i, j)) continue;
            if (blocked[(size_t) i][(size_t) j]) continue;
            if (get(lines, i, j) != generation::EMPTY) continue;
            candidates.push_back(Point{i, j});
        }
    }
    std::shuffle(candidates.begin(), candidates.end(), rng);
    if (candidates.empty()) return false;
    set(lines, candidates[0].i, candidates[0].j, sym);
    return true;
}

static bool place_required_day1(std::vector<std::string>& lines,
                               std::mt19937& rng) {
    auto [h, w] = dims(lines);
    if (h <= 0 || w <= 0) return false;

    // Clear any pre-existing required symbols (keep structural + origin).
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            char ch = get(lines, i, j);
            if (ch == generation::WALL || ch == generation::WALL2 ||
                ch == generation::EMPTY || ch == generation::ORIGIN) {
                continue;
            }
            set(lines, i, j, generation::EMPTY);
        }
    }

    ensure_single_origin(lines);

    // Pick a register location where queue strip (default FORWARD) is clear.
    std::vector<Point> reg_candidates;
    for (int i = 1; i < h - 1; i++) {
        for (int j = 1; j < w - 4; j++) {
            if (get(lines, i, j) != generation::EMPTY) continue;
            if (get(lines, i, j + 1) != generation::EMPTY) continue;
            if (get(lines, i, j + 2) != generation::EMPTY) continue;
            if (get(lines, i, j + 3) != generation::EMPTY) continue;
            reg_candidates.push_back(Point{i, j});
        }
    }
    std::shuffle(reg_candidates.begin(), reg_candidates.end(), rng);
    if (reg_candidates.empty()) return false;

    // Try a few register candidates until we can place a spawner with a path to
    // the queue-front tile.
    for (size_t reg_try = 0; reg_try < std::min<size_t>(reg_candidates.size(), 50);
         reg_try++) {
        Point reg = reg_candidates[reg_try];
        Point queue_front{reg.i, reg.j + 1};

        // Try spawner placements.
        std::vector<Point> spawner_candidates;
        spawner_candidates.reserve((size_t) (h * w));
        for (int i = 1; i < h - 1; i++) {
            for (int j = 1; j < w - 1; j++) {
                if (get(lines, i, j) != generation::EMPTY) continue;
                // Avoid occupying the queue strip.
                if (i == reg.i && (j == reg.j || j == reg.j + 1 ||
                                   j == reg.j + 2 || j == reg.j + 3)) {
                    continue;
                }
                spawner_candidates.push_back(Point{i, j});
            }
        }
        std::shuffle(spawner_candidates.begin(), spawner_candidates.end(), rng);
        if (spawner_candidates.empty()) continue;

        for (size_t sp_try = 0;
             sp_try < std::min<size_t>(spawner_candidates.size(), 150);
             sp_try++) {
            // Work on a scratch copy while we test pathability.
            std::vector<std::string> scratch = lines;
            set(scratch, reg.i, reg.j, generation::REGISTER);
            Point sp = spawner_candidates[sp_try];
            set(scratch, sp.i, sp.j, generation::CUST_SPAWNER);

            auto path = bfs_path_4_neighbor(scratch, sp, queue_front);
            if (path.empty()) continue;

            // Commit base required placement.
            lines = std::move(scratch);

            // Protect queue strip + found path.
            std::vector<std::vector<bool>> blocked(
                (size_t) h, std::vector<bool>((size_t) w, false));
            for (int dj = 1; dj <= 3; dj++) {
                blocked[(size_t) reg.i][(size_t) (reg.j + dj)] = true;
            }
            for (const Point& p : path) {
                blocked[(size_t) p.i][(size_t) p.j] = true;
            }

            // Place remaining required entities away from the critical path.
            // Note: treat them as solids for routing, so keep them off the path.
            if (!place_one(lines, generation::SODA_MACHINE, rng, blocked))
                return false;
            if (!place_one(lines, generation::CUPBOARD, rng, blocked))
                return false;
            if (!place_one(lines, generation::TRASH, rng, blocked)) return false;
            if (!place_one(lines, generation::FAST_FORWARD, rng, blocked))
                return false;
            if (!place_one(lines, generation::SOPHIE, rng, blocked)) return false;
            if (!place_one(lines, generation::TABLE, rng, blocked)) return false;

            return true;
        }
    }

    return false;
}

std::vector<std::string> stage_decoration(std::vector<std::string> lines) {
    return lines;
}

static bool validate_day1_ascii_plus_routing(
    const std::vector<std::string>& lines) {
    auto report = mapgen::playability::validate_ascii_day1(lines);
    if (!report.ok()) return false;

    // Additional runtime-aligned checks:
    // - Register queue tiles (default facing FORWARD) must be floor
    // - Spawner can path to register queue-front under ASCII routing rules
    auto [h, w] = dims(lines);
    std::optional<Point> spawner;
    std::optional<Point> reg;
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            if (get(lines, i, j) == generation::CUST_SPAWNER) spawner = Point{i, j};
            if (get(lines, i, j) == generation::REGISTER) reg = Point{i, j};
        }
    }
    if (!spawner.has_value() || !reg.has_value()) return false;

    // Ensure queue strip is clear (tiles ahead are floor).
    for (int dj = 1; dj <= HasWaitingQueue::max_queue_size; dj++) {
        if (!in_bounds(lines, reg->i, reg->j + dj)) return false;
        if (get(lines, reg->i, reg->j + dj) != generation::EMPTY) return false;
    }

    Point goal{reg->i, reg->j + 1};

    // Build a routing view: only '.' and '0' are traversable.
    std::vector<std::string> routing = lines;
    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            char ch = routing[i][j];
            if (is_walkable_for_routing(ch)) continue;
            // Allow BFS to start on spawner tile even though it's non-walkable.
            if (i == spawner->i && j == spawner->j) continue;
            routing[i][j] = generation::WALL;
        }
    }

    auto path = bfs_path_4_neighbor(routing, *spawner, goal);
    return !path.empty();
}

}  // namespace

GeneratedAscii generate_ascii(const std::string& seed,
                             const GenerationContext& context) {
    // Optional seed prefixes for selecting the layout provider without adding
    // new settings plumbing:
    // - "wfc:" forces WFC layout
    // - "simple:" forces simple layout
    LayoutSource layout_source = context.layout_source;
    std::string normalized_seed = seed;
    {
        std::string_view s = seed;
        if (lstrip_prefix(s, "wfc:") != s) {
            layout_source = LayoutSource::Wfc;
            normalized_seed = std::string(lstrip_prefix(s, "wfc:"));
        } else if (lstrip_prefix(s, "simple:") != s) {
            layout_source = LayoutSource::Simple;
            normalized_seed = std::string(lstrip_prefix(s, "simple:"));
        }
    }
    if (normalized_seed.empty()) normalized_seed = "seed";

    BarArchetype archetype = pick_archetype_from_seed(normalized_seed);

    const auto attempt_generate_with_layout = [&](LayoutSource src)
        -> std::optional<std::vector<std::string>> {
        for (int attempt = 0; attempt < playability::DEFAULT_REROLL_ATTEMPTS;
             attempt++) {
            std::vector<std::string> lines;
            if (src == LayoutSource::Wfc) {
                lines = stage_layout_wfc(normalized_seed, context, attempt);
            } else {
                lines = stage_layout_simple(normalized_seed, context, archetype,
                                            attempt);
            }

            normalize_dims(lines, context.rows, context.cols);

            std::mt19937 rng(static_cast<unsigned int>(hashString(fmt::format(
                "{}:place:{}",
                normalized_seed,
                attempt))));

            if (!place_required_day1(lines, rng)) continue;
            lines = stage_decoration(std::move(lines));
            if (!validate_day1_ascii_plus_routing(lines)) continue;
            return lines;
        }
        return std::nullopt;
    };

    std::optional<std::vector<std::string>> maybe_lines =
        attempt_generate_with_layout(layout_source);

    // Fallback: if WFC fails too often, try simple layout so gameplay always
    // gets a valid start-of-day map.
    if (!maybe_lines.has_value() && layout_source == LayoutSource::Wfc) {
        log_warn("[mapgen] WFC failed all attempts for seed {}, falling back to simple layout",
                 normalized_seed);
        maybe_lines = attempt_generate_with_layout(LayoutSource::Simple);
    }

    // Last-ditch: generate something and hope runtime repair/manual reroll is used.
    std::vector<std::string> lines;
    if (maybe_lines.has_value()) {
        lines = std::move(*maybe_lines);
    } else {
        lines = stage_layout_simple(normalized_seed, context, archetype, 0);
        normalize_dims(lines, context.rows, context.cols);
        ensure_single_origin(lines);
    }

    GeneratedAscii out;
    out.lines = std::move(lines);
    out.archetype = archetype;
    return out;
}

}  // namespace mapgen

