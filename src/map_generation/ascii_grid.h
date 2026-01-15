#pragma once

#include <string>
#include <utility>
#include <vector>

namespace mapgen::grid {

struct Cell {
    int i = 0;
    int j = 0;
};

[[nodiscard]] std::pair<int, int> dims(const std::vector<std::string>& lines);
[[nodiscard]] bool in_bounds(const std::vector<std::string>& lines, int i,
                             int j);
[[nodiscard]] char get(const std::vector<std::string>& lines, int i, int j);
void set(std::vector<std::string>& lines, int i, int j, char ch);

void normalize_dims(std::vector<std::string>& lines, int target_rows,
                    int target_cols);

void ensure_single_origin(std::vector<std::string>& lines);

[[nodiscard]] bool is_walkable_for_routing(char ch);

[[nodiscard]] std::vector<Cell> bfs_path_4_neighbor(
    const std::vector<std::string>& lines, Cell start, Cell goal);

// Converts any non-structural tokens into walkable floor, so that a generator
// can provide "layout only" and let the shared placement stage handle required
// entities.
void scrub_to_layout_only(std::vector<std::string>& lines);

// Removes 1x1 enclosed walkable areas by opening walls to merge them with
// adjacent spaces. Call this after entity placement to fix rooms that became
// enclosed due to furniture placement.
void remove_1x1_rooms(std::vector<std::string>& lines);

}  // namespace mapgen::grid
