#pragma once

#include "../ah.h"
#include "../building_locations.h"
#include "../components/can_hold_item.h"
#include "../components/has_day_night_timer.h"
#include "../components/has_name.h"
#include "../components/has_waiting_queue.h"
#include "../components/is_floor_marker.h"
#include "../components/is_item.h"
#include "../components/is_store_spawned.h"
#include "../components/responds_to_day_night.h"
#include "../components/transform.h"
#include "../engine/log.h"
#include "../engine/statemanager.h"
#include "../entity_helper.h"
#include "../entity_query.h"
#include "../network/server.h"
#include "store_management_helpers.h"
#include "system_manager.h"

namespace system_manager {

inline void move_player_out_of_building_SERVER_ONLY(Entity& entity,
                                                    const Building& building) {
    if (!is_server()) {
        log_warn(
            "you are calling a server only function from a client context, "
            "this is best case a no-op and worst case a visual desync");
    }

    vec3 position = vec::to3(building.vomit_location);
    Transform& transform = entity.get<Transform>();
    transform.update(position);

    network::Server* server = GLOBALS.get_ptr<network::Server>("server");

    int client_id = server->get_client_id_for_entity(entity);
    if (client_id == -1) {
        log_warn("Tried to find a client id for entity but didnt find one");
        return;
    }

    server->send_player_location_packet(client_id, position, transform.facing,
                                        entity.get<HasName>().name());
}

inline void close_buildings_when_night(Entity& entity) {
    // just choosing this since theres only one
    if (!check_type(entity, EntityType::Sophie)) return;

    const std::array<Building, 2> buildings_that_close = {
        PROGRESSION_BUILDING,
        STORE_BUILDING,
    };

    for (const Building& building : buildings_that_close) {
        // Teleport anyone inside a store outside
        SystemManager::get().for_each_old([&](Entity& e) {
            if (!check_type(e, EntityType::Player)) return;
            if (CheckCollisionBoxes(e.get<Transform>().bounds(),
                                    building.bounds)) {
                move_player_out_of_building_SERVER_ONLY(e, building);
            }
        });
    }
}

inline void release_mop_buddy_at_start_of_day(Entity& entity) {
    if (!check_type(entity, EntityType::MopBuddyHolder)) return;

    CanHoldItem& chi = entity.get<CanHoldItem>();
    if (chi.empty()) return;

    // grab yaboi
    Item& item = chi.item();

    // let go of the item
    item.get<IsItem>().set_held_by(EntityType::Unknown, -1);
    chi.update(nullptr, -1);
}

inline void reset_register_queue_when_leaving_inround(Entity& entity) {
    if (entity.is_missing<HasWaitingQueue>()) return;
    HasWaitingQueue& hwq = entity.get<HasWaitingQueue>();
    hwq.clear();
}

namespace day_night {

inline void on_day_ended(Entity& entity) {
    if (entity.is_missing<RespondsToDayNight>()) return;
    entity.get<RespondsToDayNight>().call_day_ended();
}

inline void on_night_started(Entity& entity) {
    if (entity.is_missing<RespondsToDayNight>()) return;
    entity.get<RespondsToDayNight>().call_night_started();
}

}  // namespace day_night

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

struct DeleteTrashWhenLeavingPlanningSystem
    : public afterhours::System<afterhours::tags::All<EntityType::FloorMarker>,
                                IsFloorMarker> {
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

    virtual void for_each_with(Entity&, IsFloorMarker& ifm, float) override {
        if (ifm.type != IsFloorMarker::Planning_TrashArea) return;

        for (size_t i = 0; i < ifm.num_marked(); i++) {
            EntityID id = ifm.marked_ids()[i];
            OptEntity marked_entity = EntityHelper::getEntityForID(id);
            if (!marked_entity) continue;
            marked_entity->cleanup = true;

            // Also delete the held item
            CanHoldItem& markedCHI = marked_entity->get<CanHoldItem>();
            if (!markedCHI.empty()) {
                markedCHI.item().cleanup = true;
                markedCHI.update(nullptr, -1);
            }
        }
    }
};

}  // namespace system_manager
