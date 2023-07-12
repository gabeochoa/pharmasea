
#pragma once

#include "components/transform.h"
#include "engine/astar.h"
#include "engine/globals_register.h"
#include "engine/ui_color.h"
#include "entity.h"
#include "entityhelper.h"
#include "item.h"
#include "item_helper.h"
#include "strings.h"
#include "system/system_manager.h"
#include "tests/test_maps.h"

constexpr int MAX_MAP_SIZE = 20;
constexpr int MAX_SEED_LENGTH = 20;

static void generate_and_insert_walls(std::string /* seed */) {
    // TODO generate walls based on seed
    const auto d_color = (Color){155, 75, 0, 255};
    for (int i = 0; i < MAX_MAP_SIZE; i++) {
        for (int j = 0; j < MAX_MAP_SIZE; j++) {
            if ((i == 0 && j == 0) || (i == 0 && j == 1)) continue;
            if (i == 0 || j == 0 || i == MAX_MAP_SIZE - 1 ||
                j == MAX_MAP_SIZE - 1) {
                vec2 location = vec2{i * TILESIZE, j * TILESIZE};
                entities::make_wall(EntityHelper::createEntity(), location,
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
const char PLAYER = '@';

const char GRABBERu = '^';
const char GRABBERl = '<';
const char GRABBERr = '>';
const char GRABBERd = 'v';

const char BAGBOX = 'B';
const char MED_CAB = 'M';
const char PILL_DISP = 'P';

const char SOPHIE = 's';

struct helper {
    std::vector<std::string> lines;

    // For testing only
    vec2 x = {0, 0};
    vec2 z = {0, 0};

    helper(const std::vector<std::string>& l) : lines(l) {}

    void generate(std::function<Entity&()> add_to_map = nullptr) {
        if (!add_to_map) {
            add_to_map = []() -> Entity& {
                return EntityHelper::createEntity();
            };
        }
        vec2 origin = find_origin();

        for (int i = 0; i < (int) lines.size(); i++) {
            auto line = lines[i];
            for (int j = 0; j < (int) line.size(); j++) {
                vec2 raw_location = vec2{i * TILESIZE, j * TILESIZE};
                vec2 location = raw_location - origin;
                auto ch = get_char(i, j);
                generate_entity_from_character(add_to_map, ch, location);
            }
        }
    }

    void generate_entity_from_character(std::function<Entity&()> create,
                                        char ch, vec2 location) {
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
                (entities::make_sophie(create(), vec::to3(location)));
                return;
            } break;
            case REGISTER: {
                (entities::make_register(create(), location));
                return;
            } break;
            case WALL2:
            case WALL: {
                const auto d_color = (Color){155, 75, 0, 255};
                (entities::make_wall(create(), location, d_color));
                return;
            } break;
            case CUSTOMER: {
                make_customer(create(), location, true);
                return;
            } break;
            case CUST_SPAWNER: {
                entities::make_customer_spawner(create(), vec::to3(location));
                return;
            } break;
            case GRABBERu: {
                (entities::make_grabber(create(), location));
                return;
            } break;
            case GRABBERl: {
                Entity& grabber = create();
                (entities::make_grabber(grabber, location));
                grabber.get<Transform>().rotate_facing_clockwise(270);
                return;
            } break;
            case GRABBERr: {
                Entity& grabber = create();
                (entities::make_grabber(grabber, location));
                grabber.get<Transform>().rotate_facing_clockwise(90);
                return;
            } break;
            case GRABBERd: {
                Entity& grabber = create();
                (entities::make_grabber(grabber, location));
                grabber.get<Transform>().rotate_facing_clockwise(180);
                return;
            } break;
            case TABLE: {
                (entities::make_table(create(), location));
                return;
            } break;
            case BAGBOX: {
                (entities::make_bagbox(create(), location));
                return;
            } break;
            case MED_CAB: {
                (entities::make_medicine_cabinet(create(), location));
                return;
            } break;
            case PILL_DISP: {
                (entities::make_pill_dispenser(create(), location));
                return;
            } break;
            case PLAYER: {
                global_player->get<Transform>().update(vec::to3(location));
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
        auto soph = EntityHelper::getFirstMatching(
            [](Entity e) { return check_name(e, strings::entity::SOPHIE); });
        VALIDATE(soph, "sophie needs to be there ");

        // find register,
        auto reg_opt = EntityHelper::getFirstMatching(
            [](Entity e) { return check_name(e, strings::entity::REGISTER); });
        VALIDATE(valid(reg_opt), "map needs to have at least one register");
        auto reg = asE(reg_opt);

        // find customer
        auto customer_opt =
            EntityHelper::getFirstMatching([](Entity& e) -> bool {
                return check_name(e, strings::entity::CUSTOMER_SPAWNER);
            });
        VALIDATE(valid(customer_opt),
                 "map needs to have at least one customer spawn point");
        auto customer = asE(customer_opt);

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

    Entities entities;
    Entities::size_type num_entities;

    Items items;
    Items::size_type num_items;

    std::string seed;

    virtual void onUpdate(Entities& players, float dt) {
        TRACY_ZONE_SCOPED;
        SystemManager::get().update_all_entities(players, dt);
    }

    virtual void onDraw(float dt) const {
        TRACY_ZONE_SCOPED;
        SystemManager::get().render_entities(entities, dt);
        SystemManager::get().render_items(items, dt);
    }

    virtual void onDrawUI(float dt) {
        SystemManager::get().render_ui(entities, dt);
    }

    void grab_things() {
        {
            entities.clear();
            EntityHelper::cleanup();
            auto es = EntityHelper::get_entities();
            this->entities = es;
            num_entities = this->entities.size();
        }

        {
            items.clear();
            auto is = ItemHelper::get_items();
            this->items = is;
            num_items = this->items.size();
        }
    }

    void ensure_generated_map(const std::string& new_seed) {
        if (was_generated) return;
        seed = new_seed;
        was_generated = true;
        generate_map();
    }

    virtual void generate_map() = 0;

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.value8b(num_entities);
        s.container(entities, num_entities,
                    [](S& s2, Entity entity) { s2.object(entity); });
        s.value8b(num_items);
        s.container(items, num_items, [](S& s2, std::shared_ptr<Item>& item) {
            s2.ext(item, bitsery::ext::StdSmartPtr{});
        });
        s.value1b(was_generated);
        s.text1b(seed, MAX_SEED_LENGTH);
    }
};

struct LobbyMapInfo : public LevelInfo {
    virtual void generate_map() override {
        {
            auto& entity = EntityHelper::createEntity();
            entities::make_character_switcher(entity, vec2{5.f, 5.f});
        }

        {
            auto& entity = EntityHelper::createEntity();
            entities::make_trigger_area(entity, {5, TILESIZE / -2.f, 10}, 8, 3,
                                        text_lookup(strings::i18n::START_GAME));
        }
    }

    virtual void onDraw(float dt) const override {
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

        LevelInfo::onDraw(dt);
    }

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<LevelInfo>{});
    }
};

struct GameMapInfo : public LevelInfo {
    //
    size_t hashed_seed;
    std::mt19937 generator;
    std::uniform_int_distribution<> dist;
    //

    void update_seed(const std::string& s) {
        seed = s;
        hashed_seed = hashString(seed);
        generator = make_engine(hashed_seed);
        // TODO leaving at 1 because we dont have a door to block the entrance
        dist = std::uniform_int_distribution<>(1, MAX_MAP_SIZE - 1);

        // TODO need to regenerate the map and clean up entitiyhelper
    }

   private:
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

    virtual void generate_map() override {
        server_entities_DO_NOT_USE.clear();
        server_items_DO_NOT_USE.clear();

        // TODO eventually this would be generated using the seed
        const std::string EXAMPLE_MAP_ = R"(
#####################
#...................#
#.....t.............#
#P....R....@........#
#B....R.............#
#M....t...0.........#
#.....t.............#
#.....t.............#
###########.....#####
#...................#
#..>v.....#.........#
#..<>.....#.........#
#..<>.....#.........#
#..^<.....#.........#
#.........#.........#
#######..############
..............C.....s)";

        auto lines = util::split_string(EXAMPLE_MAP_, "\n");
        generation::helper helper(lines);
        helper.generate();

        // TODO when you place a register we need to make sure you cant
        // start the game until it has an empty spot infront of it
        //
        // TODO run a lighter version of validate every time the player moves
        // things around
        helper.validate();

        /*
        auto generate_conveyer_test = []() {
            auto location = vec2{-5, 0};
            for (int i = 0; i < 5; i++) {
                location.y++;
                std::shared_ptr<Furniture> conveyer;
                conveyer.reset(entities::make_grabber(location));
                EntityHelper::addEntity(conveyer);
            }

            {
                location.y += 1;
                std::shared_ptr<Furniture> conveyer;
                conveyer.reset(entities::make_grabber(location));
                conveyer->get<Transform>().rotate_facing_clockwise(270);
                EntityHelper::addEntity(conveyer);
            }

            {
                location.x -= 1;
                std::shared_ptr<Furniture> conveyer;
                conveyer.reset(entities::make_grabber(location));
                conveyer->get<Transform>().rotate_facing_clockwise(180);
                EntityHelper::addEntity(conveyer);
            }

            for (int i = 0; i < 4; i++) {
                location.y--;
                std::shared_ptr<Furniture> conveyer;
                conveyer.reset(entities::make_grabber(location));
                conveyer->get<Transform>().rotate_facing_clockwise(180);
                EntityHelper::addEntity(conveyer);
            }

            {
                location.y--;
                std::shared_ptr<Furniture> conveyer;
                conveyer.reset(entities::make_grabber(location));
                conveyer->get<Transform>().rotate_facing_clockwise();
                EntityHelper::addEntity(conveyer);

                std::shared_ptr<Pill> item;
                item.reset(new Pill(location, Color{255, 15, 240, 255}));
                ItemHelper::addItem(item);
                conveyer->get<CanHoldItem>().update(item);
            }
        };
        generate_conveyer_test();

        auto generate_tables = [this]() {
            {
                const auto location = get_rand_walkable();

                std::shared_ptr<Furniture> table;
                table.reset(entities::make_table(location));
                EntityHelper::addEntity(table);

                std::shared_ptr<Pill> item;
                item.reset(new Pill(location, Color{255, 15, 240, 255}));
                ItemHelper::addItem(item);
                table->get<CanHoldItem>().update(item);
            }

            {
                const auto location = get_rand_walkable();

                std::shared_ptr<Furniture> table;
                table.reset(entities::make_table(location));
                EntityHelper::addEntity(table);

                std::shared_ptr<PillBottle> item;
                item.reset(new PillBottle(location, RED));
                ItemHelper::addItem(item);
                table->get<CanHoldItem>().update(item);
            }
        };

        const auto generate_medicine_cabinet = [this]() {
            std::shared_ptr<Furniture> medicineCab;
            const auto location = get_rand_walkable();
            medicineCab.reset(entities::make_medicine_cabinet(location));
            EntityHelper::addEntity(medicineCab);
        };

        const auto generate_bag_box = [this]() {
            std::shared_ptr<Furniture> bagbox;
            const auto location = get_rand_walkable();
            bagbox.reset(entities::make_bagbox(location));
            EntityHelper::addEntity(bagbox);
        };

        const auto generate_register = [this]() {
            std::shared_ptr<Furniture> reg;
            const auto location = get_rand_walkable();
            reg.reset(entities::make_register(location));
            EntityHelper::addEntity(reg);
        };

        // TODO replace with a CustomerSpawner eventually
        const auto generate_customer = []() {
            {
                const auto location = vec2{-10 * TILESIZE, -10 * TILESIZE};
                std::shared_ptr<Entity> customer;
                customer.reset(make_customer(vec::to3(location), false));
                EntityHelper::addEntity(dynamic_pointer_cast<Entity>(customer));
            }

            if (1) {
                const auto location = vec2{-11 * TILESIZE, -10 * TILESIZE};
                std::shared_ptr<Entity> customer;
                customer.reset(make_customer(vec::to3(location)));
                EntityHelper::addEntity(dynamic_pointer_cast<Entity>(customer));
            }

            if (0) {
                const auto location = vec2{-12 * TILESIZE, -10 * TILESIZE};
                std::shared_ptr<Entity> customer;
                customer.reset(make_customer(vec::to3(location)));
                EntityHelper::addEntity(dynamic_pointer_cast<Entity>(customer));
            }
        };

        auto generate_test = [this]() {
            for (int i = 0; i < 5; i++) {
                const auto location = get_rand_walkable();
                std::shared_ptr<Furniture> conveyer;

                if (i == 0)
                    conveyer.reset(entities::make_conveyer(location));
                else
                    conveyer.reset(entities::make_grabber(location));

                EntityHelper::addEntity(conveyer);
            }
        };

        generate_and_insert_walls(this->seed);
        generate_tables();
        generate_tables();
        generate_medicine_cabinet();
        generate_bag_box();

        generate_register();
        generate_customer();
        generate_test();
        */

        EntityHelper::invalidatePathCache();
    }

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<LevelInfo>{});
        s.value8b(hashed_seed);
        // TODO these arent serializable...
        // s.object(generator);
        // s.object(dist);
    }
};
