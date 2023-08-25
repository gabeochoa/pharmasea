
/*
#include "game.h"

namespace network {
long long total_ping = 0;
long long there_ping = 0;
long long return_ping = 0;
}  // namespace network

int main(int argc, char* argv[]) {
    process_dev_flags(argv);

    tests::run_all();

    if (TESTS_ONLY) {
        std::cout << "All tests ran " << std::endl;
        return 0;
    }

    std::cout << "Executable Path: " << fs::current_path() << std::endl;
    std::cout << "Canon Path: " << fs::canonical(fs::current_path())
              << std::endl;

    startup();
    defer(Settings::get().write_save_file());

    if (argc > 1) {
        bool is_test = strcmp(argv[1], "test") == 0;
        if (is_test) {
            bool is_host = strcmp(argv[2], "host") == 0;
            int a = setup_multiplayer_test(is_host);
            if (a < 0) {
                return -1;
            }
        }
    }

    try {
        App::get().run();
    } catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
    }
    return 0;
}
*/

#include <bitset>
#include <deque>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "engine/assert.h"
#include "engine/bitset_utils.h"
#include "engine/log.h"
#include "vec_util.h"
#include "vendor_include.h"

constexpr int NUM_PATTERNS = 8;
typedef std::bitset<NUM_PATTERNS> Possibilities;

typedef std::pair<int, int> Location;
typedef std::vector<Location> Locations;

enum Rose { N, E, W, S };

typedef std::set<Rose> Connections;
struct Pattern {
    int id = -1;
    std::vector<std::string> pat;
    Connections connections;
    bool required = false;
};

constexpr std::vector<std::string> rotateCounter90Degrees(
    const std::vector<std::string>& original) {
    std::vector<std::string> rotated;

    if (original.empty()) {
        return rotated;  // Return an empty vector if the input is empty
    }

    size_t rows = original.size();
    size_t cols = original[0].size();  // Assuming all rows have the same length

    rotated.resize(
        cols,
        std::string(rows, ' '));  // Initialize the rotated grid with spaces

    for (size_t i = 0; i < rows; ++i) {
        for (size_t j = 0; j < cols; ++j) {
            rotated[cols - j - 1][i] = original[i][j];  // Swap i and j indices
        }
    }

    return rotated;
}

constexpr std::vector<std::string> rotate90Degrees(
    const std::vector<std::string>& original) {
    std::vector<std::string> rotated;

    if (original.empty()) {
        return rotated;  // Return an empty vector if the input is empty
    }

    size_t rows = original.size();
    size_t cols = original[0].size();  // Assuming all rows have the same length

    rotated.resize(
        cols,
        std::string(rows, ' '));  // Initialize the rotated grid with spaces

    for (size_t i = 0; i < rows; ++i) {
        for (size_t j = 0; j < cols; ++j) {
            rotated[j][rows - i - 1] = original[i][j];
        }
    }

    return rotated;
}

const std::array<Pattern, NUM_PATTERNS> patterns = {
    //
    //
    //     IDK why but the pat's need to be
    //     rotated clockwise 90 degress to match correctly
    //     ...
    //
    //
    Pattern{
        .id = 0,
        .pat =
            {
                "#####",  //
                ".....",  //
                ".....",  //
                ".....",  //
                "....."   //
            },
        .connections =
            {
                E,
                W,
                S,
            }  //
    },
    //
    Pattern{
        .id = 1,
        .pat =
            {
                "....#",  //
                "....#",  //
                "....#",  //
                "....#",  //
                "....#"   //
            },
        .connections =
            {
                N,
                W,
                S,
            }  //
    },
    //
    Pattern{
        .id = 2,
        .pat =
            {
                ".....",  //
                ".....",  //
                ".....",  //
                ".....",  //
                "#####"   //
            },
        .connections =
            {
                N,
                W,
                E,
            }  //
    },
    //
    Pattern{
        .id = 3,
        .pat =
            {
                "#....",  //
                "#....",  //
                "#....",  //
                "#....",  //
                "#...."   //
            },
        .connections =
            {
                N,
                S,
                E,
            }  //
    },
    //
    Pattern{
        .id = 4,
        .pat =
            {
                "#####",  //
                "#....",  //
                "#....",  //
                "#....",  //
                "#...."   //
            },
        .connections =
            {
                S,
                E,
            }  //
    },
    //
    Pattern{
        .id = 5,
        .pat =
            {
                "#####",  //
                "....#",  //
                "....#",  //
                "....#",  //
                "....#"   //
            },
        .connections =
            {
                S,
                W,
            }  //
    },
    //
    Pattern{
        .id = 6,
        .pat =
            {
                "....#",  //
                "....#",  //
                "....#",  //
                "....#",  //
                "#####"   //
            },
        .connections =
            {
                N,
                W,
            }  //
    },
    //
    Pattern{
        .id = 7,
        .pat =
            {
                "#....",  //
                "#....",  //
                "#....",  //
                "#....",  //
                "#####"   //
            },
        .connections =
            {
                N,
                E,
            }  //
    },
    //
};

struct WaveCollapse {
    int rows = 5;
    int cols = 5;

    // represents for each cell what options are still possible
    std::vector<Possibilities> grid_options;

    // We mark this mutable since its not really what we _mean_ by const
    mutable std::mt19937 gen;
    mutable bool is_first_one;

    WaveCollapse() {
        grid_options =
            std::vector<Possibilities>(rows * cols, Possibilities().set());
        gen = std::mt19937((unsigned int) 0);
        is_first_one = true;
    }

