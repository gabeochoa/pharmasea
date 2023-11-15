
#include "level_info.h"

#include "camera.h"
#include "components/is_floor_marker.h"
#include "components/is_free_in_store.h"
#include "components/is_progression_manager.h"
#include "components/is_trigger_area.h"
#include "dataclass/ingredient.h"
#include "engine/bitset_utils.h"
#include "engine/globals.h"
#include "engine/texture_library.h"
#include "entity_helper.h"
#include "entity_makers.h"
#include "entity_type.h"
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

// Cannot be const since they will be updated in the called function
void LevelInfo::onUpdate(Entities& players, float dt) {
    TRACY_ZONE_SCOPED;
    SystemManager::get().update_all_entities(players, dt);
}

void LevelInfo::onDraw(float dt) const {
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
            entity, vec::to2(lobby_origin) + vec2{4.f, -2.f});
    }

    {
        auto& entity = EntityHelper::createPermanentEntity();
        furniture::make_map_randomizer(
            entity, vec::to2(lobby_origin) + vec2{-4.f, -2.f});
    }

    {
        auto& entity = EntityHelper::createPermanentEntity();
        furniture::make_trigger_area(
            entity, lobby_origin + vec3{0, TILESIZE / -2.f, -5}, 8, 3,
            IsTriggerArea::Lobby_PlayGame);
    }

    {
        auto& entity = EntityHelper::createPermanentEntity();
        furniture::make_trigger_area(
            entity, lobby_origin + vec3{0, TILESIZE / -2.f, 10}, 8, 3,
            IsTriggerArea::Lobby_ModelTest);
    }

    // We explicitly spawn this in the bar because its not actually a lobby item
    // its just in the lobby so that we dont delete it when we regenerate the
    // map
    {
        auto& entity = EntityHelper::createPermanentEntity();
        convert_to_type(EntityType::Face, entity, vec2{0.f, 0.f});
    }
}

struct ModelTestMapInfo {
    EntityType et;

    enum custom_spawner_type {
        None,
        Some,
        Indexable,
        Blender,
        Drink,
    } spawner_type = None;

    std::optional<int> index;
    std::optional<Ingredient> fruit_type;
    std::optional<int> drink;
};

