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

struct OnDayEndedSystem : public afterhours::System<RespondsToDayNight> {
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
        rtdn.call_day_ended();
    }
};

// just in case theres anyone in the queue still, just
// clear it before the customers start coming in
struct ResetRegisterQueueWhenLeavingInRoundSystem
    : public afterhours::System<HasWaitingQueue> {
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

    virtual void for_each_with(Entity&, HasWaitingQueue& hwq, float) override {
        hwq.clear();
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

    inline void move_player_out_of_building_SERVER_ONLY(
        Entity& entity, const Building& building) {
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

        server->send_player_location_packet(client_id, position,
                                            transform.facing,
                                            entity.get<HasName>().name());
    }

    virtual void once(float) override {
        // TODO rewrite this to use entity queries
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
};

struct OnNightStartedSystem : public afterhours::System<RespondsToDayNight> {
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
        rtdn.call_night_started();
    }
};

// - TODO keeps respawning roomba, we should probably
// not do that anymore...just need to clean it up at end
// of day i guess or let him roam??
struct ReleaseMopBuddyAtStartOfDaySystem
    : public afterhours::System<
          afterhours::tags::All<EntityType::MopBuddyHolder>, CanHoldItem> {
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

    virtual void for_each_with(Entity&, CanHoldItem& chi, float) override {
        if (chi.empty()) return;
        chi.item().cleanup = true;
        chi.update(nullptr, -1);
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
