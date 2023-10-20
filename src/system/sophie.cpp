
#include "sophie.h"

#include "../components/can_hold_furniture.h"
#include "../components/has_timer.h"
#include "../components/has_waiting_queue.h"
#include "../components/is_progression_manager.h"
#include "../components/transform.h"
#include "../dataclass/ingredient.h"
#include "../engine/assert.h"
#include "../engine/bitset_utils.h"
#include "../engine/pathfinder.h"
#include "../entity_helper.h"
#include "ingredient_helper.h"
#include "system_manager.h"

namespace system_manager {

namespace sophie {
void customers_in_store(Entity& entity) {
    // Handle customers finally leaving the store
    // TODO with the others siwtch to something else... customer
    // spawner?
    const auto endpos = vec2{GATHER_SPOT, GATHER_SPOT};

    bool all_gone = true;
    std::optional<vec2> pos = {};
    for (const Entity& e : EntityHelper::getAllWithType(EntityType::Customer)) {
        if (vec::distance(e.get<Transform>().as2(), endpos) > TILESIZE * 2.f) {
            all_gone = false;
            pos = e.get<Transform>().as2();
            break;
        }
    }
    entity.get<HasTimer>().write_reason(
        HasTimer::WaitingReason::CustomersInStore, !all_gone, pos);
}

// Handle some player is holding furniture
void player_holding_furniture(Entity& entity) {
    bool all_empty = true;
    // TODO i want to to do it this way: but players are not in
    // entities, so its not possible
    //
    // auto players =
    // EntityHelper::getAllWithComponent<CanHoldFurniture>();

    // TODO need support for 'break' to use for_each_old
    for (std::shared_ptr<Entity>& e : SystemManager::get().oldAll) {
        if (!e) continue;
        if (e->is_missing<CanHoldFurniture>()) continue;
        if (e->get<CanHoldFurniture>().is_holding_furniture()) {
            all_empty = false;
            break;
        }
    }
    entity.get<HasTimer>().write_reason(
        HasTimer::WaitingReason::HoldingFurniture, !all_empty,
        // players can figure it out on their own
        {});
}

void bar_not_clean(Entity& entity) {
    const auto vomits = EntityHelper::getAllWithType(EntityType::Vomit);
    bool has_vomit = !(vomits.empty());

    std::optional<vec2> vom_position =
        has_vomit ? (vomits[0].get().get<Transform>().as2())
                  : std::optional<vec2>{};

    entity.get<HasTimer>().write_reason(HasTimer::WaitingReason::BarNotClean,
                                        has_vomit, vom_position);
}

void overlapping_furniture(Entity& entity) {
    // We dont have a way to say "in map" ie user-controllable
    // vs outside map so we just have to check everything

    // Right now the map is starting at 00 and at most is  -50,-50 to 50,50

    OptEntity overlapping_entity =
        EntityHelper::getOverlappingSolidEntityInRange(
            {-50, -50}, {50, 50}, [](const Entity& entity) {
                // Skip the soda rope because the rope overlapps with itself
                // and we are okay with that
                if (check_type(entity, EntityType::SodaSpout)) return false;
                return true;
            });
    bool has_overlapping = overlapping_entity.valid();
    std::optional<vec2> pos = has_overlapping
                                  ? overlapping_entity->get<Transform>().as2()
                                  : std::optional<vec2>{};

    entity.get<HasTimer>().write_reason(
        HasTimer::WaitingReason::FurnitureOverlapping, has_overlapping, pos);
}

void forgot_item_in_spawn_area(Entity& entity) {
    OptEntity spawn_area =
        EntityHelper::getFirstMatching([](const Entity& entity) {
            if (entity.is_missing<IsFloorMarker>()) return false;
            const IsFloorMarker& fm = entity.get<IsFloorMarker>();
            return fm.type == IsFloorMarker::Type::Planning_SpawnArea;
        });

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

    entity.get<HasTimer>().write_reason(
        HasTimer::WaitingReason::ItemInSpawnArea, has_item_in_spawn_area,
        position);
}

void lightweight_map_validation(Entity& entity) {
    // TODO merge with map generation validation?
    // Run lightweight map validation
    // find customer
    auto customer_opt =
        EntityHelper::getFirstMatching([](const Entity& e) -> bool {
            return check_type(e, EntityType::CustomerSpawner);
        });
    // TODO we are validating this now, but we shouldnt have to worry
    // about this in the future
    VALIDATE(customer_opt,
             "map needs to have at least one customer spawn point");
    auto& customer = customer_opt.asE();

    int reg_with_no_pathing = 0;
    int reg_with_bad_spots = 0;
    std::vector<RefEntity> all_registers =
        EntityHelper::getAllWithComponent<HasWaitingQueue>();

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
            // TODO need a better way to do this
            // 0 makes sense but is the position of the entity, when its
            // infront?
            r.get<Transform>().tile_infront(1),
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
        entity.get<HasTimer>().write_reason(
            HasTimer::WaitingReason::NoPathToRegister, true,
            bad_spot_reg_location);
        return;
    }

