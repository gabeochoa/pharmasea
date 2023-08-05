
#pragma once

#include "components/transform.h"
#include "engine/astar.h"
#include "engine/globals_register.h"
#include "engine/ui_color.h"
#include "entity.h"
#include "entity_makers.h"
#include "entityhelper.h"
#include "globals.h"
#include "strings.h"
#include "system/system_manager.h"
#include "tests/test_maps.h"

constexpr int MIN_MAP_SIZE = 5;
constexpr int MAX_MAP_SIZE = 20;
constexpr int MAX_SEED_LENGTH = 20;

extern std::vector<std::string> EXAMPLE_MAP;

static void generate_and_insert_walls(std::string /* seed */) {
    // TODO generate walls based on seed
    const auto d_color = (Color){155, 75, 0, 255};
    for (int i = 0; i < MAX_MAP_SIZE; i++) {
        for (int j = 0; j < MAX_MAP_SIZE; j++) {
            if ((i == 0 && j == 0) || (i == 0 && j == 1)) continue;
            if (i == 0 || j == 0 || i == MAX_MAP_SIZE - 1 ||
                j == MAX_MAP_SIZE - 1) {
                vec2 location = vec2{i * TILESIZE, j * TILESIZE};
                furniture::make_wall(EntityHelper::createEntity(), location,
                                     d_color);
            }
        }
    }
}

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

    template<typename Func = std::function<Entity&()>>
    void generate_entity_from_character(Func&& create, char ch, vec2 location) {
        switch (ch) {
            case 'x':
                x = location;
                make_entity(create(), DebugOptions{.name = "x"},
                            vec::to3(location));
                return;
            case 'z':
                z = location;
                make_entity(create(), DebugOptions{.name = "z"},
                            vec::to3(location));
                return;
            case EMPTY:
                return;
            case SOPHIE: {
                (furniture::make_sophie(create(), vec::to3(location)));
                return;
            } break;
            case REGISTER: {
                (furniture::make_register(create(), location));
                return;
            } break;
            case WALL2:
            case WALL: {
                const auto d_color = Color{155, 75, 0, 255};
                (furniture::make_wall(create(), location, d_color));
                return;
            } break;
            case CUSTOMER: {
                make_customer(create(), location, true);
                return;
            } break;
            case CUST_SPAWNER: {
                furniture::make_customer_spawner(create(), vec::to3(location));
                return;
            } break;
            case GRABBERu: {
                (furniture::make_grabber(create(), location));
                return;
            } break;
            case GRABBERl: {
                Entity& grabber = create();
                (furniture::make_grabber(grabber, location));
                grabber.get<Transform>().rotate_facing_clockwise(270);
                return;
            } break;
            case GRABBERr: {
                Entity& grabber = create();
                (furniture::make_grabber(grabber, location));
                grabber.get<Transform>().rotate_facing_clockwise(90);
                return;
            } break;
            case GRABBERd: {
                Entity& grabber = create();
                (furniture::make_grabber(grabber, location));
                grabber.get<Transform>().rotate_facing_clockwise(180);
                return;
            } break;
            case TABLE: {
                (furniture::make_table(create(), location));
                return;
            } break;
            case MED_CAB: {
                (furniture::make_medicine_cabinet(create(), location));
                return;
            } break;
            case FRUIT: {
                (furniture::make_fruit_basket(create(), location));
                return;
            } break;
            case BLENDER: {
                (furniture::make_blender(create(), location));
                return;
            } break;
            case SODA_MACHINE: {
                (furniture::make_soda_machine(create(), location));
                return;
            } break;
            case CUPBOARD: {
                (furniture::make_cupboard(create(), location));
                return;
            } break;
            case LEMON: {
                (items::make_lemon(create(), location, 0));
                return;
            } break;
            case SIMPLE_SYRUP: {
                (items::make_simple_syrup(create(), location));
                return;
            } break;
            case SQUIRTER: {
                (furniture::make_squirter(create(), location));
                return;
            } break;
            case TRASH: {
                (furniture::make_trash(create(), location));
                return;
            } break;
            case FILTERED_GRABBER: {
                (furniture::make_filtered_grabber(create(), location));
                return;
            } break;
            case PIPE: {
                (furniture::make_pnumatic_pipe(create(), location));
                return;
            } break;
            case MOP_HOLDER: {
                (furniture::make_mop_holder(create(), location));
                return;
            } break;
            case 32: {
                // space
            } break;
            default: {
                log_warn("Found character we dont parse in string '{}'({})", ch,
                         (int) ch);
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
        auto soph = EntityHelper::getFirstMatching([](const Entity& e) {
            return check_name(e, strings::entity::SOPHIE);
        });
        VALIDATE(soph, "sophie needs to be there ");

        // find register,
        auto reg_opt = EntityHelper::getFirstMatching([](const Entity& e) {
            return check_name(e, strings::entity::REGISTER);
        });
        VALIDATE(valid(reg_opt), "map needs to have at least one register");
        const auto& reg = asE(reg_opt);

        // find customer
        auto customer_opt =
            EntityHelper::getFirstMatching([](const Entity& e) -> bool {
                return check_name(e, strings::entity::CUSTOMER_SPAWNER);
            });
        VALIDATE(valid(customer_opt),
                 "map needs to have at least one customer spawn point");
        const auto& customer = asE(customer_opt);

        // ensure customers can make it to the register

        // TODO need a better way to do this
        // 0 makes sense but is the position of the entity, when its infront?
        auto reg_pos = reg.get<Transform>().tile_infront(1);

        log_info(" reg{} rep{} c{}", reg.get<Transform>().as2(), reg_pos,
                 customer.get<Transform>().as2());

        auto new_path = astar::find_path(
            customer.get<Transform>().as2(), reg_pos,
            std::bind(EntityHelper::isWalkable, std::placeholders::_1));
        VALIDATE(new_path.size(),
                 "customer should be able to generate a path to the register");
    }
};

}  // namespace generation

