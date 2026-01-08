#include "ascii_grid.h"

#include <algorithm>
#include <cctype>
#include <queue>

#include "map_generation.h"

namespace mapgen::grid {

std::pair<int, int> dims(const std::vector<std::string>& lines) {
    int h = (int) lines.size();
    int w = 0;
    for (const auto& l : lines) w = std::max(w, (int) l.size());
    return {h, w};
}

bool in_bounds(const std::vector<std::string>& lines, int i, int j) {
    if (i < 0 || j < 0) return false;
    if (i >= (int) lines.size()) return false;
    if (j >= (int) lines[i].size()) return false;
    return true;
}

char get(const std::vector<std::string>& lines, int i, int j) {
    if (!in_bounds(lines, i, j)) return generation::WALL;
    return lines[i][j];
}

void set(std::vector<std::string>& lines, int i, int j, char ch) {
    if (!in_bounds(lines, i, j)) return;
    lines[i][j] = ch;
}

void normalize_dims(std::vector<std::string>& lines, int target_rows,
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

void ensure_single_origin(std::vector<std::string>& lines) {
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

bool is_walkable_for_routing(char ch) {
    // Treat only floor/origin as walkable for pathing.
    return ch == generation::EMPTY || ch == generation::ORIGIN;
}

std::vector<Cell> bfs_path_4_neighbor(const std::vector<std::string>& lines,
                                      Cell start, Cell goal) {
    auto [h, w] = dims(lines);
    if (h <= 0 || w <= 0) return {};
    if (!in_bounds(lines, start.i, start.j)) return {};
    if (!in_bounds(lines, goal.i, goal.j)) return {};

    std::vector<std::vector<bool>> visited(
        (size_t) h, std::vector<bool>((size_t) w, false));
    std::vector<std::vector<Cell>> parent(
        (size_t) h, std::vector<Cell>((size_t) w, Cell{-1, -1}));

    std::queue<Cell> q;
    q.push(start);
    visited[(size_t) start.i][(size_t) start.j] = true;

    const int di[4] = {-1, 1, 0, 0};
    const int dj[4] = {0, 0, -1, 1};

    while (!q.empty()) {
        Cell cur = q.front();
        q.pop();
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
            q.push(Cell{ni, nj});
        }
    }

    if (!visited[(size_t) goal.i][(size_t) goal.j]) return {};

    // Reconstruct path.
    std::vector<Cell> path;
    Cell cur = goal;
    while (!(cur.i == start.i && cur.j == start.j)) {
        path.push_back(cur);
        Cell p = parent[(size_t) cur.i][(size_t) cur.j];
        if (p.i == -1 && p.j == -1) break;
        cur = p;
    }
    path.push_back(start);
    std::reverse(path.begin(), path.end());
    return path;
}

void scrub_to_layout_only(std::vector<std::string>& lines) {
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

namespace {

// Count walkable neighbors (only EMPTY and ORIGIN are walkable for this check)
int count_walkable_neighbors(const std::vector<std::string>& lines, int i,
                             int j) {
    int count = 0;
    // Up
    if (is_walkable_for_routing(get(lines, i - 1, j))) count++;
    // Down
    if (is_walkable_for_routing(get(lines, i + 1, j))) count++;
    // Left
    if (is_walkable_for_routing(get(lines, i, j - 1))) count++;
    // Right
    if (is_walkable_for_routing(get(lines, i, j + 1))) count++;
    return count;
}

// Check if a character is a wall that can be opened
bool is_openable_wall(char ch) {
    return ch == generation::WALL || ch == generation::WALL2;
}

}  // namespace

void remove_1x1_rooms(std::vector<std::string>& lines) {
    auto [h, w] = dims(lines);
    if (h <= 2 || w <= 2) return;

    bool changed = true;
    int iterations = 0;
    const int max_iterations = 100;  // Safety limit

    while (changed && iterations < max_iterations) {
        changed = false;
        iterations++;

        // Remove isolated 1x1 rooms (walkable tiles with 0 walkable neighbors)
        for (int i = 1; i < h - 1; i++) {
            for (int j = 1; j < w - 1; j++) {
                char ch = get(lines, i, j);
                if (!is_walkable_for_routing(ch)) continue;

                int neighbors = count_walkable_neighbors(lines, i, j);
                if (neighbors == 0) {
                    // This is a 1x1 room - try to open a wall to connect it
                    if (i + 1 < h - 1 && is_openable_wall(get(lines, i + 1, j))) {
                        set(lines, i + 1, j, generation::EMPTY);
                        changed = true;
                        continue;
                    }
                    if (j + 1 < w - 1 && is_openable_wall(get(lines, i, j + 1))) {
                        set(lines, i, j + 1, generation::EMPTY);
                        changed = true;
                        continue;
                    }
                    if (i > 1 && is_openable_wall(get(lines, i - 1, j))) {
                        set(lines, i - 1, j, generation::EMPTY);
                        changed = true;
                        continue;
                    }
                    if (j > 1 && is_openable_wall(get(lines, i, j - 1))) {
                        set(lines, i, j - 1, generation::EMPTY);
                        changed = true;
                        continue;
                    }
                }
            }
        }
    }
}

}  // namespace mapgen::grid
