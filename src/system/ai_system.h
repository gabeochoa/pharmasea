
#pragma once

#include "../components/ai_clean_vomit.h"
#include "../components/ai_drinking.h"
#include "../components/ai_use_bathroom.h"
#include "../components/can_order_drink.h"
#include "../components/can_pathfind.h"
#include "../components/can_perform_job.h"
#include "../components/has_base_speed.h"
#include "../components/has_speech_bubble.h"
#include "../components/has_timer.h"
#include "../components/is_bank.h"
#include "../components/is_progression_manager.h"
#include "../components/is_round_settings_manager.h"
#include "../components/is_toilet.h"
#include "../entity.h"
#include "../entity_query.h"
#include "../job.h"

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

inline void next_job(Entity& entity, JobType suggestion) {
    Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
    const IsRoundSettingsManager& irsm = sophie.get<IsRoundSettingsManager>();
    const CanOrderDrink& cod = entity.get<CanOrderDrink>();

    int bladder_size = irsm.get<int>(ConfigKey::BladderSize);
    bool gotta_go = (cod.drinks_in_bladder >= bladder_size);
    if (gotta_go) {
        entity.get<CanPerformJob>().current = JobType::Bathroom;
    }
    entity.get<CanPerformJob>().current = suggestion;
}

inline void process_ai_drinking(Entity& entity, float dt) {
    if (entity.is_missing<AIDrinking>()) return;
    if (entity.is_missing<CanOrderDrink>()) return;

    AIDrinking& aidrinking = entity.get<AIDrinking>();
    aidrinking.pass_time(dt);
    if (!aidrinking.ready()) return;

    CanOrderDrink& cod = entity.get<CanOrderDrink>();
    if (cod.order_state != CanOrderDrink::OrderState::DrinkingNow) {
        return;
    }

    if (!aidrinking.has_available_target()) {
        // TODO choose a better place
        aidrinking.set_target(vec2{0, 0});
        // TODO add a variable for how long people drink
        aidrinking.set_drink_time(1.f);  // randfIn(1.f, 5.f));
    }

    bool reached = entity.get<CanPathfind>().travel_toward(
        aidrinking.pos(), get_speed_for_entity(entity) * dt);
    if (!reached) return;

    bool completed = aidrinking.drink(dt);
    if (!completed) {
        return;
    }

    // Done with my drink, delete it
    CanHoldItem& chi = entity.get<CanHoldItem>();
    chi.item()->cleanup = true;
    chi.update(nullptr, -1);

    // Mark our current order finished
    cod.on_order_finished();
    aidrinking.unset_target();

    //
    // Do we want another drink?
    //

    Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
    bool want_another = cod.num_orders_rem > 0;

    // done drinking
    if (!want_another) {
        // Now we are fully done so lets pay.

        // TODO add a new job type to go pay, for now just pay when they are
        // done drinking, they will still go the register but the pay
        // happens here
        {
            IsBank& bank = sophie.get<IsBank>();
            bank.deposit(cod.tab_cost);
            bank.deposit(cod.tip);

            // i dont think we need to do this, but just in case
            cod.tab_cost = 0;
            cod.tip = 0;
        }

        const HasTimer& hasTimer = sophie.get<HasTimer>();

        std::shared_ptr<Job> jshared;
        jshared.reset(new WaitJob(
            // TODO they go back to the register before leaving becausd
            // start here..
            vec2{0, 0},
            // TODO create a global so they all leave to the same spot
            vec2{GATHER_SPOT, GATHER_SPOT},
            hasTimer.remaining_time_in_round()));
        entity.get<CanPerformJob>().push_onto_queue(jshared);
        cod.order_state = CanOrderDrink::OrderState::DoneDrinking;
        next_job(entity, JobType::Wait);
        return;
    }

    // get next order
    const IsProgressionManager& progressionManager =
        sophie.get<IsProgressionManager>();

    // TODO make a function set_order()
    cod.current_order = progressionManager.get_random_unlocked_drink();
    cod.order_state = CanOrderDrink::OrderState::Ordering;

    std::shared_ptr<Job> jshared;
    jshared.reset(new WaitInQueueJob());
    entity.get<CanPerformJob>().push_onto_queue(jshared);
    next_job(entity, JobType::WaitInQueue);
}

inline void process_ai_clean_vomit(Entity& entity, float dt) {
    if (entity.is_missing<AICleanVomit>()) return;
    AICleanVomit& aiclean = entity.get<AICleanVomit>();

    aiclean.pass_time(dt);
    if (!aiclean.ready()) return;

    if (!aiclean.has_available_target()) {
        OptEntity closest =
            EntityHelper::getClosestOfType(entity, EntityType::Vomit);

        // We couldnt find anything, for now just wait a second
        if (!closest) {
            aiclean.reset();
            return;
        }
        aiclean.set_target(closest->id);
    }

    // We have a target
    OptEntity vomit = EntityHelper::getEntityForID(aiclean.id());

    if (!vomit) {
        aiclean.unset_target();
        return;
    }

    bool reached = entity.get<CanPathfind>().travel_toward(
        vomit->get<Transform>().as2(), get_speed_for_entity(entity) * dt);

    if (!reached) return;

    HasWork& vomWork = vomit->get<HasWork>();
    vomWork.call(vomit.asE(), entity, dt);
    // check if we did it
    if (vomit->cleanup) {
        aiclean.unset_target();
        return;
    }
}

inline void process_ai_use_bathroom(Entity& entity, float dt) {
    if (entity.is_missing<AIUseBathroom>()) return;
    AIUseBathroom& aibathroom = entity.get<AIUseBathroom>();

    Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
    const IsRoundSettingsManager& irsm = sophie.get<IsRoundSettingsManager>();
    const CanOrderDrink& cod = entity.get<CanOrderDrink>();

    aibathroom.pass_time(dt);
    if (!aibathroom.ready()) return;

    int bladder_size = irsm.get<int>(ConfigKey::BladderSize);
    bool gotta_go = (cod.drinks_in_bladder >= bladder_size);
    if (!gotta_go) return;

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
            return;
        }
        aibathroom.set_target(best_target->id);
    }

    // We have a target
    OptEntity opt_toilet = EntityHelper::getEntityForID(aibathroom.id());
    Entity& toilet = opt_toilet.asE();

    bool reached = entity.get<CanPathfind>().travel_toward(
        toilet.get<Transform>().as2(), get_speed_for_entity(entity) * dt);
    if (!reached) return;

    IsToilet& istoilet = toilet.get<IsToilet>();
    bool we_are_using_it = istoilet.is_user(entity.id);

    // Someone else is using it
    if (istoilet.occupied() && !we_are_using_it) {
        // TODO after a couple loops maybe you just go on the floor :(
        aibathroom.reset();
        return;
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

        // TODO move away from it for a second
        (void) entity.get<CanPathfind>().travel_toward(
            vec2{0, 0}, get_speed_for_entity(entity) * dt);
        return;
    }
}

inline void process_(Entity& entity, float dt) {
    if (entity.is_missing<CanPathfind>()) return;

    switch (entity.get<CanPerformJob>().current) {
        case Mopping:
            process_ai_clean_vomit(entity, dt);
            break;
        case Drinking:
            process_ai_drinking(entity, dt);
            break;
        case WaitInQueue:
            process_ai_use_bathroom(entity, dt);
            break;
        case NoJob:
        case Wait:
        case Wandering:
        case EnterStore:
        case WaitInQueueForPickup:
        case Paying:
        case Leaving:
        case Bathroom:
        case MAX_JOB_TYPE:
            break;
    }
}

}  // namespace ai
}  // namespace system_manager