struct LevelInfo {
    bool was_generated = false;

    game::State last_generated = game::State::InMenu;

    Entities entities;
    Entities::size_type num_entities;

    std::string seed;

    //
    size_t hashed_seed;
    std::mt19937 generator;
    std::uniform_int_distribution<> dist;
    //

    void update_seed(const std::string& s) {
        log_info("level info update seed {}", s);
        seed = s;
        hashed_seed = hashString(seed);
        generator = make_engine(hashed_seed);
        // TODO leaving at 1 because we dont have a door to block the entrance
        dist = std::uniform_int_distribution<>(1, MAX_MAP_SIZE - 1);

        was_generated = false;
    }

    virtual void onUpdate(Entities& players, float dt) {
        TRACY_ZONE_SCOPED;
        SystemManager::get().update_all_entities(players, dt);
    }

    virtual void onDraw(float dt) const {
        auto cam = GLOBALS.get_ptr<GameCam>(strings::globals::GAME_CAM);
        if (cam) {
            raylib::DrawBillboard(
                cam->camera, TextureLibrary::get().get(strings::textures::FACE),
                {
                    1.f,
                    0.f,
                    1.f,
                },
                TILESIZE, WHITE);
        }

        TRACY_ZONE_SCOPED;
        SystemManager::get().render_entities(entities, dt);
    }

    virtual void onDrawUI(float dt) {
        SystemManager::get().render_ui(entities, dt);
    }

    void grab_things() {
        {
            this->entities.clear();
            EntityHelper::cleanup();
            this->entities = EntityHelper::get_entities();
            num_entities = this->entities.size();
        }
    }

    void ensure_generated_map(const std::string& new_seed) {
        if (was_generated) return;
        log_info("generating map with new seed: {}", new_seed);
        seed = new_seed;
        was_generated = true;

        // TODO idk which of these i need and which i dont
        // TODO this will delete lobby...
        server_entities_DO_NOT_USE.clear();
        entities.clear();
        EntityHelper::delete_all_entities();

        generate_lobby_map();
        generate_in_game_map();
    }

   private:
    vec3 lobby_origin = {LOBBY_ORIGIN, 0, LOBBY_ORIGIN};