    if (reg_with_no_pathing == (int) all_registers.size()) {
        // TODO maybe we could add a separate error message for this
        entity.get<HasTimer>().write_reason(
            HasTimer::WaitingReason::NoPathToRegister, true,
            non_pathable_reg_location);
        return;
    }

    entity.get<HasTimer>().write_reason(
        HasTimer::WaitingReason::NoPathToRegister, false, {});
}

void deleting_item_needed_for_recipe(Entity& entity) {
    const auto result = [&entity](bool on, std::optional<vec2> position = {}) {
        entity.get<HasTimer>().write_reason(
            HasTimer::WaitingReason::DeletingNeededItem, on, position);
        return;
    };

    OptEntity trash_area =
        EntityHelper::getFirstMatching([](const Entity& entity) {
            if (entity.is_missing<IsFloorMarker>()) return false;
            const IsFloorMarker& fm = entity.get<IsFloorMarker>();
            return fm.type == IsFloorMarker::Type::Planning_TrashArea;
        });

    if (!trash_area.valid()) return result(false);

    const IsFloorMarker& ifm = trash_area->get<IsFloorMarker>();

    // TODO this is not robust but itll work for a while
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

        RefEntities cups = EntityHelper::getAllWithType(type);
        size_t num_in_trash = 0;
        for (const auto& cup : cups) {
            if (util::contains(trash_ids, cup.get().id)) num_in_trash++;
        }

        // if you are not throwing away all of them then we good
        return cups.size() > num_in_trash;
    };

    has_req_machines &= _hasAtLeastOneNotInTrash(EntityType::Cupboard);

    // TODO this one isnt "needed for recipies" but like 90% of the code is the
    // same so using it for now
    has_req_machines &= _hasAtLeastOneNotInTrash(EntityType::Trash);

    // can we make all the recipies with the remaining ents

    const auto hasMachinesForIngredient = [&ents,
                                           &has_req_machines](int index) {
        Ingredient ig = magic_enum::enum_value<Ingredient>(index);
        has_req_machines =
            has_req_machines &&
            IngredientHelper::has_machines_required_for_ingredient(ents, ig);
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

}  // namespace sophie

// TODO this function is 75% of our game update time spent
void update_sophie(Entity& entity, float) {
    if (entity.is_missing<HasTimer>()) return;

    const auto debug_mode_on =
        GLOBALS.get_or_default<bool>("debug_ui_enabled", false);
    const HasTimer& ht = entity.get<HasTimer>();

    // TODO i dont like that this is copy paste from layers/round_end
    if (GameState::get().is_not(game::State::Planning) &&
        ht.currentRoundTime > 0 && !debug_mode_on)
        return;

    // doing it this way so that if we wanna make them return bool itll be
    // easy
    typedef std::function<void(Entity&)> WaitingFn;

    std::vector<WaitingFn> fns{
        sophie::customers_in_store,               //
        sophie::player_holding_furniture,         //
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
