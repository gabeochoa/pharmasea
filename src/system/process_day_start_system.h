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
#include "store_management_helpers.h"
#include "system_manager.h"

namespace system_manager {

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

struct GenerateStoreOptionsSystem : public afterhours::System<> {
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
    virtual void once(float) override { store::generate_store_options(); }
};

struct OpenStoreDoorsSystem
    : public afterhours::System<IsSolid,
                                afterhours::tags::All<EntityType::Door>> {
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
    virtual void for_each_with(Entity& entity, IsSolid&, float) override {
        if (!CheckCollisionBoxes(entity.get<Transform>().bounds(),
                                 STORE_BUILDING.bounds))
            return;
        entity.removeComponentIfExists<IsSolid>();
    }
};

struct DeleteFloatingItemsWhenLeavingInRoundSystem
    : public afterhours::System<IsItem> {
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

    virtual void for_each_with(Entity& entity, IsItem& ii, float) override {
        // Its being held by something so we'll get it in the function below
        if (ii.is_held()) return;

        // Skip the mop buddy for now
        if (check_type(entity, EntityType::MopBuddy)) return;

        // mark it for cleanup
        entity.cleanup = true;

        // TODO these we likely no longer need to do
        if (false) {
            delete_held_items_when_leaving_inround(entity);

            // I dont think we want to do this since we arent
            // deleting anything anymore maybe there might be a
            // problem with spawning a simple syurup in the
            // store??
            reset_max_gen_when_after_deletion(entity);
        }
    }
};

struct TellCustomersToLeaveSystem
    : public afterhours::System<afterhours::tags::All<EntityType::Customer>,
                                CanPerformJob> {
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
    virtual void for_each_with(Entity& entity, CanPerformJob& cpj,
                               float) override {
        cpj.current = JobType::Leaving;
        entity.removeComponentIfExists<CanPathfind>();
        entity.addComponent<CanPathfind>().set_parent(&entity);
    }
};

// TODO :DESIGN: do we actually want to do this?
struct ResetToiletWhenLeavingInRoundSystem
    : public afterhours::System<IsToilet> {
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
    virtual void for_each_with(Entity&, IsToilet& istoilet, float) override {
        // TODO we want you to always have to clean >:)
        // but we need some way of having the customers
        // finishe the last job they were doing (as long as it
        // isnt ordering) and then leaving, otherwise the toilet
        // is stuck "inuse" when its really not
        istoilet.reset();
    }
};

struct ResetCustomerSpawnerWhenLeavingInRoundSystem
    : public afterhours::System<IsSpawner> {
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
    virtual void for_each_with(Entity&, IsSpawner& isspawner, float) override {
        isspawner.reset_num_spawned();
    }
};

struct UpdateNewMaxCustomersSystem
    : public afterhours::System<
          HasProgression, IsSpawner,
          afterhours::tags::All<EntityType::CustomerSpawner>> {
    IsRoundSettingsManager* irsm;
    HasDayNightTimer* hasTimer;

    virtual bool should_run(const float) override {
        if (!GameState::get().is_game_like()) return false;

        Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
        irsm = &sophie.get<IsRoundSettingsManager>();
        hasTimer = &sophie.get<HasDayNightTimer>();
        try {
            return hasTimer->needs_to_process_change && hasTimer->is_daytime();
        } catch (...) {
            return false;
        }
    }

    void once(float) override {
        Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
        irsm = &sophie.get<IsRoundSettingsManager>();
        hasTimer = &sophie.get<HasDayNightTimer>();
    }

    virtual void for_each_with(Entity&, HasProgression&, IsSpawner& isspawner,
                               float) override {
        const int day_count = hasTimer->days_passed();
        float customer_spawn_multiplier =
            irsm->get<float>(ConfigKey::CustomerSpawnMultiplier);
        float round_length = irsm->get<float>(ConfigKey::RoundLength);

        const int new_total =
            (int) fmax(2.f,  // force 2 at the beginning of the game
                       day_count * 2.f * customer_spawn_multiplier);

        // the div by 2 is so that everyone is spawned by half day, so
        // theres time for you to make their drinks and them to pay before
        // they are forced to leave
        const float time_between = (round_length / new_total) / 2.f;

        log_info("Updating progression, setting new spawn total to {}",
                 new_total);
        isspawner.set_total(new_total).set_time_between(time_between);
    }
};

struct OnNightEndedTriggerSystem
    : public afterhours::System<RespondsToDayNight> {
    virtual bool should_run(const float) override {
        if (!GameState::get().is_game_like()) return false;
        try {
            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            const HasDayNightTimer& timer = sophie.get<HasDayNightTimer>();
            return timer.needs_to_process_change && timer.is_nighttime();
        } catch (...) {
            return false;
        }
    }
    virtual void for_each_with(Entity&, RespondsToDayNight& rtdn,
                               float) override {
        rtdn.call_night_ended();
    }
};

struct OnDayStartedTriggerSystem
    : public afterhours::System<RespondsToDayNight> {
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
    virtual void for_each_with(Entity&, RespondsToDayNight& rtdn,
                               float) override {
        rtdn.call_day_started();
    }
};

struct OnRoundFinishedTriggerSystem
    : public afterhours::System<IsRoundSettingsManager> {
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
    virtual void for_each_with(Entity&, IsRoundSettingsManager& irsm,
                               float) override {
        irsm.ran_for_hour = -1;
        irsm.config.this_hours_mods.clear();
    }
};

}  // namespace system_manager
