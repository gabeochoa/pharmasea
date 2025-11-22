
#include "sophie.h"

#include <algorithm>
#include <iterator>
#include <tuple>

#include "../building_locations.h"
#include "../components/can_hold_furniture.h"
#include "../components/collects_customer_feedback.h"
#include "../components/has_day_night_timer.h"
#include "../components/has_waiting_queue.h"
#include "../components/is_progression_manager.h"
#include "../components/is_round_settings_manager.h"
#include "../components/is_store_spawned.h"
#include "../components/is_toilet.h"
#include "../components/transform.h"
#include "../dataclass/ingredient.h"
#include "../engine/assert.h"
#include "../engine/bitset_utils.h"
#include "../engine/pathfinder.h"
#include "../entity_helper.h"
#include "../entity_query.h"
#include "ingredient_helper.h"
#include "magic_enum/magic_enum.hpp"
#include "system_manager.h"

namespace system_manager {

namespace sophie {

void customers_in_store(Entity& entity) {
    // Handle customers finally leaving the store
    // TODO with the others siwtch to something else... customer
    // spawner?
    const auto endpos = vec2{GATHER_SPOT, GATHER_SPOT};

    // TODO :DUPE: used as well for nux checks
    OptEntity any = EntityQuery()
                        .whereType(EntityType::Customer)
                        .whereNotInRange(endpos, TILESIZE * 2.f)
                        .gen_first();

    vec2 pos = any.has_value() ? any->get<Transform>().as2() : vec2{0, 0};

    entity.get<CollectsCustomerFeedback>().write_reason(
        CollectsCustomerFeedback::WaitingReason::CustomersInStore,
        any.has_value(), pos);
}

// Handle some player is holding furniture
void player_holding_furniture(Entity& entity) {
    bool all_empty =
        EQ(SystemManager::get().oldAll).whereIsHoldingAnyFurniture().is_empty();

    entity.get<CollectsCustomerFeedback>().write_reason(
        CollectsCustomerFeedback::WaitingReason::HoldingFurniture, !all_empty,
        // players can figure it out on their own
        {});
}

void bar_not_clean(Entity& entity) {
    // are there any vomit anywhere?
    auto any = EntityQuery().whereType(EntityType::Vomit).gen_first_position();

    // is the toilet clean?
    if (!any.has_value()) {
        any =
            EntityQuery()
                .whereHasComponentAndLambda<IsToilet>(
                    [](const IsToilet& istoilet) {
                        return istoilet.state == IsToilet::State::NeedsCleaning;
                    })
                .gen_first_position();
    }

    vec2 pos = any.has_value() ? vec::to2(any->second) : vec2{0, 0};

    entity.get<CollectsCustomerFeedback>().write_reason(
        CollectsCustomerFeedback::WaitingReason::BarNotClean, any.has_value(),
        pos);
}

void overlapping_furniture(Entity& entity) {
    // We dont have a way to say "in map" ie user-controllable
    // vs outside map so we just have to check everything

    // Right now the map is starting at 00 and at most is  -50,-50 to 50,50

    vec2 range_min = {-50, -50};
    vec2 range_max = {50, 50};

    auto solid_ids =
        EntityQuery()                      //
            .whereHasComponent<IsSolid>()  //
                                           // Skip the soda rope because the
                                           // rope overlapps with itself and we
                                           // are okay with that
            .whereNotType(EntityType::SodaSpout)  //
            .whereInside(range_min, range_max)    //
            // Convert them all to just ids and position
            .gen_positions();

    const auto _hasDuplicates =
        [](const std::vector<std::pair<EntityID, vec3>>& arr) -> EntityID {
        for (auto e1 : arr) {
            for (auto e2 : arr) {
                if (e1.first == e2.first) continue;
                if (e1.second == e2.second) return e1.first;
            }
        }
        return -1;
    };

    EntityID overlapping_id = _hasDuplicates(solid_ids);
    OptEntity overlapping_entity =
        EntityQuery().whereID(overlapping_id).gen_first();

    bool has_overlapping = overlapping_entity.valid();
    std::optional<vec2> pos = has_overlapping
                                  ? overlapping_entity->get<Transform>().as2()
                                  : std::optional<vec2>{};

    entity.get<CollectsCustomerFeedback>().write_reason(
        CollectsCustomerFeedback::WaitingReason::FurnitureOverlapping,
        has_overlapping, pos);
}

void forgot_item_in_spawn_area(Entity& entity) {
    OptEntity spawn_area = EntityHelper::getMatchingFloorMarker(
        IsFloorMarker::Type::Planning_SpawnArea);

    bool has_item_in_spawn_area = false;
    std::optional<vec2> position;

    if (spawn_area) {
        const IsFloorMarker& ifm = spawn_area->get<IsFloorMarker>();
        has_item_in_spawn_area = ifm.has_any_marked();
        if (has_item_in_spawn_area) {
            EntityID firstMarkedID = ifm.marked_ids()[0];
            const OptEntity firstMarked =
                EntityHelper::getEntityForID(firstMarkedID);
            if (firstMarked) {
                position = firstMarked->get<Transform>().as2();
            }
        }
    }

    entity.get<CollectsCustomerFeedback>().write_reason(
        CollectsCustomerFeedback::WaitingReason::ItemInSpawnArea,
        has_item_in_spawn_area, position);
}

void lightweight_map_validation(Entity& entity) {
    // TODO merge with map generation validation?
    // Run lightweight map validation
    // find customer
    auto customer_opt =
        EntityQuery().whereType(EntityType::CustomerSpawner).gen_first();
    // TODO we are validating this now, but we shouldnt have to worry
    // about this in the future
    VALIDATE(customer_opt,
             "map needs to have at least one customer spawn point");
    auto& customer = customer_opt.asE();

    int reg_with_no_pathing = 0;
    int reg_with_bad_spots = 0;
    std::vector<RefEntity> all_registers =
        EntityQuery()
            .whereHasComponent<HasWaitingQueue>()
            .whereInside(BAR_BUILDING.min(), BAR_BUILDING.max())
            .gen();

    if (all_registers.empty()) {
        // because we require the map to spawn one, we know this is due to the
        // bar building area check above
        entity.get<CollectsCustomerFeedback>().write_reason(
            CollectsCustomerFeedback::WaitingReason::RegisterNotInside, true,
            {});
        return;
    }

    const auto _has_blocked_spot = [](const Entity& r) {
        for (int i = 0; i < (int) HasWaitingQueue::max_queue_size; i++) {
            vec2 pos = r.get<Transform>().tile_infront(i + 1);
            if (!EntityHelper::isWalkable(pos)) {
                return true;
            }
        }
        return false;
    };

    std::optional<vec2> non_pathable_reg_location = {};
    std::optional<vec2> bad_spot_reg_location = {};
    for (const Entity& r : all_registers) {
        if (_has_blocked_spot(r)) {
            reg_with_bad_spots++;
            bad_spot_reg_location = r.get<Transform>().as2();
        }

        auto new_path = pathfinder::find_path(
            customer.get<Transform>().as2(),
            r.get<Transform>().tile_directly_infront(),
            std::bind(EntityHelper::isWalkable, std::placeholders::_1));

        if (new_path.empty()) {
            reg_with_no_pathing++;
            non_pathable_reg_location = r.get<Transform>().as2();
        }
    }

    // If every register has a spot that isnt walkable directly, then its
    // probably not pathable anyway so return false
    if (reg_with_bad_spots == (int) all_registers.size()) {
        // TODO maybe we could add a separate error message for this
        entity.get<CollectsCustomerFeedback>().write_reason(
            CollectsCustomerFeedback::WaitingReason::NoPathToRegister, true,
            bad_spot_reg_location);
        return;
    }

    if (reg_with_no_pathing == (int) all_registers.size()) {
        // TODO maybe we could add a separate error message for this
        entity.get<CollectsCustomerFeedback>().write_reason(
            CollectsCustomerFeedback::WaitingReason::NoPathToRegister, true,
            non_pathable_reg_location);
        return;
    }

    entity.get<CollectsCustomerFeedback>().write_reason(
        CollectsCustomerFeedback::WaitingReason::NoPathToRegister, false, {});
}

void deleting_item_needed_for_recipe(Entity& entity) {
    const auto result = [&entity](bool on, std::optional<vec2> position = {}) {
        entity.get<CollectsCustomerFeedback>().write_reason(
            CollectsCustomerFeedback::WaitingReason::DeletingNeededItem, on,
            position);
        return;
    };

    OptEntity trash_area = EntityHelper::getMatchingFloorMarker(
        IsFloorMarker::Type::Planning_TrashArea);

    if (!trash_area.valid()) return result(false);

    const IsFloorMarker& ifm = trash_area->get<IsFloorMarker>();

    // its possible we dont have all the machines required...
    // but theres nothing in the trash
    // in that case lets just say things are okay
    // (usually happens during development)
    // see :DEV_MACHINES:
    if (ifm.num_marked() == 0) return result(false);

    // TODO this is not robust but itll work for a while
    // TODO right now getAll will return store ents but we dont want that for
    // this, the radius is saving us at the moment
    float rad = 25;
    const RefEntities ents = EntityHelper::getAllInRangeFiltered(
        {-1.f * rad, -1.f * rad}, {rad, rad}, [&ifm](const Entity& entity) {
            // Ignore the ones in the trash
            if (ifm.is_marked(entity.id)) return false;
            return true;
        });

    Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
    const IsProgressionManager& progressionManager =
        sophie.get<IsProgressionManager>();

    const auto unlockedIngredients = progressionManager.enabled_ingredients();

    bool has_req_machines = true;

    const auto _hasAtLeastOneNotInTrash =
        [&ifm](const EntityType& type) -> bool {
        const auto trash_ids = ifm.marked_ids();
        // Theoretically this doesnt suit the constraint but
        // right now we only care about trashing them so its
        // okay
        if (trash_ids.empty()) return true;

        // TODO i really want to be able to clone just the query piece
        // but cant because of the unique ptr
        size_t num_total = EntityQuery().whereType(type).gen_count();

        size_t num_in_trash =
            EntityQuery()
                .whereType(type)
                .whereLambda([trash_ids](const Entity& et) -> bool {
                    return util::contains(trash_ids, et.id);
                })
                .gen_count();
        // if you are not throwing away all of them then we good
        return num_total > num_in_trash;
    };

    has_req_machines &= _hasAtLeastOneNotInTrash(EntityType::Cupboard);

    // TODO this one isnt "needed for recipies" but like 90% of the code is
    // the same so using it for now
    has_req_machines &= _hasAtLeastOneNotInTrash(EntityType::Trash);

    // TODO this one isnt "needed for recipies" but like 90% of the code is
    // the same so using it for now
    const IsRoundSettingsManager& irsm = sophie.get<IsRoundSettingsManager>();
    for (const EntityType& et : irsm.config.forever_required) {
        has_req_machines &= _hasAtLeastOneNotInTrash(et);
    };

    // can we make all the recipies with the remaining ents

    const auto hasMachinesForIngredient = [&ents,
                                           &has_req_machines](int index) {
        Ingredient ig = magic_enum::enum_value<Ingredient>(index);
        has_req_machines =
            has_req_machines &&
            IngredientHelper::has_machines_required_for_ingredient(ents, ig);
        return bitset_utils::ForEachFlow::NormalFlow;
        // log_info(
        // "hasMachines {} {}",
        // magic_enum::enum_name<Ingredient>(ig),
        // IngredientHelper::has_machines_required_for_ingredient(ents,
        // ig)
        //
        // );
    };
    bitset_utils::for_each_enabled_bit(unlockedIngredients,
                                       hasMachinesForIngredient);

    const auto get_position_for_reason = [&ifm]() -> std::optional<vec2> {
        const auto trash_ids = ifm.marked_ids();
        if (trash_ids.empty()) return {};
        const OptEntity ent = EntityHelper::getEntityForID(trash_ids[0]);
        if (!ent.valid()) return {};
        return ent->get<Transform>().as2();
    };

    return result(!has_req_machines, get_position_for_reason());
}

void holding_stolen_item(Entity& entity) {
    // Find all players holding Store items and not in the store
    OptEntity player =
        EQ(SystemManager::get().oldAll)
            .whereNotInside(STORE_BUILDING.min(), STORE_BUILDING.max())
            .whereIsHoldingAnyFurnitureThatMatches([](const Entity& furniture) {
                return furniture.has<IsStoreSpawned>();
            })
            .gen_first();

    if (!player) {
        entity.get<CollectsCustomerFeedback>().write_reason(
            CollectsCustomerFeedback::WaitingReason::StoreStealingMachine,
            false, {});
        return;
    }

    auto position = player->get<Transform>().as2();

    entity.get<CollectsCustomerFeedback>().write_reason(
        CollectsCustomerFeedback::WaitingReason::StoreStealingMachine, true,
        position);
}

void garbage_in_store(Entity& entity) {
    // Find if players put non sellables in the store

    auto garbage = EntityQuery()
                       .whereInside(STORE_BUILDING.min(), STORE_BUILDING.max())
                       .whereMissingComponent<IsStoreSpawned>()
                       .whereHasComponent<CanBeHeld>()
                       .gen();

    if (garbage.empty()) {
        entity.get<CollectsCustomerFeedback>().write_reason(
            CollectsCustomerFeedback::WaitingReason::StoreHasGarbage, false,
            {});
        return;
    }

    Entity& garbo = garbage[0];
    auto position = garbo.get<Transform>().as2();

    entity.get<CollectsCustomerFeedback>().write_reason(
        CollectsCustomerFeedback::WaitingReason::StoreHasGarbage, true,
        position);
}

}  // namespace sophie

// TODO this function is 75% of our game update time spent
void update_sophie(Entity& entity, float dt) {
    if (!check_type(entity, EntityType::Sophie)) return;
    if (entity.is_missing<HasDayNightTimer>()) return;

    const auto debug_mode_on =
        GLOBALS.get_or_default<bool>("debug_ui_enabled", false);
    HasDayNightTimer& ht = entity.get<HasDayNightTimer>();
    CollectsCustomerFeedback& feedback = entity.get<CollectsCustomerFeedback>();

    // TODO i dont like that this is copy paste from layers/round_end
    if (SystemManager::get().is_nighttime() && ht.get_current_length() > 0 &&
        !debug_mode_on)
        return;

    if (!feedback.waiting_time_pass(dt)) {
        return;
    }

    // doing it this way so that if we wanna make them return bool itll be
    // easy
    typedef std::function<void(Entity&)> WaitingFn;

    std::vector<WaitingFn> fns{
        sophie::customers_in_store,   //
        sophie::holding_stolen_item,  //
        sophie::garbage_in_store,     //
        // sophie::player_holding_furniture,         //
        sophie::bar_not_clean,                    //
        sophie::overlapping_furniture,            //
        sophie::forgot_item_in_spawn_area,        //
        sophie::deleting_item_needed_for_recipe,  //
        sophie::lightweight_map_validation,
    };

    for (const auto& fn : fns) {
        fn(entity);
    }
}
}  // namespace system_manager
