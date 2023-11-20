
#pragma once

#include "../components/ai_clean_vomit.h"
#include "../components/ai_use_bathroom.h"
#include "../components/can_order_drink.h"
#include "../components/can_pathfind.h"
#include "../components/has_base_speed.h"
#include "../components/is_round_settings_manager.h"
#include "../components/is_toilet.h"
#include "../entity.h"
#include "../entity_query.h"

namespace system_manager {
namespace ai {

inline float get_speed_for_entity(Entity& entity) {
    float base_speed = entity.get<HasBaseSpeed>().speed();

    // TODO Does OrderDrink hold stagger information?
    // or should it live in another component?
    if (entity.has<CanOrderDrink>()) {
        const CanOrderDrink& cha = entity.get<CanOrderDrink>();
        // float speed_multiplier = cha.ailment().speed_multiplier();
        // if (speed_multiplier != 0) base_speed *= speed_multiplier;

        // TODO Turning off stagger; couple problems
        // - configuration is hard to reason about and mess with
        // - i really want it to cause them to move more, maybe we place
        // this in the path generation or something isntead?
        //
        // float stagger_multiplier = cha.ailment().stagger(); if
        // (stagger_multiplier != 0) base_speed *= stagger_multiplier;

        int denom = randIn(1, std::max(1, cha.num_alcoholic_drinks_had));
        base_speed *= 1.f / denom;

        base_speed = fmaxf(1.f, base_speed);
        // log_info("multiplier {} {} {}", speed_multiplier,
        // stagger_multiplier, base_speed);
    }
    return base_speed;
}

inline bool process_ai_clean_vomit(Entity& entity, float dt) {
    if (entity.is_missing<AICleanVomit>()) return false;
    AICleanVomit& aiclean = entity.get<AICleanVomit>();

    aiclean.pass_time(dt);
    if (!aiclean.ready()) return false;

    if (!aiclean.has_available_target()) {
        OptEntity closest =
            EntityHelper::getClosestOfType(entity, EntityType::Vomit);

        // We couldnt find anything, for now just wait a second
        if (!closest) {
            aiclean.reset();
            return false;
        }
        aiclean.set_target(closest->id);
    }

    // We have a target
    OptEntity vomit = EntityHelper::getEntityForID(aiclean.id());

    if (!vomit) {
        aiclean.unset_target();
        return false;
    }

    bool reached = entity.get<CanPathfind>().travel_toward(
        vomit->get<Transform>().as2(), get_speed_for_entity(entity) * dt);

    if (!reached) return true;

    HasWork& vomWork = vomit->get<HasWork>();
    vomWork.call(vomit.asE(), entity, dt);
    // check if we did it
    if (vomit->cleanup) {
        aiclean.unset_target();
        return true;
    }
    return true;
}

inline bool process_ai_use_bathroom(Entity& entity, float dt) {
    if (entity.is_missing<AIUseBathroom>()) return false;
    AIUseBathroom& aibathroom = entity.get<AIUseBathroom>();

    aibathroom.pass_time(dt);
    if (!aibathroom.ready()) return false;

    Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
    const IsRoundSettingsManager& irsm = sophie.get<IsRoundSettingsManager>();
    const CanOrderDrink& cod = entity.get<CanOrderDrink>();

    int bladder_size = irsm.get<int>(ConfigKey::BladderSize);
    bool gotta_go = (cod.drinks_in_bladder >= bladder_size);
    if (!gotta_go) return false;

    if (!aibathroom.has_available_target()) {
        std::vector<RefEntity> all_toilets =
            EntityQuery().whereHasComponent<IsToilet>().gen();

        // TODO sort by distance?
        OptEntity best_target = {};
        for (Entity& r : all_toilets) {
            const IsToilet& toilet = r.get<IsToilet>();
            if (toilet.available()) {
                best_target = r;
            }
        }
        // We couldnt find anything, for now just wait a second
        if (!best_target) {
            aibathroom.reset();
            return false;
        }
        aibathroom.set_target(best_target->id);
    }

    // We have a target
    OptEntity opt_toilet = EntityHelper::getEntityForID(aibathroom.id());
    Entity& toilet = opt_toilet.asE();

    bool reached = entity.get<CanPathfind>().travel_toward(
        toilet.get<Transform>().as2(), get_speed_for_entity(entity) * dt);
    if (!reached) return true;

    IsToilet& istoilet = toilet.get<IsToilet>();
    bool we_are_using_it = istoilet.is_user(entity.id);

    // Someone else is using it
    if (istoilet.occupied() && !we_are_using_it) {
        // TODO after a couple loops maybe you just go on the floor :(
        aibathroom.reset();
        return false;
    }

    // we are using it
    if (!we_are_using_it) {
        // ?TODO right now we dont mark occupied until the person gets there
        // obv this is like real life where two people just gotta go
        // at the same time.
        //
        // this means that its possible there is a free toilet on the other side
        // of the map and people are still using this one because they all
        // grabbed at the same time
        //
        // this might be frustrating as a player since you are like "why are
        // they so dumb"
        //
        // instead of the wait above, maybe do a wait and search?

        float piss_timer = irsm.get<float>(ConfigKey::PissTimer);
        aibathroom.set_piss_time(piss_timer);
        istoilet.start_use(entity.id);
    }

    bool completed = aibathroom.piss(dt);
    if (completed) {
        aibathroom.unset_target();
        entity.get<CanOrderDrink>().empty_bladder();
        istoilet.end_use();
        return true;
    }
    return true;
}

using AIFunc = std::function<bool(Entity&, float)>;

inline void process_(Entity& entity, float dt) {
    if (entity.is_missing<CanPathfind>()) return;

    std::array<AIFunc, 2> funcs = {{
        process_ai_clean_vomit,
        process_ai_use_bathroom,
    }};

    for (const auto& fn : funcs) {
        bool focused = fn(entity, dt);
        if (focused) break;
    }
}

}  // namespace ai
}  // namespace system_manager
