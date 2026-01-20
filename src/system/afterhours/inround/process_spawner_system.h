#pragma once

#include "../../../ah.h"
#include "../../../components/has_day_night_timer.h"
#include "../../../components/is_spawner.h"
#include "../../../components/transform.h"
#include "../../../dataclass/settings.h"
#include "../../../engine/statemanager.h"
#include "../../../entity_helper.h"
#include "../../../entity_makers.h"
#include "../../../entity_query.h"
#include "../../../network/server.h"
#include "../../core/system_manager.h"

namespace system_manager {

struct ProcessSpawnerSystem : public afterhours::System<Transform, IsSpawner> {
    virtual bool should_run(const float) override {
        if (!GameState::get().is_game_like()) return false;
        try {
            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            const HasDayNightTimer& hastimer = sophie.get<HasDayNightTimer>();
            // Don't run during transitions to avoid spawners creating entities
            // before transition logic completes
            if (hastimer.needs_to_process_change) return false;
            return hastimer.is_bar_open();
        } catch (...) {
            return false;
        }
    }
    void for_each_with(Entity& entity, Transform& transform, IsSpawner& spawner,
                       float dt) override {
        vec2 pos = transform.as2();

        bool is_time_to_spawn = spawner.pass_time(dt);
        if (!is_time_to_spawn) return;

        SpawnInfo info{
            .location = pos,
            .is_first_this_round = (spawner.get_num_spawned() == 0),
        };

        // If there is a validation function check that first
        bool can_spawn_here_and_now = spawner.validate(entity, info);
        if (!can_spawn_here_and_now) return;

        bool should_prev_dupes = spawner.prevent_dupes();
        if (should_prev_dupes) {
            for (const Entity& e : EQ().whereInRange(pos, TILESIZE).gen()) {
                if (e.id == entity.id) continue;

                // Other than invalid and Us, is there anything else there?
                // log_info(
                // "was ready to spawn but then there was someone there
                // already");
                return;
            }
        }

        auto& new_ent = EntityHelper::createEntity();
        spawner.spawn(new_ent, info);
        spawner.post_spawn_reset();

        if (spawner.has_spawn_sound()) {
            network::Server::play_sound(pos, spawner.get_spawn_sound());
        }
    }
};

}  // namespace system_manager