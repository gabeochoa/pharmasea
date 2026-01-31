#pragma once

#include "../../ah.h"
#include "../../components/can_pathfind.h"
#include "../../components/has_ai_target_entity.h"
#include "../../components/has_ai_target_location.h"
#include "../../components/has_waiting_queue.h"
#include "../../components/has_work.h"
#include "../../components/is_ai_controlled.h"
#include "../../engine/statemanager.h"
#include "../../entity_query.h"
#include "../../entity_type.h"
#include "ai_entity_helpers.h"
#include "ai_shared_utilities.h"
#include "ai_tags.h"
#include "ai_targeting.h"

namespace system_manager {

struct AICleanVomitSystem
    : public afterhours::System<
          IsAIControlled, CanPathfind,
          afterhours::tags::None<afterhours::tags::AITag::AITransitionPending,
                                 afterhours::tags::AITag::AINeedsResetting>> {
    bool should_run(const float) override {
        return GameState::get().is_game_like();
    }

    void wander_in_bar(Entity& entity, CanPathfind& pathfind, float dt) {
        HasAITargetLocation& roam = entity.get<HasAITargetLocation>();
        if (!roam.pos.has_value()) {
            roam.pos = system_manager::ai::pick_random_walkable_in_building(
                           entity, BAR_BUILDING)
                           .value_or(entity.get<Transform>().as2());
        }
        bool reached = pathfind.travel_toward(
            roam.pos.value(),
            system_manager::ai::get_speed_for_entity(entity) * dt);
        if (reached) roam.pos.reset();
    }

    void for_each_with(Entity& entity, IsAIControlled& ctrl,
                       CanPathfind& pathfind, float dt) override {
#if !__APPLE__
        if (entity.hasTag(afterhours::tags::AITag::AITransitionPending)) return;
        if (entity.hasTag(afterhours::tags::AITag::AINeedsResetting)) return;
#endif
        if (ctrl.state != IsAIControlled::State::CleanVomit) return;
        if (!ctrl.has_ability(IsAIControlled::AbilityCleanVomit)) return;

        HasAITargetEntity& tgt = entity.get<HasAITargetEntity>();

        if (!system_manager::ai::entity_ref_valid(tgt.entity)) {
            auto other_ais = EntityQuery()
                                 .whereNotID(entity.id)
                                 .whereHasComponent<IsAIControlled>()
                                 .whereLambda([](const Entity& e) {
                                     if (e.is_missing<IsAIControlled>())
                                         return false;
                                     return e.get<IsAIControlled>().has_ability(
                                         IsAIControlled::AbilityCleanVomit);
                                 })
                                 .gen();
            std::set<int> existing_targets;
            for (const Entity& mop : other_ais) {
                if (mop.is_missing<HasAITargetEntity>()) continue;
                const EntityRef& r = mop.get<HasAITargetEntity>().entity;
                if (r.empty()) continue;
                existing_targets.insert(static_cast<int>(r.id));
            }

            bool more_boys_than_vomit =
                existing_targets.size() < other_ais.size();

            OptEntity vomit = EntityQuery()
                                  .whereType(EntityType::Vomit)
                                  .whereLambda([&](const Entity& v) {
                                      if (more_boys_than_vomit) return true;
                                      return !existing_targets.contains(v.id);
                                  })
                                  .orderByDist(entity.get<Transform>().as2())
                                  .gen_first();
            if (!vomit) {
                wander_in_bar(entity, pathfind, dt);
                return;
            }
            tgt.entity.set(vomit.asE());
            entity.removeComponentIfExists<HasAITargetLocation>();
        }

        OptEntity vomit = tgt.entity.resolve();
        if (!vomit) {
            tgt.entity.clear();
            wander_in_bar(entity, pathfind, dt);
            return;
        }

        bool reached = pathfind.travel_toward(
            vomit->get<Transform>().as2(),
            system_manager::ai::get_speed_for_entity(entity) * dt);
        if (!reached) return;

        if (!vomit->has<HasWork>()) return;
        HasWork& vomWork = vomit->get<HasWork>();
        vomWork.call(vomit.asE(), entity, dt);
        if (vomit->cleanup) {
            tgt.entity.clear();
        }
    }
};

}  // namespace system_manager