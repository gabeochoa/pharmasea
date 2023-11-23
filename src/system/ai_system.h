
#pragma once

#include "../components/ai_clean_vomit.h"
#include "../components/ai_drinking.h"
#include "../components/ai_use_bathroom.h"
#include "../components/ai_wait_in_queue.h"
#include "../components/can_order_drink.h"
#include "../components/can_pathfind.h"
#include "../components/can_perform_job.h"
#include "../components/has_base_speed.h"
#include "../components/has_patience.h"
#include "../components/has_speech_bubble.h"
#include "../components/has_timer.h"
#include "../components/has_waiting_queue.h"
#include "../components/is_bank.h"
#include "../components/is_progression_manager.h"
#include "../components/is_round_settings_manager.h"
#include "../components/is_toilet.h"
#include "../entity.h"
#include "../entity_query.h"
#include "../job.h"

namespace system_manager {
namespace ai {

inline bool WIQ_can_move_up(const Entity& reg, const Entity& customer) {
    VALIDATE(reg.has<HasWaitingQueue>(),
             "Trying to can-move-up for entity which doesnt "
             "have a waiting queue ");
    return reg.get<HasWaitingQueue>().matching_id(customer.id, 0);
}

inline int WIQ_position_in_line(const Entity& reg, const Entity& customer) {
    // VALIDATE(customer, "entity passed to position-in-line should not be
    // null"); VALIDATE(reg, "entity passed to position-in-line should not be
    // null");
    VALIDATE(
        reg.has<HasWaitingQueue>(),
        "Trying to pos-in-line for entity which doesnt have a waiting queue ");
    const HasWaitingQueue& hwq = reg.get<HasWaitingQueue>();
    return hwq.has_matching_person(customer.id);
}

inline vec2 WIQ_add_to_queue_and_get_position(Entity& reg,
                                              const Entity& customer) {
    VALIDATE(reg.has<HasWaitingQueue>(),
             "Trying to get-next-queue-pos for entity which doesnt have a "
             "waiting queue ");
    // VALIDATE(customer, "entity passed to register queue should not be null");

    int next_position = reg.get<HasWaitingQueue>()
                            .add_customer(customer)  //
                            .get_next_pos();

    // the place the customers stand is 1 tile infront of the register
    return reg.get<Transform>().tile_infront((next_position + 1));
}

inline void WIQ_leave_line(Entity& reg, const Entity& customer) {
    // VALIDATE(customer, "entity passed to leave-line should not be null");
    // VALIDATE(reg, "register passed to leave-line should not be null");
    VALIDATE(
        reg.has<HasWaitingQueue>(),
        "Trying to leave-line for entity which doesnt have a waiting queue ");

    int pos = WIQ_position_in_line(reg, customer);
    if (pos == -1) return;

    reg.get<HasWaitingQueue>().erase(pos);
}

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
    if (entity.has<AIUseBathroom>()) {
        Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
        const IsRoundSettingsManager& irsm =
            sophie.get<IsRoundSettingsManager>();
        const CanOrderDrink& cod = entity.get<CanOrderDrink>();

        int bladder_size = irsm.get<int>(ConfigKey::BladderSize);
        bool gotta_go = (cod.drinks_in_bladder >= bladder_size);
        if (gotta_go) {
            entity.get<CanPerformJob>().current = JobType::Bathroom;
            entity.get<AIUseBathroom>().next_job = suggestion;
            return;
        }
    }
    entity.get<CanPerformJob>().current = suggestion;
}

inline void process_ai_waitinqueue(Entity& entity, float dt) {
    if (entity.is_missing<AIWaitInQueue>()) return;
    if (entity.is_missing<CanOrderDrink>()) return;

    AIWaitInQueue& aiwait = entity.get<AIWaitInQueue>();

    if (!aiwait.has_available_target()) {
        std::vector<RefEntity> all_registers =
            EntityQuery().whereHasComponent<HasWaitingQueue>().gen();

        // Find the register with the least people on it
        OptEntity best_target = {};
        int best_pos = -1;
        for (Entity& r : all_registers) {
            const HasWaitingQueue& hwq = r.get<HasWaitingQueue>();
            if (hwq.is_full()) continue;
            int rpos = hwq.get_next_pos();

            // Check to see if we can path to that spot

            // TODO causing no valid register to be found
            // auto end = r.get<Transform>().tile_infront(rpos);
            // auto new_path = bfs::find_path(
            // entity.get<Transform>().as2(), end,
            // std::bind(EntityHelper::isWalkable, std::placeholders::_1));
            // if (new_path.empty()) continue;

            if (best_pos == -1 || rpos < best_pos) {
                best_target = r;
                best_pos = rpos;
            }
        }
        if (!best_target) {
            aiwait.reset();
            return;
        }

        aiwait.set_target(best_target->id);
        aiwait.position =
            WIQ_add_to_queue_and_get_position(best_target.asE(), entity);

        return;
    }

    bool reached = entity.get<CanPathfind>().travel_toward(
        aiwait.position, get_speed_for_entity(entity) * dt);
    if (!reached) return;

    aiwait.pass_time(dt);
    if (!aiwait.ready()) return;

    OptEntity opt_reg = EntityHelper::getEntityForID(aiwait.id());
    if (!opt_reg) {
        log_warn("got an invalid register");
        return;
    }
    Entity& reg = opt_reg.asE();
    entity.get<Transform>().turn_to_face_pos(reg.get<Transform>().as2());

    int spot_in_line = WIQ_position_in_line(reg, entity);
    if (spot_in_line != 0) {
        // Waiting in line :)

        // TODO We didnt move so just wait a bit before trying again

        if (!WIQ_can_move_up(reg, entity)) {
            // We cant move so just wait a bit before trying again
            log_trace("im just going to wait a bit longer");

            // Add the current job to the queue,
            // and then add the waiting job

            aiwait.reset();
            return;
        }
        // otherwise walk up one spot
        aiwait.position = reg.get<Transform>().tile_infront(spot_in_line);
        return;
    } else {
        aiwait.position = reg.get<Transform>().tile_infront(1);
    }

    // Now we should be at the front of the line

    // we are at the front so turn it on
    {
        // TODO this logic likely should move to system
        // TODO safer way to do it?
        entity.get<HasSpeechBubble>().on();
        entity.get<HasPatience>().enable();
    }

    CanOrderDrink& canOrderDrink = entity.get<CanOrderDrink>();
    VALIDATE(canOrderDrink.has_order(), "I should have an order");

    CanHoldItem& regCHI = reg.get<CanHoldItem>();

    if (regCHI.empty()) {
        log_trace("my drink isnt ready yet");
        aiwait.reset();
        return;
    }

    std::shared_ptr<Item> drink = reg.get<CanHoldItem>().item();
    if (!drink || !check_if_drink(*drink)) {
        log_info("this isnt a drink");
        aiwait.reset();
        return;
    }

    log_info("i got **A** drink ");
    // TODO how many ingredients have to be correct?
    // as people get more drunk they should care less and less
    //
    Drink orderdDrink = canOrderDrink.order();
    bool all_ingredients_match =
        drink->get<IsDrink>().matches_drink(orderdDrink);

    // For debug, if we have this set, just assume it was correct
    bool skip_ing_match =
        GLOBALS.get_or_default<bool>("skip_ingredient_match", false);
    if (skip_ing_match) all_ingredients_match = true;

    if (!all_ingredients_match) {
        log_info("this isnt what i ordered");
        aiwait.reset();
        return;
    }

    // I'm relatively happy with my drink

    const Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
    const IsRoundSettingsManager& irsm = sophie.get<IsRoundSettingsManager>();

    // mark how much we are paying for this drink
    // + how much we will tip
    {
        float cost_multiplier = irsm.get<float>(ConfigKey::DrinkCostMultiplier);
        int price = static_cast<int>(
            cost_multiplier *
            get_base_price_for_drink(canOrderDrink.current_order));
        canOrderDrink.tab_cost += price;

        const HasPatience& hasPatience = entity.get<HasPatience>();
        int tip = (int) fmax(0, ceil(price * 0.8f * hasPatience.pct()));
        canOrderDrink.tip += tip;
    }

    CanHoldItem& ourCHI = entity.get<CanHoldItem>();
    ourCHI.update(regCHI.item(), entity.id);
    regCHI.update(nullptr, -1);

    log_info("got it");
    WIQ_leave_line(reg, entity);

    // TODO Should move to system
    {
        canOrderDrink.order_state = CanOrderDrink::OrderState::DrinkingNow;
        entity.get<HasSpeechBubble>().off();
        entity.get<HasPatience>().disable();
        entity.get<HasPatience>().reset();
    }
    // Now that we are done and got our item, time to leave the store
    log_info("leaving line");

    entity.get<AIWaitInQueue>().unset_target();
    entity.get<CanPerformJob>().current = JobType::Drinking;
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
        next_job(entity, JobType::Leaving);
        cod.order_state = CanOrderDrink::OrderState::DoneDrinking;
        return;
    }

    // get next order
    const IsProgressionManager& progressionManager =
        sophie.get<IsProgressionManager>();

    // TODO make a function set_order()
    cod.current_order = progressionManager.get_random_unlocked_drink();

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

        entity.get<CanPerformJob>().current = aibathroom.next_job;
        return;
    }
}

inline void process_ai_leaving(Entity& entity, float dt) {
    (void) entity.get<CanPathfind>().travel_toward(
        vec2{GATHER_SPOT, GATHER_SPOT}, get_speed_for_entity(entity) * dt);
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
        case Bathroom:
            process_ai_use_bathroom(entity, dt);
            break;
        case WaitInQueue:
            process_ai_waitinqueue(entity, dt);
            break;
        case Leaving:
            process_ai_leaving(entity, dt);
            break;
        case NoJob:
        case Wait:
        case Wandering:
        case EnterStore:
        case WaitInQueueForPickup:
        case Paying:
        case MAX_JOB_TYPE:
            break;
    }
}

}  // namespace ai
}  // namespace system_manager
