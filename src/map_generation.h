

#include <string>
#include <vector>

#include "engine/astar.h"
#include "engine/bitset_utils.h"
#include "entityhelper.h"
#include "strings.h"

namespace wfc {

constexpr int MAX_NUM_PATTERNS = 64;
typedef std::bitset<MAX_NUM_PATTERNS> Possibilities;

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
typedef std::vector<Pattern> Patterns;

struct MapGenerationInformation {
    Patterns patterns;
    int rows = 5;
    int cols = 5;
};
extern MapGenerationInformation MAP_GEN_INFO;

struct WaveCollapse {
    int rows;
    int cols;

    // represents for each cell what options are still possible
    std::vector<Possibilities> grid_options;

    // We mark this mutable since its not really what we _mean_ by const
    mutable std::mt19937 gen;
    mutable bool is_first_one;

    WaveCollapse(int r, int c) : rows(r), cols(c) {
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

    Patterns& patterns() { return MAP_GEN_INFO.patterns; }

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
            for (size_t i = 0; i < patterns()[0].pat.size(); i++) {
                for (int c = 0; c < cols; c++) {
                    int bit = bitset_utils::get_random_enabled_bit(
                        grid_options[r * rows + c], gen);
                    if (bit == -1) {
                        std::cout << ".....";
                    } else {
                        std::cout << (patterns()[bit].pat)[i];
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
        log_error("How did we get a Rose that wasnt handled in the switch");
        return N;
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
        log_error("How did we get a Rose that wasnt handled in the switch");
        return {x, y};
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

    size_t num_patterns() const { return MAP_GEN_INFO.patterns.size(); }

    Location _find_lowest_entropy() const {
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

        Pattern selected = patterns()[bit];
        return selected;
    }

    bool _are_patterns_compatible(const Pattern& a, const Pattern& b,
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
                                if (patterns()[bit].connections.contains(
                                        edge)) {
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

}  // namespace wfc

namespace generation {

const char WALL = '#';
const char WALL2 = 'w';
const char EMPTY = '.';
const char ORIGIN = '0';
const char CUSTOMER = 'c';
const char CUST_SPAWNER = 'C';
const char REGISTER = 'R';
const char TABLE = 't';

const char GRABBERu = '^';
const char GRABBERl = '<';
const char GRABBERr = '>';
const char GRABBERd = 'v';

const char MED_CAB = 'M';
const char FRUIT = 'F';
const char BLENDER = 'b';
const char SODA_MACHINE = 'S';

const char CUPBOARD = 'd';
const char LEMON = 'l';
const char SIMPLE_SYRUP = 'y';
const char SQUIRTER = 'q';
const char TRASH = 'T';
const char FILTERED_GRABBER = 'G';
const char PIPE = 'p';
const char MOP_HOLDER = 'm';
const char FAST_FORWARD = 'f';
const char MOP_BUDDY = 'B';

const char SOPHIE = 's';

struct helper {
    std::vector<std::string> lines;

    // For testing only
    vec2 x = {0, 0};
    vec2 z = {0, 0};

    helper(const std::vector<std::string>& l) : lines(l) {}

    template<typename Func = std::function<Entity&()>>
    void generate(Func&& add_to_map = nullptr) {
        vec2 origin = find_origin();

        const auto default_create = []() -> Entity& {
            return EntityHelper::createEntity();
        };

        for (int i = 0; i < (int) lines.size(); i++) {
            auto line = lines[i];
            for (int j = 0; j < (int) line.size(); j++) {
                vec2 raw_location = vec2{i * TILESIZE, j * TILESIZE};
                vec2 location = raw_location - origin;
                auto ch = get_char(i, j);
                if (add_to_map) {
                    generate_entity_from_character(add_to_map, ch, location);
                } else {
                    generate_entity_from_character(default_create, ch,
                                                   location);
                }
            }
        }
    }

    EntityType convert_character_to_type(char ch) {
        switch (ch) {
            case EMPTY:
                return EntityType::Unknown;
            case 'x':
                return EntityType::x;
            case 'z':
                return EntityType::z;
            case SOPHIE:
                return EntityType::Sophie;
            case REGISTER: {
                return EntityType::Register;
            } break;
            case WALL2:
            case WALL: {
                return EntityType::Wall;
            } break;
            case CUSTOMER: {
                return EntityType::Customer;
            } break;
            case CUST_SPAWNER: {
                return EntityType::CustomerSpawner;
            } break;
            case GRABBERu: {
                return EntityType::Grabber;
            } break;
            case GRABBERl: {
                return EntityType::Grabber;
            } break;
            case GRABBERr: {
                return EntityType::Grabber;
            } break;
            case GRABBERd: {
                return EntityType::Grabber;
            } break;
            case TABLE: {
                return EntityType::Table;
            } break;
            case MED_CAB: {
                return EntityType::MedicineCabinet;
            } break;
            case FRUIT: {
                return EntityType::PillDispenser;
            } break;
            case BLENDER: {
                return EntityType::Blender;
            } break;
            case SODA_MACHINE: {
                return EntityType::SodaMachine;
            } break;
            case CUPBOARD: {
                return EntityType::Cupboard;
            } break;
            case LEMON: {
                return EntityType::Lemon;
            } break;
            case SIMPLE_SYRUP: {
                return EntityType::SimpleSyrup;
            } break;
            case SQUIRTER: {
                return EntityType::Squirter;
            } break;
            case TRASH: {
                return EntityType::Trash;
            } break;
            case FILTERED_GRABBER: {
                return EntityType::FilteredGrabber;
            } break;
            case PIPE: {
                return EntityType::PnumaticPipe;
            } break;
            case MOP_HOLDER: {
                return EntityType::MopHolder;
            } break;
            case FAST_FORWARD: {
                return EntityType::FastForward;
            } break;
            case MOP_BUDDY: {
                return EntityType::MopBuddy;
            } break;
            case 32: {
                // space
            } break;
            default: {
                log_warn("Found character we dont parse in string '{}'({})", ch,
                         (int) ch);
            } break;
        }
        return EntityType::Unknown;
    }

    template<typename Func = std::function<Entity&()>>
    void generate_entity_from_character(Func&& create, char ch, vec2 location) {
        // This is not a warning since most maps are made up of '.'s
        if (ch == EMPTY || ch == 32) return;

        EntityType et = convert_character_to_type(ch);

        if (et == EntityType::Unknown) {
            log_warn(
                "you are trying to create an unknown type with character {}, "
                "refusing to make it ",
                ch);
            return;
        }

        Entity& entity = create();
        convert_to_type(et, entity, location);
        switch (ch) {
            case 'x': {
                x = location;
            } break;
            case 'z': {
                z = location;
            } break;
            case GRABBERl: {
                entity.get<Transform>().rotate_facing_clockwise(270);
            } break;
            case GRABBERr: {
                entity.get<Transform>().rotate_facing_clockwise(90);
            } break;
            case GRABBERd: {
                entity.get<Transform>().rotate_facing_clockwise(180);
            } break;
        };
        return;
    }

    inline vec2 find_origin() {
        vec2 origin{0.0, 0.0};
        for (int i = 0; i < (int) lines.size(); i++) {
            auto line = lines[i];
            for (int j = 0; j < (int) line.size(); j++) {
                auto ch = line[j];
                if (ch == '0') {
                    origin = vec2{j * TILESIZE, i * TILESIZE};
                    break;
                }
            }
        }
        return origin;
    }

    inline char get_char(int i, int j) {
        if (i < 0) return '.';
        if (j < 0) return '.';
        if (i >= (int) lines.size()) return '.';
        if (j >= (int) lines[i].size()) return '.';
        return lines[i][j];
    }

    void validate() {
        const auto validate_exist = [](EntityType et) {
            auto item = EntityHelper::getFirstMatching(
                [et](const Entity& e) { return check_type(e, et); });
            VALIDATE(item, fmt::format("{} needs to be there ", et));
        };

        validate_exist(EntityType::Sophie);
        validate_exist(EntityType::Register);
        validate_exist(EntityType::CustomerSpawner);
        validate_exist(EntityType::FastForward);
        validate_exist(EntityType::Cupboard);
        validate_exist(EntityType::SodaMachine);
        validate_exist(EntityType::MopHolder);

        // ensure customers can make it to the register
        {
            // find customer
            auto customer_opt =
                EntityHelper::getFirstMatching([](const Entity& e) -> bool {
                    return check_type(e, EntityType::CustomerSpawner);
                });
            // TODO :DESIGN: we are validating this now, but we shouldnt have to
            // worry about this in the future
            VALIDATE(valid(customer_opt),
                     "map needs to have at least one customer spawn point");
            auto& customer = asE(customer_opt);

            auto reg_opt =
                EntityHelper::getFirstMatching([&customer](const Entity& e) {
                    if (!check_type(e, EntityType::Register)) return false;
                    // TODO :INFRA: need a better way to do this
                    // 0 makes sense but is the position of the entity, when
                    // its infront?
                    auto new_path =
                        astar::find_path(customer.get<Transform>().as2(),
                                         e.get<Transform>().tile_infront(1),
                                         std::bind(EntityHelper::isWalkable,
                                                   std::placeholders::_1));
                    return new_path.size() > 0;
                });

            VALIDATE(valid(reg_opt),
                     "customer should be able to generate a path to the "
                     "register");
        }
    }
};

}  // namespace generation
