
#include "level_info.h"

#include "building_locations.h"
#include "camera.h"
#include "components/can_change_settings_interactively.h"
#include "components/can_hold_furniture.h"
#include "components/is_bank.h"
#include "components/is_floor_marker.h"
#include "components/is_free_in_store.h"
#include "components/is_progression_manager.h"
#include "components/is_round_settings_manager.h"
#include "components/is_store_spawned.h"
#include "components/is_trigger_area.h"
#include "components/transform.h"
#include "dataclass/ingredient.h"
#include "engine/bitset_utils.h"
#include "engine/globals.h"
#include "engine/random_engine.h"
#include "engine/texture_library.h"
#include "entity_helper.h"
#include "entity_makers.h"
#include "entity_query.h"
#include "entity_type.h"
#include "map_generation.h"
#include "recipe_library.h"
#include "simple.h"
#include "strings.h"
#include "system/system_manager.h"
#include "vec_util.h"
#include "wave_collapse.h"

vec3 lobby_origin = LOBBY_BUILDING.to3();
vec3 progression_origin = PROGRESSION_BUILDING.to3();
vec3 model_test_origin = MODEL_TEST_BUILDING.to3();
vec3 store_origin = STORE_BUILDING.to3();

namespace wfc {
extern Rectangle SPAWN_AREA;
extern Rectangle TRASH_AREA;
}  // namespace wfc

void LevelInfo::update_seed(const std::string& s) {
    log_info("level info update seed {}", s);
    seed = s;
    RandomEngine::set_seed(seed);

    was_generated = false;
}

// Cannot be const since they will be updated in the called function
void LevelInfo::onUpdate(const Entities& players, float dt) {
    TRACY_ZONE_SCOPED;
    SystemManager::get().update_all_entities(players, dt);
}

void LevelInfo::onDraw(float dt) const {
    SystemManager::get().render_entities(entities, dt);
}

void LevelInfo::onDrawUI(float dt) {
    SystemManager::get().render_ui(entities, dt);
}

