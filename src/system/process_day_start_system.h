#pragma once

#include "../ah.h"
#include "../building_locations.h"
#include "../components/can_hold_item.h"
#include "../components/can_order_drink.h"
#include "../components/can_pathfind.h"
#include "../components/can_perform_job.h"
#include "../components/has_day_night_timer.h"
#include "../components/has_progression.h"
#include "../components/is_item.h"
#include "../components/is_item_container.h"
#include "../components/is_round_settings_manager.h"
#include "../components/is_solid.h"
#include "../components/is_spawner.h"
#include "../components/is_store_spawned.h"
#include "../components/is_toilet.h"
#include "../components/responds_to_day_night.h"
#include "../engine/bitset_utils.h"
#include "../engine/log.h"
#include "../engine/statemanager.h"
#include "../entity_helper.h"
#include "../entity_query.h"
#include "magic_enum/magic_enum.hpp"
#include "system_manager.h"

namespace system_manager {

inline void delete_floating_items_when_leaving_inround(Entity& entity) {
    if (entity.is_missing<IsItem>()) return;

    const IsItem& ii = entity.get<IsItem>();

    // Its being held by something so we'll get it in the function below
    if (ii.is_held()) return;

    // Skip the mop buddy for now
    if (check_type(entity, EntityType::MopBuddy)) return;

    // mark it for cleanup
    entity.cleanup = true;
}

inline void delete_held_items_when_leaving_inround(Entity& entity) {
    if (entity.is_missing<CanHoldItem>()) return;

    CanHoldItem& canHold = entity.get<CanHoldItem>();
    if (canHold.empty()) return;

    // Mark it as deletable
    // let go of the item
    Item& item = canHold.item();
    item.cleanup = true;
    canHold.update(nullptr, -1);
}

inline void reset_max_gen_when_after_deletion(Entity& entity) {
    if (entity.is_missing<CanHoldItem>()) return;
    if (entity.is_missing<IsItemContainer>()) return;

    const CanHoldItem& canHold = entity.get<CanHoldItem>();
    // If something wasnt deleted, then just ignore it for now
    if (canHold.is_holding_item()) return;

    entity.get<IsItemContainer>().reset_generations();
}

inline void tell_customers_to_leave(Entity& entity) {
    if (!check_type(entity, EntityType::Customer)) return;

    // Force leaving job
    entity.get<CanPerformJob>().current = JobType::Leaving;
    entity.removeComponentIfExists<CanPathfind>();
    entity.addComponent<CanPathfind>().set_parent(&entity);
}

// TODO :DESIGN: do we actually want to do this?
inline void reset_toilet_when_leaving_inround(Entity& entity) {
    if (entity.is_missing<IsToilet>()) return;

    IsToilet& istoilet = entity.get<IsToilet>();
    istoilet.reset();
}

inline void reset_customer_spawner_when_leaving_inround(Entity& entity) {
    if (entity.is_missing<IsSpawner>()) return;
    entity.get<IsSpawner>().reset_num_spawned();
}

inline void update_new_max_customers(Entity& entity, float) {
    if (entity.is_missing<HasProgression>()) return;

    Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
    const IsRoundSettingsManager& irsm = sophie.get<IsRoundSettingsManager>();

    const HasDayNightTimer& hasTimer = sophie.get<HasDayNightTimer>();
    const int day_count = hasTimer.days_passed();

    if (check_type(entity, EntityType::CustomerSpawner)) {
        float customer_spawn_multiplier =
            irsm.get<float>(ConfigKey::CustomerSpawnMultiplier);
        float round_length = irsm.get<float>(ConfigKey::RoundLength);

        const int new_total =
            (int) fmax(2.f,  // force 2 at the beginning of the game
                             //
                       day_count * 2.f * customer_spawn_multiplier);

        // the div by 2 is so that everyone is spawned by half day, so
        // theres time for you to make their drinks and them to pay before
        // they are forced to leave
        const float time_between = (round_length / new_total) / 2.f;

        log_info("Updating progression, setting new spawn total to {}",
                 new_total);
        entity
            .get<IsSpawner>()  //
            .set_total(new_total)
            .set_time_between(time_between);
        return;
    }
}

namespace day_night {

inline void on_night_ended(Entity& entity) {
    if (entity.is_missing<RespondsToDayNight>()) return;
    entity.get<RespondsToDayNight>().call_night_ended();
}

inline void on_day_started(Entity& entity) {
    if (entity.is_missing<RespondsToDayNight>()) return;
    entity.get<RespondsToDayNight>().call_day_started();
}

}  // namespace day_night

namespace store {

inline void generate_store_options() {
    // Figure out what kinds of things we can spawn generally
    // - what is spawnable?
    // - are they capped by progression? (alcohol / fruits for sure
    // right?) choose a couple options to spawn
    // - how many?
    // spawn them
    // - use the place machine thing

    OptEntity spawn_area = EntityHelper::getMatchingFloorMarker(
        IsFloorMarker::Type::Store_SpawnArea);

    Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
    const IsProgressionManager& ipp = sophie.get<IsProgressionManager>();
    const EntityTypeSet& unlocked = ipp.enabled_entity_types();
    IsRoundSettingsManager& irsm = sophie.get<IsRoundSettingsManager>();

    int num_to_spawn = irsm.get<int>(ConfigKey::NumStoreSpawns);

    // NOTE: areas expand outward so as2() refers to the center
    // so we have to go back half the size
    Transform& area_transform = spawn_area->get<Transform>();
    vec2 area_origin = area_transform.as2();
    float half_width = area_transform.sizex() / 2.f;
    float half_height = area_transform.sizez() / 2.f;
    float reset_x = area_origin.x - half_width;
    float reset_y = area_origin.y - half_height;

    vec2 spawn_position = vec2{reset_x, reset_y};

    while (num_to_spawn) {
        int entity_type_id = bitset_utils::get_random_enabled_bit(unlocked);
        EntityType etype = magic_enum::enum_value<EntityType>(entity_type_id);
        if (get_price_for_entity_type(etype) <= 0) continue;

        log_info("generate_store_options: random: {}",
                 magic_enum::enum_name<EntityType>(etype));

        auto& entity = EntityHelper::createEntity();
        entity.addComponent<IsStoreSpawned>();
        bool success = convert_to_type(etype, entity, spawn_position);
        if (success) {
            num_to_spawn--;
        } else {
            entity.cleanup = true;
        }

        spawn_position.x += 2;

        if (spawn_position.x > (area_origin.x + half_width)) {
            spawn_position.x = reset_x;
            spawn_position.y += 2;
        } else if (spawn_position.y > (area_origin.y + half_height)) {
            reset_x += 1;
            spawn_position.x = reset_x;
            spawn_position.y = reset_y;
        }
    }
}

inline void open_store_doors() {
    for (RefEntity door :
         EntityQuery()
             .whereType(EntityType::Door)
             .whereInside(STORE_BUILDING.min(), STORE_BUILDING.max())
             .gen()) {
        door.get().removeComponentIfExists<IsSolid>();
    }
}

}  // namespace store

namespace upgrade {

inline void on_round_finished(Entity& entity, float) {
    if (entity.is_missing<IsRoundSettingsManager>()) return;
    IsRoundSettingsManager& irsm = entity.get<IsRoundSettingsManager>();

    irsm.ran_for_hour = -1;
    irsm.config.this_hours_mods.clear();
}

}  // namespace upgrade

// TODO eventually split this into a separate system for each day start logic
// System that processes day start logic when needs_to_process_change is true
// and is_daytime is true
struct ProcessDayStartSystem : public afterhours::System<> {
    virtual bool should_run(const float) override {
        if (!GameState::get().is_game_like()) return false;
        try {
            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            const HasDayNightTimer& timer = sophie.get<HasDayNightTimer>();
            return timer.needs_to_process_change && timer.is_daytime();
        } catch (...) {
            return false;
        }
    }

    virtual void once(float dt) override {
        log_info("DAY STARTED");

        // Store setup
        store::generate_store_options();
        store::open_store_doors();

        // Process day start logic for all entities
        SystemManager::get().for_each_old([dt](Entity& entity) {
            day_night::on_night_ended(entity);
            day_night::on_day_started(entity);

            delete_floating_items_when_leaving_inround(entity);

            // TODO these we likely no longer need to do
            if (false) {
                delete_held_items_when_leaving_inround(entity);

                // I dont think we want to do this since we arent
                // deleting anything anymore maybe there might be a
                // problem with spawning a simple syurup in the
                // store??
                reset_max_gen_when_after_deletion(entity);
            }

            tell_customers_to_leave(entity);

            // TODO we want you to always have to clean >:)
            // but we need some way of having the customers
            // finishe the last job they were doing (as long as it
            // isnt ordering) and then leaving, otherwise the toilet
            // is stuck "inuse" when its really not
            reset_toilet_when_leaving_inround(entity);

            reset_customer_spawner_when_leaving_inround(entity);

            // Handle updating all the things that rely on
            // progression
            update_new_max_customers(entity, dt);

            upgrade::on_round_finished(entity, dt);
        });
    }
};

}  // namespace system_manager
