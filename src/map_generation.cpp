
#include "map_generation.h"

#include "engine/bitset_utils.h"

namespace wfc {
size_t WaveCollapse::pat_size() const { return patterns[0].pat.size(); }

void WaveCollapse::_photo() const {
    std::cout << "PHOTO BOOTH" << std::endl;
    for (int r = 0; r < rows; r++) {
        for (size_t i = 0; i < pat_size(); i++) {
            for (int c = 0; c < cols; c++) {
                int bit = bitset_utils::get_first_enabled_bit(
                    grid_options[r * rows + c]);
                if (bit == -1) {
                    std::cout << std::string(pat_size(), '.');
                } else {
                    std::cout << (patterns[bit].pat)[i];
                }
                std::cout << "";
            }
            std::cout << std::endl;
        }
    }

    std::cout << std::endl;
};

std::vector<std::string> WaveCollapse::get_lines() {
    bool add_extra_space = true;

    std::vector<std::string> lines;
    std::stringstream temp;

    for (int r = 0; r < rows; r++) {
        for (size_t i = 0; i < pat_size(); i++) {
            for (int c = 0; c < cols; c++) {
                int bit = bitset_utils::get_first_enabled_bit(
                    grid_options[r * rows + c]);
                if (bit == -1) {
                    temp << std::string(pat_size(), '.');
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
        std::cout << line << std::endl;
    }

    return lines;
}

void WaveCollapse::_dump() const {
    for (int i = 0; i < rows * cols; i++) {
        if (i != 0 && i % (rows) == 0) {
            std::cout << std::endl;
        }

        if (grid_options[i].count() >= 2) {
            std::cout << (grid_options[i].count());
        }
        if (grid_options[i].count() == 1) {
            int bit = bitset_utils::get_first_enabled_bit(grid_options[i]);
            std::cout << (char) (bit + 'A');
        }
        if (grid_options[i].count() == 0) {
            std::cout << "_";
        }
        std::cout << " ";
    }
    std::cout << std::endl;
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
        log_info("Selected Pattern {}({})", pattern_id,
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
            if (pattern.max_count == -1) continue;
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

void WaveCollapse::_propagate_all() {
    for_each_non_collapsed_cell([&](int i, int j) { _propagate_choice(i, j); });
}

void WaveCollapse::run() {
    const auto _dump_and_print = [&](const std::string& msg) {
        _dump();
        log_info("{}", msg);
    };

    log_info("starting generation with {} rows and {} cols", rows, cols);

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
    _dump_and_print("placed walls");

    // force a propagate so we can get started
    _propagate_all();
    _dump_and_print("completed initial manual placements");

    get_lines();

    do {
        _dump();
        auto [x, y] = _find_lowest_entropy();
        log_info("lowest entropy was {} {}", x, y);
        if (_collapsed(x, y)) break;

        auto pattern = _choose_pattern(x, y);
        log_info("selected pattern {} ", pattern.id);
        _announce_max_count_chosen(pattern.id, x, y);
        _propagate_choice(x, y);
    } while (_has_non_collapsed());

    _dump();
}

int WaveCollapse::_gen_rand(int a, int b) const {
    return a + (gen() % (b - a));
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

Location WaveCollapse::_get_relative_loc(Rose r, int x, int y) const {
    switch (r) {
        case N:
            return {x - 1, y - 0};
        case S:
            return {x + 1, y + 0};
        case E:
            return {x + 0, y + 1};
        case W:
            return {x - 0, y - 1};
    }
    log_error("How did we get a Rose that wasnt handled in the switch");
    return {x, y};
}

bool WaveCollapse::_in_grid(int x, int y) const {
    return x >= 0 && x < rows && y >= 0 && y < cols;
}

bool WaveCollapse::_collapsed(int x, int y) const {
    if (x == -1 || y == -1) return true;
    return collapsed.contains(x * rows + y);
}

bool WaveCollapse::_has_non_collapsed() const {
    return (int) (collapsed.size()) == (rows * cols);
}

size_t WaveCollapse::num_patterns() const { return patterns.size(); }

Location WaveCollapse::_find_lowest_entropy() const {
    if (is_first_one) {
        is_first_one = false;
        return {_gen_rand(0, rows), _gen_rand(0, cols)};
    }

    Location loc{-1, -1};
    size_t c_max = 1;
    for (int i : numbers) {
        // if you are at max, congrats
        if (grid_options[i].count() == num_patterns()) {
            return {i / rows, i % rows};
        }
        if (grid_options[i].count() > c_max) {
            c_max = grid_options[i].count();
            loc = {i / rows, i % rows};
        }
    }
    _shuffle_visit_order();

    return loc;
}

Pattern& WaveCollapse::_choose_pattern(int x, int y) {
    Possibilities& possibilities = grid_options[(x * rows) + y];
    if (possibilities.none()) {
        _dump();
        log_error("we dont have any options...");
    }

    int bit = -1;
    do {
        bit = bitset_utils::get_random_enabled_bit(possibilities, gen,
                                                   num_patterns());
        if (bit == -1) {
            _dump();
            log_error("we dont have any more valid things");
        }
        if (!_eligible_pattern(x, y, bit)) {
            possibilities.set(bit, false);
            bit = -1;
        }

    } while (bit == -1);
    log_info("got random bit {}", bit);

    _place_pattern(x, y, bit);

    Pattern& selected = patterns[bit];
    return selected;
}

bool WaveCollapse::_are_patterns_compatible(const Pattern& a, const Pattern& b,
                                            const Rose& AtoB) const {
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

std::pair<bool, Location> WaveCollapse::_propagate_in_direction(
    Rose r, const Location root, const Pattern& root_pattern) {
    bool changed = false;

    const Location n = _get_relative_loc(r, root.first, root.second);
    // is there a neighbor here?
    if (!_in_grid(n.first, n.second)) return {false, n};
    if (_is_collapsed(n.first * rows + n.second)) return {false, n};

    Possibilities& possibilities = grid_options[n.first * rows + n.second];

    // It has no possible patterns left
    // * TODO revert the previous one?
    if (possibilities.none()) return {false, n};

    // disable all the neighbor's patterns that dont match us
    for (size_t bit = 0; bit < num_patterns(); bit++) {
        // does this location have this pattern enabled?
        if (possibilities.test(bit)) {
            // check to see if it matches us
            bool compatible =
                _are_patterns_compatible(root_pattern, patterns[bit], r);

            // if it doesnt, then disable and add to the list
            if (!compatible) {
                // disable this one since it no longer fits
                possibilities.set(bit, false);
                changed = true;
            }
        }
    }

    return {changed, n};
}

void WaveCollapse::_propagate_choice(int root_x, int root_y) {
    std::deque<Location> q;

    q.push_back({root_x, root_y});

    while (!q.empty()) {
        Location root_loc = q.front();
        auto [x, y] = root_loc;
        q.pop_front();

        Possibilities& pos = grid_options[(x * rows) + y];
        if (pos.none()) continue;
        if (pos.count() != 1) continue;
        int bit = bitset_utils::get_first_enabled_bit(pos);
        // log_info("collapsed pattern in prop was {}", bit);
        const Pattern& collapsed_pattern = patterns[bit];

        // queue up all the neighbors
        magic_enum::enum_for_each<Rose>([&](auto val) {
            // if we did disable any, queue them up to propagate
            auto [changed, n] =
                _propagate_in_direction(val, root_loc, collapsed_pattern);
            if (changed) q.push_back(n);
        });
    }
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

void WaveCollapse::_announce_max_count_chosen(int pattern_id, int x, int y) {
    Pattern& pattern = patterns[pattern_id];
    // infinite uses
    if (pattern.max_count == -1) return;

    // we just used it
    pattern.max_count--;

    // still got more uses
    if (pattern.max_count > 0) return;

    // otherwise no more allowed

    for_each_non_collapsed_cell([&](int i, int j) {
        // skip the one that brought us here..
        if (i == x && j == y) return;

        // set it no longer as a possibility
        Possibilities& possibilities = grid_options[i * rows + j];
        possibilities.set(pattern_id, false);
    });
}

}  // namespace wfc
