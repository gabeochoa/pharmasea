
#pragma once

#include <sstream>
#include <string>
#include <vector>

#include "engine/bitset_utils.h"
#include "engine/pathfinder.h"
#include "entity_helper.h"

namespace wfc {

constexpr int MAX_NUM_PATTERNS = 64;
using Possibilities = std::bitset<MAX_NUM_PATTERNS>;

using Location = std::pair<int, int>;
using Locations = std::vector<Location>;

enum Rose { N, E, W, S };

using Connections = std::set<Rose>;
struct Pattern {
    int id = -1;
    std::vector<std::string> pat;
    Connections connections;
    bool required = false;
    bool any_connection = false;
    int max_count = -1;
    bool edge_only = false;
};
using Patterns = std::vector<Pattern>;

struct MapGenerationInformation {
    Patterns patterns;
    int rows = 0;
    int cols = 0;
};
extern MapGenerationInformation MAP_GEN_INFO;

struct WaveCollapse {
    std::set<int> collapsed;

    Patterns patterns;

    int rows;
    int cols;

    // represents for each cell what options are still possible
    std::vector<Possibilities> grid_options;

    // We mark this mutable since its not really what we _mean_ by const
    mutable std::vector<int> numbers;
    mutable std::mt19937 gen;

    WaveCollapse(int r, int c, unsigned int seed) : rows(r), cols(c) {
        numbers.resize(rows * cols);
        for (int i = 0; i < rows * cols; ++i) numbers[i] = i;

        patterns = Patterns();
        patterns.reserve(MAP_GEN_INFO.patterns.size());
        for (const Pattern& pattern : MAP_GEN_INFO.patterns) {
            // copy the patterns into our object because we
            // edit them as we generate
            patterns.push_back(Pattern(pattern));
        }

        // We manually only set the num patterns we have
        // because its likely smaller than MAX_NUM_PATTERNS
        Possibilities default_val;
        for (size_t i = 0; i < num_patterns(); i++) {
            default_val.set(i, true);
        }

        grid_options = std::vector<Possibilities>(
            static_cast<long>(rows * cols), default_val);
        gen = std::mt19937(seed);

        // Random stuff should happen under here
        //

        _shuffle_visit_order();
    }

    explicit WaveCollapse(unsigned int seed)
        : WaveCollapse(MAP_GEN_INFO.rows, MAP_GEN_INFO.cols, seed) {}

    std::vector<std::string> get_lines();
    void _dump() const;

    void run();

   private:
    bool _in_grid(int x, int y) const;
    bool _is_edge(int x, int y) const;
    bool _can_be_trivially_collapsed(int x, int y) const;
    bool _is_collapsed(int index) const { return collapsed.contains(index); }

    size_t num_options(size_t index) const;
    Location _find_least_choices() const;
    void _collapse(int x, int y);

    void for_each_cell(std::function<void(int, int)> cb) {
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                if (collapsed.contains(i * rows + j)) continue;
                cb(i, j);
            }
        }
    }
    void for_each_non_collapsed_cell(std::function<void(int, int)> cb) {
        for_each_cell([&](int i, int j) {
            if (collapsed.contains(i * rows + j)) return;
            cb(i, j);
        });
    }

    void for_each_cell(std::function<void(int, int)> cb) const {
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                if (collapsed.contains(i * rows + j)) continue;
                cb(i, j);
            }
        }
    }

    void for_each_non_collapsed_cell(std::function<void(int, int)> cb) const {
        for_each_cell([&](int i, int j) {
            if (collapsed.contains(i * rows + j)) return;
            cb(i, j);
        });
    }

    bool _are_patterns_compatible(const Pattern& a, const Pattern& b,
                                  const Rose& AtoB) const;
    // if either dont care, then we good

    size_t pat_size() const;
    Rose _get_opposite_connection(Rose r) const;
    bool _eligible_pattern(int x, int y, int pattern_id) const;
    void _place_pattern(int x, int y, int pattern_id);
    bool _has_non_collapsed() const;
    size_t num_patterns() const;
    void _validate_patterns() const;
    void _place_required();
    void _place_walls();

    void _shuffle_visit_order() const {
        std::shuffle(numbers.begin(), numbers.end(), gen);
    }

    std::vector<Rose> _get_edges(int x, int y);

    void _initial_cleanup();
};

}  // namespace wfc