void LevelInfo::generate_model_test_map() {
    {
        auto& entity = EntityHelper::createPermanentEntity();
        furniture::make_trigger_area(
            entity, model_test_origin + vec3{-5, TILESIZE / -2.f, 5}, 8, 3,
            // TODO when you leave we MUST delete these items because
            //      otherwise the pipes will connect to eachother and get messed
            //      up
            IsTriggerArea::ModelTest_BackToLobby);
    }

    const auto custom_spawner = [](const ModelTestMapInfo& mtmi, Entity& entity,
                                   vec2 location) {
        switch (mtmi.et) {
            case EntityType::Table: {
                furniture::make_table(entity, location);

                switch (mtmi.spawner_type) {
                    case ModelTestMapInfo::None:
                    case ModelTestMapInfo::Some:
                    case ModelTestMapInfo::Indexable:
                        break;
                    case ModelTestMapInfo::Drink: {
                        CanHoldItem& canHold = entity.get<CanHoldItem>();
                        // create item
                        Entity& item = EntityHelper::createEntity();
                        items::make_drink(item, location);

                        const auto igs =
                            RecipeLibrary::get()
                                .get(std::string(magic_enum::enum_name<Drink>(
                                    static_cast<Drink>(mtmi.drink.value()))))
                                .ingredients;
                        bitset_utils::for_each_enabled_bit(
                            igs, [&item](size_t index) {
                                Ingredient ig =
                                    magic_enum::enum_value<Ingredient>(index);
                                item.get<IsDrink>().add_ingredient(ig);
                            });

                        canHold.update(EntityHelper::getEntityAsSharedPtr(item),
                                       entity.id);
                    }

                    break;

                    case ModelTestMapInfo::Blender: {
                        CanHoldItem& canHold = entity.get<CanHoldItem>();
                        // create item
                        Entity& item = EntityHelper::createEntity();
                        items::make_juice(item, location,
                                          mtmi.fruit_type.value());
                        canHold.update(EntityHelper::getEntityAsSharedPtr(item),
                                       entity.id);
                    } break;
                }

            }

            break;
            case EntityType::FruitBasket:
                furniture::make_fruit_basket(entity, location,
                                             mtmi.index.value());
                break;
            case EntityType::SingleAlcohol:
                furniture::make_single_alcohol(entity, location,
                                               mtmi.index.value());
                break;
            case EntityType::Vomit:
                furniture::make_vomit(entity, SpawnInfo{location, true});
                break;
            default:
                log_warn(
                    "you are generating something in model test map, that "
                    "requires a custom spawn function but you forgot to set it "
                    "{}",
                    mtmi.et);
        }
        return;
    };

    vec2 start_location = vec::to2(model_test_origin) + vec2{-10, -5};
    float x = 0;
    float y = 0;
    float xspace = 2.f;
    float yspace = 2.f;
    float room_width = 15;

    const auto _process_single_mtmi = [&](const ModelTestMapInfo& mtmi) {
        Entity& entity = EntityHelper::createEntity();
        vec2 location = start_location + vec2{x, y};

        if (mtmi.spawner_type != ModelTestMapInfo::None) {
            custom_spawner(mtmi, entity, location);
        } else {
            convert_to_type(mtmi.et, entity, location);
        }

        x += xspace;

        if (x > room_width) {
            x = 0;
            y -= yspace;
        }
    };

    // TODO base the count hereVVVV on the actual number of entity types
    // so that we know we should consider adding it
    std::array<ModelTestMapInfo, 23> static_map_info = {{
        //
        {EntityType::Cupboard},
        {EntityType::Table},
        {EntityType::Conveyer},
        {EntityType::Grabber},
        {EntityType::Register},
        {EntityType::Blender},
        {EntityType::SodaMachine},
        {EntityType::Cupboard},
        {EntityType::Squirter},
        {EntityType::FilteredGrabber},
        {EntityType::PnumaticPipe},
        {EntityType::MopHolder},
        {EntityType::MopBuddyHolder},
        {EntityType::SimpleSyrupHolder},
        {EntityType::IceMachine},
        {EntityType::Face},
        {EntityType::Trash},
        {EntityType::FastForward},
        {EntityType::DraftTap},
        {EntityType::AlcoholCabinet},
        // Add another section for upgrade guys
        {EntityType::Toilet},
        {EntityType::Guitar},
        {
            .et = EntityType::Vomit,
            .spawner_type = ModelTestMapInfo::Some,
        },
    }};

    const auto _carraige_return = [&]() {
        // reset the x and increment height
        // so that the dynamic ones are separate for now
        // makes it easier to read the names
        x = 0;
        y -= yspace;
    };

    for (const auto& mtmi : static_map_info) {
        _process_single_mtmi(mtmi);
    }

    _carraige_return();
    _carraige_return();
    for (size_t i = 0; i < ingredient::Alcohols.size(); i++) {
        _process_single_mtmi({.et = EntityType::SingleAlcohol,
                              .spawner_type = ModelTestMapInfo::Indexable,
                              .index = i});
    }

    _carraige_return();
    _carraige_return();
    for (size_t i = 0; i < ingredient::Fruits.size(); i++) {
        _process_single_mtmi({.et = EntityType::FruitBasket,
                              .spawner_type = ModelTestMapInfo::Indexable,
                              .index = i});
    }

    // only a single one here
    _carraige_return();
    for (size_t i = 0; i < ingredient::BlendConvert.size(); i++) {
        _process_single_mtmi(
            {.et = EntityType::Table,
             .spawner_type = ModelTestMapInfo::Blender,
             .fruit_type = ingredient::BlendConvert.key_for_index(i)});
    }

    _carraige_return();
    _carraige_return();

    for (size_t i = 0; i < magic_enum::enum_count<Drink>(); i++) {
        _process_single_mtmi(ModelTestMapInfo{
            .et = EntityType::Table,
            .spawner_type = ModelTestMapInfo::Drink,
            .drink = magic_enum::enum_value<Drink>(i),
        });
    }

    //
    // {EntityType::Drink},
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

void LevelInfo::generate_store_map() {
    {
        auto& entity = EntityHelper::createPermanentEntity();
        furniture::make_trigger_area(
            entity, store_origin + vec3{5, TILESIZE / -2.f, -10}, 8, 3,
            IsTriggerArea::Store_BackToPlanning);

        entity.get<IsTriggerArea>().set_validation_fn(
            [](const IsTriggerArea& ita) -> ValidationResult {
                OptEntity sophie =
                    EntityHelper::getFirstOfType(EntityType::Sophie);

                // TODO translate these strings .
                if (!sophie.valid()) return {false, "Internal Error"};

                // Do we have enough money?
                const IsBank& bank = sophie->get<IsBank>();
                int balance = bank.balance();
                int cart = bank.cart();
                if (balance < cart) return {false, "Not enough coins"};

                // Are all the required machines here?
                OptEntity cart_area =
                    EntityHelper::getFirstMatching([](const Entity& entity) {
                        if (entity.is_missing<IsFloorMarker>()) return false;
                        const IsFloorMarker& fm = entity.get<IsFloorMarker>();
                        return fm.type ==
                               IsFloorMarker::Type::Store_PurchaseArea;
                    });
                if (!cart_area.valid()) return {false, "Internal Error"};

                // TODO :STORE_CLEANUP: instead we should just keep track of
                // the store spawned ones
                float rad = 20;
                const auto ents = EntityHelper::getAllInRange(
                    {STORE_ORIGIN - rad, -1.f * rad},
                    {STORE_ORIGIN + rad, rad});

                // If its free and not marked, then we cant continue
                for (const Entity& ent : ents) {
                    if (ent.is_missing<IsFreeInStore>()) continue;
                    if (!cart_area->get<IsFloorMarker>().is_marked(ent.id)) {
                        return {false, "Missing required machine"};
                    }
                }

                // Only run this one if everyone is on the thing
                if (ita.has_min_matching_entrants()) {
                    bool all_empty = true;
                    for (std::shared_ptr<Entity>& e :
                         SystemManager::get().oldAll) {
                        if (!e) continue;
                        if (e->is_missing<CanHoldFurniture>()) continue;
                        if (e->get<CanHoldFurniture>().is_holding_furniture()) {
                            all_empty = false;
                            break;
                        }
                    }
                    if (!all_empty)
                        return {false, "Please put that machine back"};
                }

                return {true, ""};
            });
    }
    {
        auto& entity = EntityHelper::createEntity();
        furniture::make_floor_marker(
            entity, store_origin + vec3{-5, TILESIZE / -2.f, -10}, 8, 3,
            IsFloorMarker::Type::Store_PurchaseArea);
    }
    {
        auto& entity = EntityHelper::createEntity();
        furniture::make_floor_marker(
            entity, store_origin + vec3{-5, TILESIZE / -2.f, 0}, 8, 3,
            IsFloorMarker::Type::Store_SpawnArea);
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
        furniture::make_floor_marker(entity, position, wfc::SPAWN_AREA.width,
                                     wfc::SPAWN_AREA.height,
                                     IsFloorMarker::Type::Planning_SpawnArea);
    }

    {
        auto& entity = EntityHelper::createEntity();
        vec3 position = {
            origin.x + wfc::TRASH_AREA.x,
            TILESIZE / -2.f,
            origin.y + wfc::TRASH_AREA.y,
        };
        furniture::make_floor_marker(entity, position, wfc::TRASH_AREA.width,
                                     wfc::TRASH_AREA.height,
                                     IsFloorMarker::Type::Planning_TrashArea);
    }
}

void LevelInfo::generate_in_game_map() {
    if (seed == "default_seed") {
        generate_default_seed();
        add_outside_triggers({0, 0});

        // TODO :DEV_MACHINES: for dev add missing machines
        // - for dev purposes sometimes we force enable all drinks
        //   but it causes issues when the machiens dont exist on day one
        //   so we gotta spawn them

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
    generate_store_map();
    generate_in_game_map();
    // We dont call this because the trigger area will call it
    // generate_model_test_map();
}