    void _dump() {
        for (int i = 0; i < rows * cols; i++) {
            if (i != 0 && i % (rows) == 0) {
                std::cout << std::endl;
            }

            if (grid_options[i].count() >= 2) {
                std::cout << (char) (grid_options[i].count() + 'A');
            }
            if (grid_options[i].count() == 1) {
                int bit =
                    bitset_utils::get_random_enabled_bit(grid_options[i], gen);
                std::cout << (bit);
            }
            if (grid_options[i].count() == 0) {
                std::cout << "_";
            }
        }
        std::cout << std::endl;
    }

    void run() {
        _collapse_edges_and_propagate();
        _dump();

        do {
            auto [x, y] = _find_lowest_entropy();
            log_info("lowest entropy was {} {}", x, y);
            if (_collapsed(x, y)) break;

            auto pattern = _choose_pattern(x, y);
            log_info("selected pattern {} ", pattern.id);
            _propagate_choice(x, y);
        } while (_has_non_collapsed());
    }

    auto _photo() {
        for (int r = 0; r < rows; r++) {
            for (int i = 0; i < patterns[0].pat.size(); i++) {
                for (int c = 0; c < cols; c++) {
                    int bit = bitset_utils::get_random_enabled_bit(
                        grid_options[r * rows + c], gen);
                    if (bit == -1) {
                        std::cout << ".....";
                    } else {
                        std::cout << (patterns[bit].pat)[i];
                    }
                    std::cout << " ";
                }
                std::cout << std::endl;
            }
        }

        std::cout << std::endl;
    };

   private:
    int _gen_rand(int a, int b) const { return a + (gen() % (b - a)); }

    Rose _get_opposite_connection(Rose r) const {
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
    }

    Location _get_relative_loc(Rose r, int x, int y) const {
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
    }

    bool _in_grid(int x, int y) const {
        return x >= 0 && x < rows && y >= 0 && y < cols;
    }

    bool _collapsed(int x, int y) const {
        if (x == -1 || y == -1) return true;
        return grid_options[x * rows + y].count() == 1;
    }

    bool _has_non_collapsed() const {
        for (int i = 0; i < rows * cols; i++) {
            if (grid_options[i].count() != 1) return true;
        }
        return false;
    }

    Location _find_lowest_entropy() const {
        if (is_first_one) {
            is_first_one = false;
            return {_gen_rand(0, rows), _gen_rand(0, cols)};
        }

        Location loc{-1, -1};
        size_t c_max = 1;
        for (int i = 0; i < rows * cols; i++) {
            // if you are at max, congrats
            if (grid_options[i].count() == patterns.size()) {
                return {i / rows, i % rows};
            }
            if (grid_options[i].count() > c_max) {
                c_max = grid_options[i].count();
                loc = {i / rows, i % rows};
            }
        }
        return loc;
    }

    Pattern _choose_pattern(int x, int y) {
        Possibilities& possibilities = grid_options[(x * rows) + y];
        if (possibilities.none()) {
            _dump();
            log_error("we dont have any options...");
        }
        int bit = bitset_utils::get_random_enabled_bit(possibilities, gen);
        log_info("got random bit {}", bit);

        // Clear all and set the one we selected
        possibilities.reset();
        possibilities.set(bit);

        Pattern selected = patterns[bit];
        return selected;
    }

    bool _are_patterns_compatible(const Pattern& a, const Pattern& b,
                                  const Rose& AtoB) const {
        log_info("are_compatible {} {} {}", a.id, b.id,
                 magic_enum::enum_name(AtoB));
        bool does_a_connect_in_this_direction = false;
        for (const auto c : a.connections) {
            if (c == AtoB) {
                does_a_connect_in_this_direction = true;
                break;
            }
        }

        // if (!does_a_connect_in_this_direction) {
        // log_info("noaconnect {}, {}, {}", a.id, b.id,
        // magic_enum::enum_name(AtoB));
        // return false;
        // }

        bool does_b_connect_in_this_direction = false;
        for (const auto c : b.connections) {
            if (c == _get_opposite_connection(AtoB)) {
                does_b_connect_in_this_direction = true;
                break;
            }
        }
        // if (!does_b_connect_in_this_direction) {
        // log_info("nobconnect {}, {}, {}", a.id, b.id,
        // magic_enum::enum_name(AtoB));
        // return false;
        // }
        log_info("true {}, {}, {}", a.id, b.id, magic_enum::enum_name(AtoB));

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

    std::pair<bool, Location> _propagate_in_direction(
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
        for (size_t bit = 0; bit < possibilities.size(); bit++) {
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

    void _propagate_choice(int root_x, int root_y) {
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
            int bit = bitset_utils::get_random_enabled_bit(pos, gen);
            log_info("collapsed pattern in prop was {}", bit);
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

    std::vector<Rose> _get_edges(int x, int y) {
        std::vector<Rose> edges;
        if (x == 0) edges.push_back(Rose::N);
        if (x == rows - 1) edges.push_back(Rose::S);
        if (y == 0) edges.push_back(Rose::W);
        if (y == cols - 1) edges.push_back(Rose::E);
        return edges;
    }

    void _collapse_edges_and_propagate() {
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                if (i == 0 || j == 0 || j == cols - 1 || i == rows - 1) {
                    // What edge are we on
                    auto banned_edges = _get_edges(i, j);

                    Possibilities& possibilities = grid_options[i * rows + j];
                    // disable all the neighbor's patterns that dont match us
                    for (size_t bit = 0; bit < possibilities.size(); bit++) {
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
                }
            }
        }
        _dump();
        std::cout << "collapsed edges, now prop" << std::endl;
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                _propagate_choice(i, j);
            }
        }
    }
};

void wave_collapse() {
    WaveCollapse wc;

    wc.run();
    wc._dump();
    wc._photo();
}

int main() {
    wave_collapse();
    return 0;
}
