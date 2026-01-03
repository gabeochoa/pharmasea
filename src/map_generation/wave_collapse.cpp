#include "wave_collapse.h"

#include <fmt/core.h>

#include <fstream>
#include <iostream>
#include <optional>
#include <utility>

#include "../ah.h"
#include "magic_enum/magic_enum.hpp"
#include "nlohmann/json.hpp"

namespace wfc {
template<typename... Args>
static void wfc_log(const char* format, Args&&... args) {
    std::string message =
        fmt::format(fmt::runtime(format), std::forward<Args>(args)...);
    std::cout << message << '\n';
    std::cout.flush();
}

void ensure_map_generation_info_loaded() {
    if (MAP_GEN_INFO.rows > 0 && MAP_GEN_INFO.cols > 0 &&
        !MAP_GEN_INFO.patterns.empty()) {
        return;
    }

    const std::string path = "resources/config/map_generator_input.json";
    std::ifstream ifs(path);
    if (!ifs.good()) {
        wfc_log("Failed to open {}", path);
        return;
    }

    nlohmann::json contents;
    try {
        contents = nlohmann::json::parse(ifs, nullptr, true, true);
    } catch (const std::exception& e) {
        wfc_log("Failed to parse {}: {}", path, e.what());
        return;
    }

    MAP_GEN_INFO.rows = contents.value("max_rows", 3);
    MAP_GEN_INFO.cols = contents.value("max_cols", 3);

    if (!contents.contains("patterns")) {
        wfc_log("map_generator_input.json missing patterns");
        return;
    }

    const nlohmann::json jpatterns = contents["patterns"];
    MAP_GEN_INFO.patterns.clear();
    MAP_GEN_INFO.patterns.reserve(jpatterns.size());

    int id = 0;
    for (const nlohmann::json& jpat : jpatterns) {
        Connections connections;
        for (const nlohmann::json& c : jpat["connections"]) {
            const std::optional<Rose> maybe =
                magic_enum::enum_cast<Rose>(c.get<std::string>());
            if (maybe.has_value()) {
                connections.insert(maybe.value());
            }
        }

        Pattern pattern;
        pattern.id = id;
        pattern.pat = jpat["pat"].get<std::vector<std::string>>();
        pattern.connections = std::move(connections);
        pattern.required = jpat.value("required", false);
        pattern.any_connection = jpat.value("any_connection", false);
        pattern.max_count = jpat.value("max_count", -1);
        pattern.edge_only = jpat.value("edge_only", false);

        MAP_GEN_INFO.patterns.push_back(pattern);
        id++;
    }
}

size_t WaveCollapse::pat_size() const { return patterns[0].pat.size(); }

std::vector<std::string> WaveCollapse::get_lines() {
    bool add_extra_space = false;

    std::vector<std::string> lines;
    std::stringstream temp;

    for (int r = 0; r < rows; r++) {
        for (size_t i = 0; i < pat_size(); i++) {
            for (int c = 0; c < cols; c++) {
                int bit = bitset_utils::get_first_enabled_bit(
                    grid_options[r * rows + c]);
                if (bit == -1) {
                    temp << std::string(pat_size(), '.');
                } else if (grid_options[r * rows + c].count() > 1) {
                    temp << std::string(pat_size(), '?');
                } else {
                    temp << (patterns[bit].pat)[i];
                }
                if (add_extra_space) temp << " ";
                // ;
            }
            lines.push_back(std::string(temp.str()));
            temp.str(std::string());
        }
        if (add_extra_space) lines.push_back(" ");
    }

    for (auto line : lines) {
        wfc_log("{}", line);
    }

    return lines;
}

void WaveCollapse::_dump() const {
    std::vector<std::string> lines;
    std::stringstream temp;

    for (int i = 0; i < rows * cols; i++) {
        if (i != 0 && i % (rows) == 0) {
            lines.push_back(std::string(temp.str()));
            temp.str(std::string());
        }

        if (grid_options[i].count() >= 2) {
            temp << (grid_options[i].count());
        }
        if (grid_options[i].count() == 1) {
            int bit = bitset_utils::get_first_enabled_bit(grid_options[i]);
            temp << (char) (bit + 'A');
        }
        if (grid_options[i].count() == 0) {
            temp << "_";
        }
        temp << " ";
    }
    lines.push_back(std::string(temp.str()));
    temp.str(std::string());
    lines.push_back("");

    for (auto line : lines) {
        wfc_log("{}", line);
    }
}

bool WaveCollapse::_eligible_pattern(int x, int y, int pattern_id) const {
    const Possibilities& possibilities = grid_options[x * rows + y];
    const Pattern& pattern = patterns[pattern_id];

    // Pattern is edge only and we arent
    if (pattern.edge_only && !_is_edge(x, y)) {
        return false;
    }

    // Already used up our allocation
    if (pattern.max_count == 0) {
        return false;
    }

    // Cannot be placed here (probably due to propagation
    if (!possibilities.test(pattern_id)) {
        return false;
    }

    //
    return true;
}

void WaveCollapse::_place_pattern(int x, int y, int pattern_id) {
    const auto _print_pattern_and_name = [&]() {
        wfc_log("Selected Pattern {}({})", pattern_id,
                (char) (pattern_id + 'A'));

        for (size_t i = 0; i < patterns[pattern_id].pat.size(); i++) {
            for (size_t j = 0; j < patterns[pattern_id].pat[i].size(); j++) {
                std::cout << patterns[pattern_id].pat[i][j];
            }
            std::cout << std::endl;
        }
    };

    Possibilities& possibilities = grid_options[x * rows + y];

    // things should be valided by now but for now lets double validate
    bool fits = _eligible_pattern(x, y, pattern_id);
    if (!fits) {
        possibilities.set(pattern_id, false);
        log_warn("ran validation on place for {} {} {} but it failed", x, y,
                 pattern_id);
    }

    if (collapsed.contains(x * rows + y)) {
        log_warn("we already collapsed this before {} {}", x, y);
    }
    collapsed.insert(x * rows + y);

    // collapse pattern
    possibilities.reset();
    possibilities.set(pattern_id, true);

    // use up the max count
    patterns[pattern_id].max_count--;

    _print_pattern_and_name();

    // Go through and clean up any max counts
    for_each_non_collapsed_cell([&](int i, int j) {
        Possibilities& possibilities = grid_options[i * rows + j];

        for (size_t bit = 0; bit < num_patterns(); bit++) {
            const Pattern& pattern = patterns[bit];
            // inf uses
            if (pattern.max_count == -1) continue;
            // still has some uses
            if (pattern.max_count != 0) continue;
            if (possibilities.test(bit)) {
                possibilities.set(bit, false);
            }
        }
    });
}

void WaveCollapse::_validate_patterns() const {
    // TODO have a fallback map when generation dies
    size_t pat_size = 0;

    if (patterns.size() > MAX_NUM_PATTERNS) {
        log_warn("You have {} patterns which is more than we support ({})",
                 patterns.size(), MAX_NUM_PATTERNS);
    }

    for (const auto& pattern : patterns) {
        // Handle pattern size warnings
        {
            if (pat_size == 0) pat_size = pattern.pat.size();
            if (pat_size != pattern.pat.size()) {
                log_warn("pattern {} is {} srows but hould probably be {}",
                         pattern.id, pattern.pat.size(), pat_size);
            }
            if (pat_size != pattern.pat[0].size()) {
                log_warn("pattern {} is {} cols but should probably be {}",
                         pattern.id, pattern.pat[0].size(), pat_size);
            }
        }

        // Make sure required ones have a max count
        if (pattern.required) {
            if (pattern.max_count == -1) {
                log_warn(
                    "You have a required pattern {} with no max count, "
                    "consider setting "
                    "to 1",
                    pattern.id);
            }
        }
    }
}

void WaveCollapse::_place_walls() {
    for_each_non_collapsed_cell([&](int i, int j) {
        if (!_is_edge(i, j)) return;

        for (Pattern& pattern : patterns) {
            if (!pattern.edge_only) continue;
            if (_eligible_pattern(i, j, pattern.id)) {
                _place_pattern(i, j, pattern.id);
            }
        }
    });
}

void WaveCollapse::_place_required() {
    const auto _place_in_map = [&](Pattern& pattern) {
        bool placed = false;
        for (int index : numbers) {
            int i = index / rows;
            int j = index % rows;

            //
            if (pattern.edge_only && !_is_edge(i, j)) continue;

            // == 0 since -1 means inf
            if (pattern.max_count == 0) return;

            Possibilities& possibilities = grid_options[i * rows + j];

            // can we hold this pattern at the moment?
            if (possibilities.test(pattern.id)) {
                _place_pattern(i, j, pattern.id);
                placed = true;
                return;
            }
        }
        // we already visited this one so shuffle it
        _shuffle_visit_order();
        if (!placed)
            log_warn("we did not place required pattern {}({})", pattern.id,
                     (char) (pattern.id + 'A'));
    };

    for (Pattern& pattern : patterns) {
        if (pattern.required) {
            _place_in_map(pattern);
        }
    }
}

void WaveCollapse::run() {
    const auto _dump_and_print = [&](const std::string& msg) {
        _dump();
        wfc_log("{}", msg);
    };

    wfc_log("starting generation with {} rows and {} cols", rows, cols);

    // double check if we have any common errors with our patterns
    _validate_patterns();

    // Cleaning up starting possibilities that are invalid
    _initial_cleanup();
    _dump_and_print("first pass");

    // place anything thats required before we start the generation
    _place_required();
    _dump_and_print("initial phase complete");

    // place walls after required since there might be required walls
    _place_walls();

    _dump_and_print("completed initial manual placements");

    get_lines();
    wfc_log("{}", "starting wfc");

    // If we filled everything then we are done
    while (_has_non_collapsed()) {
        // shuffle the visit order randomly so we dont always just go in order
        _shuffle_visit_order();

        // find the one that has the least number of options
        auto [x, y] = _find_least_choices();
        _collapse(x, y);
    }

    wfc_log("{}", "completed wfc");
}

Rose WaveCollapse::_get_opposite_connection(Rose r) const {
    switch (r) {
        case N:
            return S;
        case S:
            return N;
        case E:
            return W;
        case W:
            return E;
    }
    log_error("How did we get a Rose that wasnt handled in the switch");
    return N;
}

bool WaveCollapse::_in_grid(int x, int y) const {
    return x >= 0 && x < rows && y >= 0 && y < cols;
}

bool WaveCollapse::_has_non_collapsed() const {
    return (int) (collapsed.size()) != (rows * cols);
}

size_t WaveCollapse::num_patterns() const { return patterns.size(); }

size_t WaveCollapse::num_options(size_t index) const {
    return grid_options[index].count();
}

Location WaveCollapse::_find_least_choices() const {
    Location loc{-1, -1};
    size_t c_max = 99;
    for (int i : numbers) {
        // skip if already collapsed
        if (_is_collapsed(i)) continue;

        // if you have less options than max,
        // best option for now
        if (num_options(i) < c_max) {
            c_max = num_options(i);
            loc = {i / rows, i % rows};
        }
    }
    return loc;
}

bool WaveCollapse::_can_be_trivially_collapsed(int x, int y) const {
    return num_options(x * rows + y) == 1;
}

void WaveCollapse::_collapse(int x, int y) {
    Possibilities& possibilities = grid_options[(x * rows) + y];

    if (_can_be_trivially_collapsed(x, y)) {
        int bit = bitset_utils::get_first_enabled_bit(possibilities);

        Pattern& pattern = patterns[bit];
        if (pattern.max_count == 0) {
            log_warn(
                "we are trivially collapseable to something already at the max "
                "count");
            // instead just place pattern 0
            _place_pattern(x, y, 0);
            return;
        }
        _place_pattern(x, y, bit);
        return;
    }

    // More than one choice...
    int bit = bitset_utils::get_random_enabled_bit(possibilities, gen,
                                                   num_patterns());

    // no validation
    _place_pattern(x, y, bit);
    return;
}

bool WaveCollapse::_are_patterns_compatible(const Pattern& a, const Pattern& b,
                                            const Rose& AtoB) const {
    // if either dont care, then we good
    if (a.any_connection || b.any_connection) return true;

    bool does_a_connect_in_this_direction = false;
    for (const auto c : a.connections) {
        if (c == AtoB) {
            does_a_connect_in_this_direction = true;
            break;
        }
    }

    bool does_b_connect_in_this_direction = false;
    for (const auto c : b.connections) {
        if (c == _get_opposite_connection(AtoB)) {
            does_b_connect_in_this_direction = true;
            break;
        }
    }

    if (!does_a_connect_in_this_direction &&
        !does_b_connect_in_this_direction) {
        // both fail?
        return true;
    }

    if (!does_a_connect_in_this_direction ||
        !does_b_connect_in_this_direction) {
        return false;
    }

    return true;
}

std::vector<Rose> WaveCollapse::_get_edges(int x, int y) {
    std::vector<Rose> edges;
    if (x == 0) edges.push_back(Rose::N);
    if (x == rows - 1) edges.push_back(Rose::S);
    if (y == 0) edges.push_back(Rose::W);
    if (y == cols - 1) edges.push_back(Rose::E);
    return edges;
}

void WaveCollapse::_initial_cleanup() {
    // NOTE: we use num_patterns() and not possibilities.size()
    // because generally we are going to have less than max patterns

    // since we statically allocate Possibilities to MAX_NUM
    // we have a bunch extra, this just disables those
    const auto _disable_unused_patterns = [&](int i, int j) {
        Possibilities& possibilities = grid_options[i * rows + j];
        // TODO size-1?
        for (size_t bit = num_patterns(); bit < possibilities.size(); bit++) {
            possibilities.set(bit, false);
        }
    };

    //

    const auto _disable_edge_only = [&](int i, int j) {
        // disable edge only patterns on non edges
        Possibilities& possibilities = grid_options[i * rows + j];

        for (size_t bit = 0; bit < num_patterns(); bit++) {
            const Pattern& pattern = patterns[bit];
            if (!pattern.edge_only) continue;
            if (possibilities.test(bit)) {
                possibilities.set(bit, false);
            }
        }
    };

    const auto _remove_banned_edges = [&](int i, int j) {
        // What edge are we on
        auto banned_edges = _get_edges(i, j);

        Possibilities& possibilities = grid_options[i * rows + j];
        // disable all the neighbor's patterns that dont match us
        for (size_t bit = 0; bit < num_patterns(); bit++) {
            // does this location have this pattern enabled?
            if (possibilities.test(bit)) {
                // does it use the banned edge?
                for (auto edge : banned_edges) {
                    if (patterns[bit].connections.contains(edge)) {
                        possibilities.set(bit, false);
                    }
                }
            }
        }
    };

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            _disable_unused_patterns(i, j);
            // not edge
            if (!_is_edge(i, j)) {
                _disable_edge_only(i, j);
                continue;
            }
            _remove_banned_edges(i, j);
        }
    }
}

bool WaveCollapse::_is_edge(int i, int j) const {
    return (i == 0 || j == 0 || j == cols - 1 || i == rows - 1);
}

}  // namespace wfc
