
#include "map_generation.h"

namespace wfc {

std::vector<std::string> WaveCollapse::get_lines() {
    std::vector<std::string> lines;
    std::stringstream temp;
    for (int r = 0; r < rows; r++) {
        for (size_t i = 0; i < patterns()[0].pat.size(); i++) {
            for (int c = 0; c < cols; c++) {
                int bit = bitset_utils::get_random_enabled_bit(
                    grid_options[r * rows + c], gen, num_patterns());
                if (bit == -1) {
                    temp << ".....";
                } else {
                    temp << (patterns()[bit].pat)[i];
                }
                // ;
            }
            lines.push_back(std::string(temp.str()));
            temp.str(std::string());
        }
    }

    for (auto line : lines) {
        std::cout << line << std::endl;
    }

    return lines;
}

void WaveCollapse::_dump() {
    for (int i = 0; i < rows * cols; i++) {
        if (i != 0 && i % (rows) == 0) {
            std::cout << std::endl;
        }

        if (grid_options[i].count() >= 2) {
            std::cout << (char) (grid_options[i].count() + 'A');
        }
        if (grid_options[i].count() == 1) {
            int bit = bitset_utils::get_random_enabled_bit(grid_options[i], gen,
                                                           num_patterns());
            std::cout << (bit);
        }
        if (grid_options[i].count() == 0) {
            std::cout << "_";
        }
    }
    std::cout << std::endl;
}

Patterns& WaveCollapse::patterns() { return MAP_GEN_INFO.patterns; }

void WaveCollapse::_validate_patterns() {
    // right now the only rule is that anything required needs max_count

    for (auto& pattern : patterns()) {
        if (pattern.required) {
            if (pattern.max_count == -1) {
                pattern.max_count = 1;
                log_warn(
                    "You have a required pattern {} with no max count, setting "
                    "to 1",
                    pattern.id);
            }
        }
    }
}

void WaveCollapse::_place_required() {
    const auto _place_in_map = [&](Pattern& pattern) {
        // TODO it would be nice to place these randomly instead of just in
        // order...
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                //
                if (pattern.edge_only && !_is_edge(i, j)) continue;

                Possibilities& possibilities = grid_options[i * rows + j];
                // can we hold this pattern at the moment?
                if (possibilities.test(pattern.id)) {
                    possibilities.reset();
                    possibilities.set(pattern.id, true);
                    pattern.max_count--;
                    return;
                }
            }
        }
    };

    for (auto& pattern : patterns()) {
        if (pattern.required) {
            _place_in_map(pattern);
        }
    }
}

void WaveCollapse::_propagate_all() {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            _propagate_choice(i, j);
        }
    }
}

void WaveCollapse::run() {
    _validate_patterns();
    _collapse_edges();
    {
        _dump();
        std::cout << "collapsed edges" << std::endl;
    }
    _propagate_all();
    _place_required();
    {
        _dump();
        std::cout << "placed required" << std::endl;
    }
    _propagate_all();
    _dump();
    std::cout << "completed inital manual placements" << std::endl;

    do {
        auto [x, y] = _find_lowest_entropy();
        log_info("lowest entropy was {} {}", x, y);
        if (_collapsed(x, y)) break;

        auto pattern = _choose_pattern(x, y);
        log_info("selected pattern {} ", pattern.id);
        _handle_max_count(pattern.id, x, y);
        _propagate_choice(x, y);
    } while (_has_non_collapsed());
}

void WaveCollapse::_photo() {
    for (int r = 0; r < rows; r++) {
        for (size_t i = 0; i < patterns()[0].pat.size(); i++) {
            for (int c = 0; c < cols; c++) {
                int bit = bitset_utils::get_random_enabled_bit(
                    grid_options[r * rows + c], gen, num_patterns());
                if (bit == -1) {
                    std::cout << ".....";
                } else {
                    std::cout << (patterns()[bit].pat)[i];
                }
                std::cout << "";
            }
            std::cout << std::endl;
        }
    }

    std::cout << std::endl;
};

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
    return grid_options[x * rows + y].count() == 1;
}

bool WaveCollapse::_has_non_collapsed() const {
    for (int i = 0; i < rows * cols; i++) {
        if (grid_options[i].count() != 1) return true;
    }
    return false;
}

size_t WaveCollapse::num_patterns() const {
    return MAP_GEN_INFO.patterns.size();
}

Location WaveCollapse::_find_lowest_entropy() const {
    if (is_first_one) {
        is_first_one = false;
        return {_gen_rand(0, rows), _gen_rand(0, cols)};
    }

    Location loc{-1, -1};
    size_t c_max = 1;
    for (int i = 0; i < rows * cols; i++) {
        // if you are at max, congrats
        if (grid_options[i].count() == num_patterns()) {
            return {i / rows, i % rows};
        }
        if (grid_options[i].count() > c_max) {
            c_max = grid_options[i].count();
            loc = {i / rows, i % rows};
        }
    }
    return loc;
}

Pattern WaveCollapse::_choose_pattern(int x, int y) {
    Possibilities& possibilities = grid_options[(x * rows) + y];
    if (possibilities.none()) {
        _dump();
        log_error("we dont have any options...");
    }
    int bit = bitset_utils::get_random_enabled_bit(possibilities, gen,
                                                   num_patterns());
    log_info("got random bit {}", bit);

    // Clear all and set the one we selected
    possibilities.reset();
    possibilities.set(bit);

    Pattern selected = patterns()[bit];
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
                _are_patterns_compatible(root_pattern, patterns()[bit], r);

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
        _dump();
        Location root_loc = q.front();
        auto [x, y] = root_loc;
        q.pop_front();

        Possibilities& pos = grid_options[(x * rows) + y];
        if (pos.none()) continue;
        if (pos.count() != 1) continue;
        int bit =
            bitset_utils::get_random_enabled_bit(pos, gen, num_patterns());
        // log_info("collapsed pattern in prop was {}", bit);
        const Pattern& collapsed_pattern = patterns()[bit];

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

// NOTE: we use num_patterns() and not possibilities.size()
// because generally we are going to have less than max patterns
void WaveCollapse::_collapse_edges() {
    const auto _disable_edge_only = [&](int i, int j) {
        // disable edge only patterns on non edges
        Possibilities& possibilities = grid_options[i * rows + j];

        for (size_t bit = 0; bit < num_patterns(); bit++) {
            const Pattern& pattern = patterns()[bit];
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
                    if (patterns()[bit].connections.contains(edge)) {
                        possibilities.set(bit, false);
                    }
                }
            }
        }
    };

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
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

void WaveCollapse::_handle_max_count(int pattern_id, int x, int y) {
    Pattern& pattern = MAP_GEN_INFO.patterns[pattern_id];
    // infinite uses
    if (pattern.max_count == -1) return;

    // we just used it
    pattern.max_count--;

    // still got more uses
    if (pattern.max_count > 0) return;

    // otherwise no more allowed

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            // skip the one that brought us here..
            if (i == x && j == y) continue;

            // set it no longer as a possibility
            Possibilities& possibilities = grid_options[i * rows + j];
            possibilities.set(pattern_id, false);
        }
    }
}

}  // namespace wfc
