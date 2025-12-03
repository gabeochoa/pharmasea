#pragma once

#include "../ah.h"
#include "../components/has_day_night_timer.h"
#include "../engine/log.h"
#include "../engine/statemanager.h"
#include "../entity_helper.h"
#include "system_manager.h"

namespace system_manager {

struct CleanUpOldStoreOptionsSystem : public afterhours::System<> {
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

    virtual void once(float) override { store::cleanup_old_store_options(); }
};

struct OnDayEndedSystem : public afterhours::System<> {
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

    virtual void once(float) override {
        SystemManager::get().for_each_old(
            [](Entity& entity) { day_night::on_day_ended(entity); });
    }
};

// just in case theres anyone in the queue still, just
// clear it before the customers start coming in
struct ResetRegisterQueueWhenLeavingInRoundSystem
    : public afterhours::System<> {
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

    virtual void once(float) override {
        SystemManager::get().for_each_old([](Entity& entity) {
            reset_register_queue_when_leaving_inround(entity);
        });
    }
};

struct CloseBuildingsWhenNightSystem : public afterhours::System<> {
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

    virtual void once(float) override {
        SystemManager::get().for_each_old(
            [](Entity& entity) { close_buildings_when_night(entity); });
    }
};

struct OnNightStartedSystem : public afterhours::System<> {
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

    virtual void once(float) override {
        SystemManager::get().for_each_old(
            [](Entity& entity) { day_night::on_night_started(entity); });
    }
};

// - TODO keeps respawning roomba, we should probably
// not do that anymore...just need to clean it up at end
// of day i guess or let him roam??
struct ReleaseMopBuddyAtStartOfDaySystem : public afterhours::System<> {
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

    virtual void once(float) override {
        SystemManager::get().for_each_old(
            [](Entity& entity) { release_mop_buddy_at_start_of_day(entity); });
    }
};

struct DeleteTrashWhenLeavingPlanningSystem : public afterhours::System<> {
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
    virtual void once(float) override {
        SystemManager::get().for_each_old(
            [](Entity& entity) { delete_trash_when_leaving_planning(entity); });
    }
};

}  // namespace system_manager