    void generate_lobby_map() {
        {
            auto& entity = EntityHelper::createEntity();
            furniture::make_character_switcher(
                entity, vec::to2(lobby_origin) + vec2{5.f, 5.f});
        }

        {
            auto& entity = EntityHelper::createEntity();
            furniture::make_map_randomizer(
                entity, vec::to2(lobby_origin) + vec2{7.f, 7.f});
        }

        {
            auto& entity = EntityHelper::createEntity();
            furniture::make_trigger_area(
                entity, lobby_origin + vec3{5, TILESIZE / -2.f, 10}, 8, 3,
                text_lookup(strings::i18n::START_GAME));
        }
    }

    void generate_default_seed() {
        generation::helper helper(EXAMPLE_MAP);
        helper.generate();
        helper.validate();
        EntityHelper::invalidatePathCache();
    }

    [[nodiscard]] int gen_rand(int a, int b) {
        return a + (generator() % (b - a));
    }

    void generate_in_game_map() {
        if (seed == "default_seed") {
            generate_default_seed();
            return;
        }

        std::vector<std::string> lines;

        int rows = gen_rand(MIN_MAP_SIZE, MAX_MAP_SIZE);
        int cols = gen_rand(MIN_MAP_SIZE, MAX_MAP_SIZE);

        const auto get_char = [lines](int i, int j) -> char {
            if (i < 0) return '.';
            if (j < 0) return '.';
            if (i >= (int) lines.size()) return '.';
            if (j >= (int) lines[i].size()) return '.';
            return lines[i][j];
        };

        const auto _is_empty = [get_char, lines](int i, int j) -> bool {
            // TODO probably should allow caller to set default on fail
            return get_char(i, j) == '.';
        };

        const auto _get_valid_register_location =
            [_is_empty, rows, cols, this]() -> std::pair<int, int> {
            int tries = 0;
            int x;
            int y;
            do {
                x = gen_rand(1, rows - 1);
                y = gen_rand(1, cols - 1);
                if (tries++ > 100) {
                    x = 0;
                    y = 0;
                    break;
                }
            } while (  //
                !_is_empty(x, y) && !_is_empty(x, y + 1));
            return {x, y};
        };

        // setup_lines
        {
            for (int i = 0; i < rows + 5; i++) {
                std::string l(cols + 10, '.');
                lines.push_back(l);
            }
        }

        // setup_boundary_walls
        {
            for (int i = 0; i < rows; i++) {
                if (i == 0 || i == rows - 1) {
                    lines[i] = std::string(cols, '#');
                    continue;
                }
                lines[i][0] = generation::WALL;
                lines[i][cols] = generation::WALL;
            }

            // Add one empty spot so that we can get into the box
            int y = gen_rand(1, rows - 1);
            lines[y][cols] = '.';
        }

        // add_customer_spawner + sophie
        {
            lines[1][cols + 1] = generation::CUST_SPAWNER;
            lines[2][cols + 1] = generation::SOPHIE;
        }

        // place register
        {
            auto [x, y] = _get_valid_register_location();
            lines[x][y] = generation::REGISTER;
            log_info("placing register at {} {}", x, y);
        }

        for (auto line : lines) {
            log_info("{}", line);
        }

        //////////
        //////////
        //////////

        generation::helper helper(lines);
        helper.generate();
        helper.validate();
        EntityHelper::invalidatePathCache();
    }

    auto get_rand_walkable() {
        vec2 location;
        do {
            location =
                vec2{dist(generator) * TILESIZE, dist(generator) * TILESIZE};
        } while (!EntityHelper::isWalkable(location));
        return location;
    }

    auto get_rand_walkable_register() {
        vec2 location;
        do {
            location =
                vec2{dist(generator) * TILESIZE, dist(generator) * TILESIZE};
        } while (!EntityHelper::isWalkable(location) &&
                 !EntityHelper::isWalkable(
                     vec2{location.x, location.y + 1 * TILESIZE}));
        return location;
    }

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.value8b(num_entities);
        s.container(entities, num_entities,
                    [](S& s2, std::shared_ptr<Entity>& entity) {
                        s2.ext(entity, bitsery::ext::StdSmartPtr{});
                    });
        s.value1b(was_generated);
        s.text1b(seed, MAX_SEED_LENGTH);

        s.value8b(hashed_seed);
        // TODO these arent serializable...
        // s.object(generator);
        // s.object(dist);
    }
};