void generate_walls_for_building(const Building& building) {
    std::vector<RefEntity> walls;
    walls.reserve((int) (building.area.width + building.area.height) * 2);

    for (int i = 0; i < (int) building.area.width; i++) {
        // top
        {
            auto& entity = EntityHelper::createEntity();
            convert_to_type(
                EntityType::Wall, entity,
                vec2{building.area.x, building.area.y} + vec2{i * 1.f, 0});
            walls.push_back(entity);
        }
        // bottom
        {
            auto& entity = EntityHelper::createEntity();
            convert_to_type(EntityType::Wall, entity,
                            vec2{building.area.x, building.area.y} +
                                vec2{i * 1.f, building.area.height - 1});
            walls.push_back(entity);
        }
    }

    for (int j = 1; j < (int) building.area.height - 1; j++) {
        // left
        {
            auto& entity = EntityHelper::createEntity();
            convert_to_type(
                EntityType::Wall, entity,
                vec2{building.area.x, building.area.y} + vec2{0, j * 1.f});
            walls.push_back(entity);
        }
        // right
        {
            auto& entity = EntityHelper::createEntity();
            convert_to_type(EntityType::Wall, entity,
                            vec2{building.area.x, building.area.y} +
                                vec2{building.area.width - 1, j * 1.f});
            walls.push_back(entity);
        }
    }

    bool skip = false;
    for (auto& door_pos : building.doors) {
        for (RefEntity entityref : walls) {
            // we already deleted one wall,
            if (skip) continue;

            Entity& entity = entityref;
            if (entity.cleanup) continue;
            const Transform& transform = entity.get<Transform>();
            vec2 pos = transform.as2();
            if (vec::distance(pos, door_pos) >= 1.f) continue;
            entity.cleanup = true;
        }

        skip = false;

        // place door in that spot
        {
            auto& entity = EntityHelper::createEntity();
            convert_to_type(EntityType::Door, entity, door_pos);
        }
    }
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
        furniture::make_interactive_settings_changet(
            entity, vec::to2(lobby_origin) + vec2{6.f, -2.f},
            CanChangeSettingsInteractively::ToggleIsTutorial);
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
                        items::make_drink(item, vec::to3(location));

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
                                return bitset_utils::ForEachFlow::NormalFlow;
                            });

                        canHold.update(EntityHelper::getEntityAsSharedPtr(item),
                                       entity.id);
                    }

                    break;

                    case ModelTestMapInfo::Blender: {
                        CanHoldItem& canHold = entity.get<CanHoldItem>();
                        // create item
                        Entity& item = EntityHelper::createEntity();
                        items::make_juice(item, vec::to3(location),
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

    int num_items_spawned = 0;

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

    // TODO i was hoping this could enforce any missing
    // but it doesnt seem like it
    std::array<ModelTestMapInfo, magic_enum::enum_count<EntityType>() - 25>
        static_map_info = {{
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
            {EntityType::ChampagneHolder},
            // Add another section for upgrade guys
            {EntityType::Toilet},
            {EntityType::Guitar},
            {
                .et = EntityType::Vomit,
                .spawner_type = ModelTestMapInfo::Some,
            },
            {EntityType::Jukebox},
            {EntityType::HandTruck},
            {EntityType::SodaFountain},
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
        num_items_spawned++;
    }

    _carraige_return();
    _carraige_return();
    for (size_t i = 0; i < ingredient::AlcoholsInCycle.size(); i++) {
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
        num_items_spawned++;
    }

    int known_missing =
        // describe each +1 please
        0    //
        + 1  // Unknown
        + 3  // x,y,z
        + 1  // RemotePlayer
        + 1  // Player
        + 1  // Customer
        + 1  // CharacterSwitcher
        + 1  // MapRandomizer
        ;

    int shouldve_spawned =
        (magic_enum::enum_count<EntityType>() - known_missing);

    VALIDATE(num_items_spawned >= shouldve_spawned,
             fmt::format(
                 "Model test is missing {} items (spawned {} shouldve been {})",
                 (num_items_spawned - shouldve_spawned), num_items_spawned,
                 shouldve_spawned));

    //
    // {EntityType::Drink},
}

void LevelInfo::generate_progression_map() {
    generate_walls_for_building(PROGRESSION_BUILDING);

    {
        auto& entity = EntityHelper::createPermanentEntity();
        furniture::make_trigger_area(
            entity, progression_origin + vec3{-5, TILESIZE / -2.f, 0}, 8, 3,
            IsTriggerArea::Progression_Option1);
        entity.get<IsTriggerArea>().set_required_entrants_type(
            IsTriggerArea::EntrantsRequired::AllInBuilding,
            PROGRESSION_BUILDING);
    }

    {
        auto& entity = EntityHelper::createPermanentEntity();
        furniture::make_trigger_area(
            entity, progression_origin + vec3{5, TILESIZE / -2.f, 0}, 8, 3,
            IsTriggerArea::Progression_Option2);
        entity.get<IsTriggerArea>().set_required_entrants_type(
            IsTriggerArea::EntrantsRequired::AllInBuilding,
            PROGRESSION_BUILDING);
    }
}

void LevelInfo::generate_store_map() {
    generate_walls_for_building(STORE_BUILDING);

    const float trig_x = -4;
    const float trig_width = 8;
    const float trig_height = 3;

    {
        auto& entity = EntityHelper::createPermanentEntity();
        furniture::make_trigger_area(
            entity, store_origin + vec3{trig_x, TILESIZE / -2.f, 10},
            trig_width, trig_height, IsTriggerArea::Store_BackToPlanning);

        entity.get<IsTriggerArea>()
            .set_validation_fn([](const IsTriggerArea& ita)
                                   -> ValidationResult {
                // should we only run the below when there is at least one
                // person standing on it?
                // TODO right now we only show it when someone is standing,
                // but it does run every frame (i think)

                OptEntity sophie =
                    EntityQuery().whereType(EntityType::Sophie).gen_first();

                if (!sophie.valid())
                    return {false, strings::i18n::InternalError};

                // Do we have enough money?
                const IsBank& bank = sophie->get<IsBank>();
                int balance = bank.balance();
                int cart = bank.cart();
                if (balance < cart)
                    return {false, strings::i18n::StoreNotEnoughCoins};

                // Are all the required machines here?
                OptEntity cart_area =
                    EntityQuery()
                        .whereHasComponent<IsFloorMarker>()
                        .whereLambda([](const Entity& entity) {
                            if (entity.is_missing<IsFloorMarker>())
                                return false;
                            const IsFloorMarker& fm =
                                entity.get<IsFloorMarker>();
                            return fm.type ==
                                   IsFloorMarker::Type::Store_PurchaseArea;
                        })
                        .include_store_entities()
                        .gen_first();
                if (!cart_area.valid())
                    return {false, strings::i18n::InternalError};

                const IsFloorMarker& ifm = cart_area->get<IsFloorMarker>();

                std::vector<RefEntity> ents;
                for (size_t i = 0; i < ifm.num_marked(); i++) {
                    EntityID id = ifm.marked_ids()[i];
                    OptEntity marked_entity = EntityHelper::getEntityForID(id);
                    if (!marked_entity) continue;
                    if (marked_entity->is_missing<IsStoreSpawned>()) continue;
                    ents.push_back(marked_entity.asE());
                }

                if (ents.empty()) {
                    return {false, strings::i18n::StoreCartEmpty};
                }

                // If its free and not marked, then we cant continue
                for (const Entity& ent : ents) {
                    if (ent.is_missing<IsFreeInStore>()) continue;
                    if (!cart_area->get<IsFloorMarker>().is_marked(ent.id)) {
                        return {false, strings::i18n::StoreMissingRequired};
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
                        return {false, strings::i18n::StoreStealingMachine};
                }

                return {true, strings::i18n::Empty};
            })
            .set_required_entrants_type(
                IsTriggerArea::EntrantsRequired::AllInBuilding, STORE_BUILDING);
    }
    {
        auto& entity = EntityHelper::createEntity();
        furniture::make_floor_marker(
            entity, store_origin + vec3{trig_x, TILESIZE / -2.f, 5}, trig_width,
            trig_height, IsFloorMarker::Type::Store_PurchaseArea);
    }
    {
        auto& entity = EntityHelper::createEntity();
        furniture::make_floor_marker(
            entity, store_origin + vec3{trig_x, TILESIZE / -2.f, -5},
            trig_width, trig_height, IsFloorMarker::Type::Store_SpawnArea);
    }

    {
        auto& entity = EntityHelper::createEntity();
        furniture::make_floor_marker(
            entity, store_origin + vec3{trig_x, TILESIZE / -2.f, 0}, trig_width,
            trig_height, IsFloorMarker::Type::Store_LockedArea);
    }

    {
        auto& entity = EntityHelper::createPermanentEntity();
        furniture::make_trigger_area(
            entity, store_origin + vec3{trig_x, TILESIZE / -2.f, -10},
            trig_width, trig_height, IsTriggerArea::Store_Reroll);

        entity.get<IsTriggerArea>().update_cooldown_max(2.f).set_validation_fn(
            [](const IsTriggerArea&) -> ValidationResult {
                // TODO should we only run the below when there is at least
                // one person standing on it?

                OptEntity sophie =
                    EntityQuery().whereType(EntityType::Sophie).gen_first();

                // TODO translate these strings .
                if (!sophie.valid())
                    return {false, strings::i18n::InternalError};

                // Do we have enough money?
                const IsBank& bank = sophie->get<IsBank>();
                int balance = bank.balance();

                const IsRoundSettingsManager& isrm =
                    sophie->get<IsRoundSettingsManager>();

                if (balance < isrm.get<int>(ConfigKey::StoreRerollPrice))
                    // TODO more accurate string?
                    return {false, strings::i18n::StoreNotEnoughCoins};

                // TODO only run if everyone is on this?

                return {true, strings::i18n::Empty};
            });
    }
}

void LevelInfo::generate_default_seed() {
    generation::helper helper(EXAMPLE_MAP);
    helper.generate();
    helper.validate();
    EntityHelper::invalidateCaches();
}

vec2 generate_in_game_map_wfc(const std::string&) {
    // int rows = gen_rand(MIN_MAP_SIZE, MAX_MAP_SIZE);
    // int cols = gen_rand(MIN_MAP_SIZE, MAX_MAP_SIZE);
    // int rows = 5;
    // int cols = 5;

    // wfc::WaveCollapse wc(static_cast<unsigned int>(hashString(seed)));
    // wc.run();
    // generation::helper helper(wc.get_lines());

    int rows = 20;
    int cols = 20;
    std::vector<char> chars = something(rows, cols);

    std::vector<std::string> lines;
    std::string tmp;
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            tmp.push_back(chars[i * rows + j]);
        }
        lines.push_back(tmp);
        tmp.clear();
    }

    std::vector<char> required = {{
        generation::CUST_SPAWNER,
        generation::SODA_MACHINE,
        generation::TRASH,
        generation::REGISTER,
        generation::ORIGIN,
        generation::SOPHIE,
        generation::FAST_FORWARD,
        generation::CUPBOARD,

        generation::TABLE,
        generation::TABLE,
        generation::TABLE,
        generation::TABLE,
    }};
    tmp.clear();
    for (auto c : required) {
        tmp.push_back(c);
        if (tmp.size() == (size_t) rows) {
            lines.push_back(tmp);
            tmp.clear();
        }
    }
    lines.push_back(tmp);

    for (const auto& c : lines) {
        std::cout << c;
        std::cout << std::endl;
    }

    generation::helper helper(lines);
    vec2 max_location = helper.generate();
    helper.validate();
    EntityHelper::invalidateCaches();

    log_info("max location {}", max_location);

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

    int rows = RandomEngine::get().get_int(MIN_MAP_SIZE, MAX_MAP_SIZE);
    int cols = RandomEngine::get().get_int(MIN_MAP_SIZE, MAX_MAP_SIZE);

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

    const auto _get_random_empty = [_is_empty, rows,
                                    cols]() -> std::pair<int, int> {
        int tries = 0;
        int x;
        int y;
        do {
            x = RandomEngine::get().get_int(1, rows - 1);
            y = RandomEngine::get().get_int(1, cols - 1);
            if (tries++ > 100) {
                x = 0;
                y = 0;
                break;
            }
        } while (  //
            !_is_empty(x, y));
        return {x, y};
    };

    const auto _get_empty_neighbor = [_is_empty](int i,
                                                 int j) -> std::pair<int, int> {
        auto ns = vec::get_neighbors_i(i, j);
        std::shuffle(std::begin(ns), std::end(ns), RandomEngine::generator());

        for (auto n : ns) {
            if (_is_empty(n.first, n.second)) {
                return n;
            }
        }
        return ns[0];
    };

    const auto _get_valid_register_location = [_is_empty, rows,
                                               cols]() -> std::pair<int, int> {
        int tries = 0;
        int x;
        int y;
        do {
            x = RandomEngine::get().get_int(1, rows - 1);
            y = RandomEngine::get().get_int(1, cols - 1);
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
        int y = RandomEngine::get().get_int(1, rows - 1);
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
    EntityHelper::invalidateCaches();
}

auto LevelInfo::get_rand_walkable() {
    vec2 location;
    do {
        location = vec2{RandomEngine::get().get_float(1, MAX_MAP_SIZE - 1),
                        RandomEngine::get().get_float(1, MAX_MAP_SIZE - 1)};
    } while (!EntityHelper::isWalkable(location));
    return location;
}

auto LevelInfo::get_rand_walkable_register() {
    vec2 location;
    do {
        location = vec2{RandomEngine::get().get_float(1, MAX_MAP_SIZE - 1),
                        RandomEngine::get().get_float(1, MAX_MAP_SIZE - 1)};
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
