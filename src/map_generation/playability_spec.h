#pragma once

#include <algorithm>
#include <optional>
#include <queue>
#include <string>
#include <string_view>
#include <vector>

#include "../ah.h"
#include "map_generation.h"

// Phase-1 deliverable: a small, testable “playability spec” surface area that
// map generators can validate against.
//
// Canonical human-readable spec: `docs/map_playability_spec.md`
namespace mapgen::playability {

// Phase-1 default (also documented in `docs/map_playability_spec.md`)
constexpr int DEFAULT_REROLL_ATTEMPTS = 25;

enum class FailureClass {
    Repairable,
    RerollOnly,
};

enum class FailureKind {
    MissingOrigin,
    MultipleOrigins,

    MissingRequiredEntity,  // One or more required symbols missing

    DisconnectedWalkable_4Neighbor,  // diagonal-only connections are invalid
};

struct Failure {
    FailureKind kind;
    FailureClass classification;
    std::string message;
};

struct Report {
    std::vector<Failure> failures;

    [[nodiscard]] bool ok() const { return failures.empty(); }
};

namespace detail {

inline bool is_wall(char ch) {
    return ch == generation::WALL || ch == generation::WALL2;
}

inline char get_char(const std::vector<std::string>& lines, int i, int j) {
    if (i < 0 || j < 0) return generation::WALL;
    if (i >= (int) lines.size()) return generation::WALL;
    if (j >= (int) lines[i].size()) return generation::WALL;
    return lines[i][j];
}

inline std::pair<int, int> dims(const std::vector<std::string>& lines) {
    int h = static_cast<int>(lines.size());
    int w = 0;
    for (const auto& l : lines) w = std::max(w, static_cast<int>(l.size()));
    return {h, w};
}

inline bool is_walkable_ascii(char ch) {
    // ASCII-level structural definition: anything not an outside-facing wall is
    // “walkable enough” for structural connectivity checks.
    //
    // Runtime walkability is authoritative (EntityHelper::isWalkable), but that
    // requires instantiating entities.
    return !is_wall(ch);
}

inline bool is_connected_4_neighbor(const std::vector<std::string>& lines) {
    auto [h, w] = dims(lines);
    if (h <= 0 || w <= 0) return true;

    int total_walkable = 0;
    std::optional<std::pair<int, int>> start;

    for (int i = 0; i < h; i++) {
        for (int j = 0; j < w; j++) {
            char ch = get_char(lines, i, j);
            if (!is_walkable_ascii(ch)) continue;
            total_walkable++;
            if (!start.has_value()) start = std::make_pair(i, j);
        }
    }

    if (total_walkable == 0) return true;
    if (!start.has_value()) return true;

    std::vector<std::vector<bool>> visited(h, std::vector<bool>(w, false));

    std::queue<std::pair<int, int>> q;
    q.push(*start);
    visited[start->first][start->second] = true;
    int visited_walkable = 0;

    const int di[4] = {-1, 1, 0, 0};
    const int dj[4] = {0, 0, -1, 1};

    while (!q.empty()) {
        auto [ci, cj] = q.front();
        q.pop();
        visited_walkable++;

        for (int k = 0; k < 4; k++) {
            int ni = ci + di[k];
            int nj = cj + dj[k];
            if (ni < 0 || nj < 0 || ni >= h || nj >= w) continue;
            if (visited[ni][nj]) continue;
            char ch = get_char(lines, ni, nj);
            if (!is_walkable_ascii(ch)) continue;
            visited[ni][nj] = true;
            q.push({ni, nj});
        }
    }

    return visited_walkable == total_walkable;
}

inline int count_char(const std::vector<std::string>& lines, char needle) {
    int count = 0;
    for (const auto& l : lines) {
        for (char ch : l) {
            if (ch == needle) count++;
        }
    }
    return count;
}

}  // namespace detail

// Validates only the **ASCII-level** Phase-1 playability rules:
// - Exactly one origin marker
// - Required symbols exist (Day-1 minimum)
// - 4-neighbor connectivity of non-wall tiles (no diagonal-only connectivity)
//
// Note: runtime checks (register inside BAR_BUILDING, queue strip, spawner→reg
// path under BFS cutoff) are implemented elsewhere today (Sophie +
// generation::helper::validate()) and will be unified in Phase 2/3 refactor.
inline Report validate_ascii_day1(const std::vector<std::string>& lines) {
    Report r;

    const int origin_count = detail::count_char(lines, generation::ORIGIN);
    if (origin_count == 0) {
        r.failures.push_back(Failure{
            .kind = FailureKind::MissingOrigin,
            .classification = FailureClass::RerollOnly,
            .message = "missing origin marker '0'",
        });
    } else if (origin_count > 1) {
        r.failures.push_back(Failure{
            .kind = FailureKind::MultipleOrigins,
            .classification = FailureClass::RerollOnly,
            .message = "multiple origin markers '0' found",
        });
    }

    // Day-1 required symbols (see `docs/map_playability_spec.md`)
    const std::pair<char, std::string_view> required[] = {
        {generation::CUST_SPAWNER, "CustomerSpawner (C)"},
        {generation::REGISTER, "Register (R)"},
        {generation::SODA_MACHINE, "SodaMachine (S)"},
        {generation::CUPBOARD, "Cupboard (d)"},
        {generation::TRASH, "Trash (g)"},
        {generation::FAST_FORWARD, "FastForward (f)"},
        {generation::SOPHIE, "Sophie (+)"},
        {generation::TABLE, "Table (t)"},
    };

    for (const auto& [sym, label] : required) {
        if (detail::count_char(lines, sym) <= 0) {
            r.failures.push_back(Failure{
                .kind = FailureKind::MissingRequiredEntity,
                .classification = FailureClass::RerollOnly,
                .message = fmt::format("missing required symbol: {}", label),
            });
        }
    }

    if (!detail::is_connected_4_neighbor(lines)) {
        r.failures.push_back(Failure{
            .kind = FailureKind::DisconnectedWalkable_4Neighbor,
            .classification = FailureClass::RerollOnly,
            .message =
                "walkable tiles are not connected under 4-neighbor adjacency",
        });
    }

    return r;
}

}  // namespace mapgen::playability
