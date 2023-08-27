

#include <sstream>
#include <string>
#include <vector>

#include "engine/astar.h"
#include "engine/bitset_utils.h"
#include "entity_helper.h"
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
    int max_count = -1;
    bool edge_only = false;
};
typedef std::vector<Pattern> Patterns;

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
    mutable bool is_first_one;

    WaveCollapse(int r, int c, unsigned int seed) : rows(r), cols(c) {
        numbers.resize(rows * cols);
        for (int i = 0; i < rows * cols; ++i) numbers[i] = i;

        patterns = Patterns();
        patterns.reserve(MAP_GEN_INFO.patterns.size());
        // taking as non-ref to copy the patterns into our object because we
        // edit them as we generate
        for (Pattern pattern : MAP_GEN_INFO.patterns) {
            patterns.push_back(pattern);
        }

        // We manually only set the num patterns we have
        // because its likely smaller than MAX_NUM_PATTERNS
        Possibilities default_val;
        for (size_t i = 0; i < num_patterns(); i++) {
            default_val.set(i, true);
        }

        grid_options = std::vector<Possibilities>(rows * cols, default_val);
        gen = std::mt19937(seed);
        is_first_one = true;

        // Random stuff should happen under here
        //

        _shuffle_visit_order();
    }

    WaveCollapse(unsigned int seed)
        : WaveCollapse(MAP_GEN_INFO.rows, MAP_GEN_INFO.cols, seed) {}

    std::vector<std::string> get_lines();
    void _dump() const;

    void run();
    void _photo() const;

   private:
    bool _is_collapsed(int index) const { return collapsed.contains(index); }
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

    size_t pat_size() const;
    int _gen_rand(int a, int b) const;
    Rose _get_opposite_connection(Rose r) const;
    Location _get_relative_loc(Rose r, int x, int y) const;
    bool _in_grid(int x, int y) const;
    bool _is_edge(int x, int y) const;
    bool _collapsed(int x, int y) const;
    bool _eligible_pattern(int x, int y, int pattern_id) const;
    void _place_pattern(int x, int y, int pattern_id);
    bool _has_non_collapsed() const;
    size_t num_patterns() const;
    void _validate_patterns() const;
    void _place_required();
    void _place_walls();
    void _propagate_all();
    Location _find_lowest_entropy() const;
    Pattern& _choose_pattern(int x, int y);
    bool _are_patterns_compatible(const Pattern& a, const Pattern& b,
                                  const Rose& AtoB) const;
    std::pair<bool, Location> _propagate_in_direction(
        Rose r, const Location root, const Pattern& root_pattern);

    void _shuffle_visit_order() const {
        std::shuffle(numbers.begin(), numbers.end(), gen);
    }

    void _propagate_choice(int root_x, int root_y);

    std::vector<Rose> _get_edges(int x, int y);

    void _initial_cleanup();

    void _announce_max_count_chosen(int pattern_id, int x, int y);
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
            OptEntity customer =
                EntityHelper::getFirstMatching([](const Entity& e) -> bool {
                    return check_type(e, EntityType::CustomerSpawner);
                });
            // TODO :DESIGN: we are validating this now, but we shouldnt have to
            // worry about this in the future
            VALIDATE(customer,
                     "map needs to have at least one customer spawn point");

            OptEntity reg =
                EntityHelper::getFirstMatching([&customer](const Entity& e) {
                    if (!check_type(e, EntityType::Register)) return false;
                    // TODO :INFRA: need a better way to do this
                    // 0 makes sense but is the position of the entity, when
                    // its infront?
                    auto new_path =
                        astar::find_path(customer->get<Transform>().as2(),
                                         e.get<Transform>().tile_infront(1),
                                         std::bind(EntityHelper::isWalkable,
                                                   std::placeholders::_1));
                    return new_path.size() > 0;
                });

            VALIDATE(
                reg,
                "customer should be able to generate a path to the register");
        }
    }
};

}  // namespace generation
