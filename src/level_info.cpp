
#include "level_info.h"

#include "camera.h"
#include "components/is_progression_manager.h"
#include "components/is_trigger_area.h"
#include "engine/globals.h"
#include "engine/texture_library.h"
#include "map_generation.h"
#include "network/server.h"
#include "recipe_library.h"
#include "system/system_manager.h"
#include "vec_util.h"
#include "wave_collapse.h"

namespace wfc {
extern Rectangle SPAWN_AREA;
extern Rectangle TRASH_AREA;
}  // namespace wfc

void LevelInfo::update_seed(const std::string& s) {
    // TODO implement this
    // randomizer.get<HasName>().update(server->get_map_SERVER_ONLY()->seed);

    log_info("level info update seed {}", s);
    seed = s;
    hashed_seed = hashString(seed);
    generator = make_engine(hashed_seed);
    // TODO leaving at 1 because we dont have a door to block the entrance
    dist = std::uniform_int_distribution<>(1, MAX_MAP_SIZE - 1);

    was_generated = false;
}

void LevelInfo::onUpdate(Entities& players, float dt) {
    TRACY_ZONE_SCOPED;
    SystemManager::get().update_all_entities(players, dt);
}

void LevelInfo::onDraw(float dt) const {
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

void LevelInfo::onDrawUI(float dt) {
    SystemManager::get().render_ui(entities, dt);
}

void LevelInfo::grab_things() {
    {
        this->entities.clear();
        EntityHelper::cleanup();
        this->entities = EntityHelper::get_entities();
        num_entities = this->entities.size();
    }
}

void LevelInfo::generate_lobby_map() {
    {
        auto& entity = EntityHelper::createPermanentEntity();
        furniture::make_character_switcher(
            entity, vec::to2(lobby_origin) + vec2{5.f, 5.f});
    }

    {
        auto& entity = EntityHelper::createPermanentEntity();
        furniture::make_map_randomizer(entity,
                                       vec::to2(lobby_origin) + vec2{7.f, 7.f});
    }

    {
        auto& entity = EntityHelper::createPermanentEntity();
        furniture::make_trigger_area(
            entity, lobby_origin + vec3{5, TILESIZE / -2.f, 10}, 8, 3,
            IsTriggerArea::Lobby_PlayGame);
    }
}

void LevelInfo::generate_progression_map() {
    {
        auto& entity = EntityHelper::createPermanentEntity();
        furniture::make_trigger_area(
            entity, progression_origin + vec3{-5, TILESIZE / -2.f, -10}, 8, 3,
            IsTriggerArea::Progression_Option1);
    }

    {
        auto& entity = EntityHelper::createPermanentEntity();
        furniture::make_trigger_area(
            entity, progression_origin + vec3{5, TILESIZE / -2.f, -10}, 8, 3,
            IsTriggerArea::Progression_Option2);
    }
}

void LevelInfo::generate_default_seed() {
    generation::helper helper(EXAMPLE_MAP);
    helper.generate();
    helper.validate();
    EntityHelper::invalidatePathCache();
}

vec2 generate_in_game_map_wfc(const std::string& seed) {
    // int rows = gen_rand(MIN_MAP_SIZE, MAX_MAP_SIZE);
    // int cols = gen_rand(MIN_MAP_SIZE, MAX_MAP_SIZE);
    // int rows = 5;
    // int cols = 5;

    wfc::WaveCollapse wc(static_cast<unsigned int>(hashString(seed)));
    wc.run();

    generation::helper helper(wc.get_lines());
    vec2 max_location = helper.generate();
    helper.validate();
    EntityHelper::invalidatePathCache();

    return max_location;
}

void LevelInfo::add_outside_triggers(vec2 origin) {
    {
        auto& entity = EntityHelper::createEntity();
        vec3 position = {
            origin.x + wfc::SPAWN_AREA.x,
            TILESIZE / -2.f,
            origin.y + wfc::SPAWN_AREA.y,
        };
        furniture::make_trigger_area(entity, position, wfc::SPAWN_AREA.width,
                                     wfc::SPAWN_AREA.height,
                                     IsTriggerArea::Lobby_PlayGame);
    }

    {
        auto& entity = EntityHelper::createEntity();
        vec3 position = {
            origin.x + wfc::TRASH_AREA.x,
            TILESIZE / -2.f,
            origin.y + wfc::TRASH_AREA.y,
        };
        furniture::make_trigger_area(entity, position, wfc::TRASH_AREA.width,
                                     wfc::TRASH_AREA.height,
                                     IsTriggerArea::Progression_Option1);
    }
}

void LevelInfo::generate_in_game_map() {
    if (seed == "default_seed") {
        generate_default_seed();
        add_outside_triggers({0, 0});
        return;
    }
    vec2 mx = generate_in_game_map_wfc(seed);
    add_outside_triggers(mx);

    return;

    std::vector<std::string> lines;

    int rows = gen_rand(MIN_MAP_SIZE, MAX_MAP_SIZE);
    int cols = gen_rand(MIN_MAP_SIZE, MAX_MAP_SIZE);

    const auto _is_inside = [lines](int i, int j) -> bool {
        if (i < 0 || j < 0 || i >= (int) lines.size() ||
            j >= (int) lines[i].size())
            return false;
        return true;
    };

    const auto get_char = [lines, _is_inside](int i, int j) -> char {
        if (_is_inside(i, j)) return lines[i][j];
        return '.';
    };

    const auto _is_empty = [get_char, lines](int i, int j) -> bool {
        // TODO probably should allow caller to set default on fail
        return get_char(i, j) == '.';
    };

    const auto _get_random_empty = [_is_empty, rows, cols,
                                    this]() -> std::pair<int, int> {
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
            !_is_empty(x, y));
        return {x, y};
    };

    const auto _get_empty_neighbor = [_is_empty, this](
                                         int i, int j) -> std::pair<int, int> {
        auto ns = vec::get_neighbors_i(i, j);
        std::shuffle(std::begin(ns), std::end(ns), generator);

        for (auto n : ns) {
            if (_is_empty(n.first, n.second)) {
                return n;
            }
        }
        return ns[0];
    };

    const auto _get_valid_register_location = [_is_empty, rows, cols,
                                               this]() -> std::pair<int, int> {
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
        lines[3][cols + 1] = generation::FAST_FORWARD;
    }

    {
        auto [x, y] = _get_random_empty();
        lines[x][y] = generation::MOP_HOLDER;
    }

    int num_tables = std::min(rows, cols);
    for (int i = 0; i < num_tables; i++) {
        auto [x, y] = _get_random_empty();
        do {
            // place table
            lines[x][y] = generation::TABLE;
            // move over one spot
            auto p = _get_empty_neighbor(x, y);
            x = p.first;
            y = p.second;
            // was it empty? okay place and go back
        } while (_is_inside(x, y) && _is_empty(x, y));
    }

    {
        // using soda machine here to enforce that we have these two next to
        // eachother
        auto [x, y] = _get_valid_register_location();
        lines[x][y] = generation::SODA_MACHINE;
        lines[x][y + 1] = generation::CUPBOARD;
    }

    // place register last so that we guarantee it remains valid
    // ie the place infront of it isnt full
    {
        auto [x, y] = _get_valid_register_location();
        lines[x][y] = generation::REGISTER;
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

auto LevelInfo::get_rand_walkable() {
    vec2 location;
    do {
        location = vec2{dist(generator) * TILESIZE, dist(generator) * TILESIZE};
    } while (!EntityHelper::isWalkable(location));
    return location;
}

auto LevelInfo::get_rand_walkable_register() {
    vec2 location;
    do {
        location = vec2{dist(generator) * TILESIZE, dist(generator) * TILESIZE};
    } while (
        !EntityHelper::isWalkable(location) &&
        !EntityHelper::isWalkable(vec2{location.x, location.y + 1 * TILESIZE}));
    return location;
}

void LevelInfo::ensure_generated_map(const std::string& new_seed) {
    if (was_generated) return;
    log_info("generating map with new seed: {}", new_seed);
    seed = new_seed;
    was_generated = true;

    EntityHelper::delete_all_entities();

    generate_lobby_map();
    generate_progression_map();
    generate_in_game_map();
}
