
#pragma once

#include "../components/ai_clean_vomit.h"
#include "../components/ai_close_tab.h"
#include "../components/ai_drinking.h"
#include "../components/ai_play_jukebox.h"
#include "../components/ai_use_bathroom.h"
#include "../components/ai_wait_in_queue.h"
#include "../components/can_order_drink.h"
#include "../components/can_pathfind.h"
#include "../components/can_perform_job.h"
#include "../components/has_base_speed.h"
#include "../components/has_last_interacted_customer.h"
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

template<typename T>
inline void reset_job_component(Entity& entity) {
    entity.removeComponent<T>();
    entity.addComponent<T>();
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
        // TODO :DUPE: same as process_ai_paying
        OptEntity best_register =
            EntityQuery()
                .whereType(EntityType::Register)
                .whereHasComponent<HasWaitingQueue>()
                .whereLambda([](const Entity& entity) {
                    // Exclude full registers
                    const HasWaitingQueue& hwq = entity.get<HasWaitingQueue>();
                    if (hwq.is_full()) return false;
                    return true;
                })
                // Find the register with the least people on it
                .orderByLambda([](const Entity& r1, const Entity& r2) {
                    const HasWaitingQueue& hwq1 = r1.get<HasWaitingQueue>();
                    int rpos1 = hwq1.get_next_pos();
                    const HasWaitingQueue& hwq2 = r2.get<HasWaitingQueue>();
                    int rpos2 = hwq2.get_next_pos();
                    return rpos1 < rpos2;
                })
                .gen_first();

        // TODO Check to see if we can path to that spot

        if (!best_register) {
            aiwait.reset();
            return;
        }
        aiwait.set_target(best_register->id);
        aiwait.position =
            WIQ_add_to_queue_and_get_position(best_register.asE(), entity);

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

    const Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
    const IsRoundSettingsManager& irsm = sophie.get<IsRoundSettingsManager>();

    const auto validate_drink_order = [&]() {
        // TODO how many ingredients have to be correct?
        // as people get more drunk they should care less and less
        //
        Drink orderdDrink = canOrderDrink.order();
        bool all_ingredients_match =
            drink->get<IsDrink>().matches_drink(orderdDrink);

        // all good
        if (all_ingredients_match) return true;
        // Otherwise Something was wrong with the drink

        // For debug, if we have this set, just assume it was correct
        if (GLOBALS.get_or_default<bool>("skip_ingredient_match", false)) {
            return true;
        }

        // If you have the mocktail upgrade an an ingredient was wrong,
        // figure out if it was alcohol and if theres only one missing then
        // we are good
        if (irsm.has_upgrade_unlocked(UpgradeClass::Mocktails)) {
            Recipe recipe = RecipeLibrary::get().get(
                std::string(magic_enum::enum_name(orderdDrink)));

            IngredientBitSet orderedDrinkSet = recipe.ingredients;
            IngredientBitSet madeDrinkSet = drink->get<IsDrink>().ing();

            // Imagine we have a Mojito order (Soda, Rum, LimeJ, SimpleSyrup for
            // ex) We also support Whiskey. So mojito would look like 11011
            // (whiskey being zero)
            //
            // Two examples, one where we forget the rum (good) and one where we
            // swap rum with whiskey (bad)
            //
            // one, madeDrink 10011 => this is good
            // 11011 xor 10011 => 01000
            // count_bits(01000) => 1 (good)
            // is_bit_alc(01000) => true (good)
            // return true;
            //
            // two madeDrink 10111 => this is bad
            // 11011 xor 10111 => 01100
            // count_bits(01100) => 2 (bad)
            // is_bit_alc(01100) => true for both (?mixed)

            auto xorbits = orderedDrinkSet ^ madeDrinkSet;
            // How many ingredients did we mess up?
            if (xorbits.count() != 1) {
                return false;
            }
            // TODO idk if index is right 100% of the time but lets try it
            Ingredient ig = get_ingredient_from_index(
                bitset_utils::get_first_enabled_bit(xorbits));

            // is the (one) ingredient we messed up an alcoholic one?
            // if so then we are good
            if (ingredient::is_alcohol(ig)) {
                return true;
            }
        }

        return false;
    };

    bool was_drink_correct = validate_drink_order();
    if (!was_drink_correct) {
        log_info("this isnt what i ordered");
        aiwait.reset();
        return;
    }

    // I'm relatively happy with my drink

    // mark how much we are paying for this drink
    // + how much we will tip
    {
        float base_price =
            get_base_price_for_drink(canOrderDrink.current_order);

        float speakeasy_multiplier = 1.f;
        if (irsm.has_upgrade_unlocked(UpgradeClass::Speakeasy)) {
            speakeasy_multiplier +=
                (0.01f * entity.get<CanPathfind>().get_max_length());
        }

        float cost_multiplier = irsm.get<float>(ConfigKey::DrinkCostMultiplier);

        float price_float = cost_multiplier * speakeasy_multiplier * base_price;
        int price = static_cast<int>(price_float);
        canOrderDrink.tab_cost += price;

        log_info(
            "Drink price was {} (base_price({}) * speakeasy({}) * "
            "cost_mult({}) => {})",
            price, base_price, speakeasy_multiplier, cost_multiplier,
            price_float);

        const HasPatience& hasPatience = entity.get<HasPatience>();
        int tip = (int) fmax(0, ceil(price * 0.8f * hasPatience.pct()));
        canOrderDrink.tip += tip;

        // If the drink has any "fancy" ingredients or other multipliers
        canOrderDrink.tip = static_cast<int>(floor(
            canOrderDrink.tip * drink->get<IsDrink>().get_tip_multiplier()));
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

    // Not using next_job because you shouldnt go to the bathroom with your
    // drink in your hand0
    entity.get<CanPerformJob>().current = JobType::Drinking;

    reset_job_component<AIWaitInQueue>(entity);
}

inline void _set_customer_next_order(Entity& entity) {
    Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
    // get next order
    const IsProgressionManager& progressionManager =
        sophie.get<IsProgressionManager>();

    CanOrderDrink& cod = entity.get<CanOrderDrink>();
    // TODO make a function set_order()
    cod.current_order = progressionManager.get_random_unlocked_drink();

    reset_job_component<AIDrinking>(entity);
    next_job(entity, JobType::WaitInQueue);
}

inline void process_ai_drinking(Entity& entity, float dt) {
    if (entity.is_missing<AIDrinking>()) return;
    if (entity.is_missing<CanOrderDrink>()) return;

    Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
    const IsRoundSettingsManager& irsm = sophie.get<IsRoundSettingsManager>();

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
        float drink_time = irsm.get<float>(ConfigKey::MaxDrinkTime);
        drink_time += randfIn(0.1f, 1.f);
        aidrinking.set_drink_time(drink_time);
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

    bool want_another = cod.num_orders_rem > 0;

    // done drinking
    if (!want_another) {
        // Now we are fully done so lets pay.
        next_job(entity, JobType::Paying);
        cod.order_state = CanOrderDrink::OrderState::DoneDrinking;
        return;
    }

    // TODO decide how often customers want to do this...
    // for now lets just say every time
    // TODO only have them go up when someone else changes the song (like as if
    // they are annoyed) for now just random
    bool jukebox_unlocked = irsm.has_upgrade_unlocked(UpgradeClass::Jukebox);
    if (jukebox_unlocked) {
        if (randBool()) {
            next_job(entity, JobType::PlayJukebox);
            return;
        }
    }
    _set_customer_next_order(entity);
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
        OptEntity closest_available_toilet =
            EntityQuery()
                .whereHasComponent<IsToilet>()
                .whereLambda([](const Entity& entity) {
                    const IsToilet& toilet = entity.get<IsToilet>();
                    return toilet.available();
                })
                .orderByDist(entity.get<Transform>().as2())
                .gen_first();

        // We couldnt find anything, for now just wait a second
        if (!closest_available_toilet) {
            aibathroom.reset();
            return;
        }

        aibathroom.set_target(closest_available_toilet->id);
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

        // We specificaly dont use next_job() here because
        // we dont want to infinite loop
        entity.get<CanPerformJob>().current = aibathroom.next_job;

        reset_job_component<AIUseBathroom>(entity);
        return;
    }
}

inline void process_ai_leaving(Entity& entity, float dt) {
    // TODO check the return value here and if true, stop running the
    // pathfinding
    //
    // I noticed this during profiling :)
    //
    (void) entity.get<CanPathfind>().travel_toward(
        vec2{GATHER_SPOT, GATHER_SPOT}, get_speed_for_entity(entity) * dt);
}

// TODO make this take time
inline void process_ai_paying(Entity& entity, float dt) {
    if (entity.is_missing<AICloseTab>()) return;
    if (entity.is_missing<CanOrderDrink>()) return;

    AICloseTab& aiclosetab = entity.get<AICloseTab>();

    if (!aiclosetab.has_available_target()) {
        // TODO :DUPE: same as process_ai_waitinqueue
        OptEntity best_register =
            EntityQuery()
                .whereType(EntityType::Register)
                .whereHasComponent<HasWaitingQueue>()
                .whereLambda([](const Entity& entity) {
                    // Exclude full registers
                    const HasWaitingQueue& hwq = entity.get<HasWaitingQueue>();
                    if (hwq.is_full()) return false;
                    return true;
                })
                // Find the register with the least people on it
                .orderByLambda([](const Entity& r1, const Entity& r2) {
                    const HasWaitingQueue& hwq1 = r1.get<HasWaitingQueue>();
                    int rpos1 = hwq1.get_next_pos();
                    const HasWaitingQueue& hwq2 = r2.get<HasWaitingQueue>();
                    int rpos2 = hwq2.get_next_pos();
                    return rpos1 < rpos2;
                })
                .gen_first();

        if (!best_register) {
            aiclosetab.reset();
            return;
        }

        aiclosetab.set_target(best_register->id);
        aiclosetab.position =
            WIQ_add_to_queue_and_get_position(best_register.asE(), entity);

        return;
    }

    bool reached = entity.get<CanPathfind>().travel_toward(
        aiclosetab.position, get_speed_for_entity(entity) * dt);
    if (!reached) return;

    aiclosetab.pass_time(dt);
    if (!aiclosetab.ready()) return;

    OptEntity opt_reg = EntityHelper::getEntityForID(aiclosetab.id());
    if (!opt_reg) {
        log_warn("got an invalid register");
        aiclosetab.unset_target();
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

            aiclosetab.reset();
            return;
        }
        // otherwise walk up one spot
        aiclosetab.position = reg.get<Transform>().tile_infront(spot_in_line);
        return;
    } else {
        aiclosetab.position = reg.get<Transform>().tile_infront(1);
        if (aiclosetab.PayProcessingTime == -1) {
            // TODO make into a config?
            float pay_process_time = 1.f;
            aiclosetab.set_PayProcessing_time(pay_process_time);
        }
    }

    // TODO show an icon cause right now it just looks like they are standing
    // there
    bool completed = aiclosetab.PayProcessing(dt);
    if (!completed) {
        return;
    }

    // TODO just copy the time stuff from the other ai so its not instant

    // Now we should be at the front of the line
    // TODO i would like for the player to have to go over and "work" to process
    // their payment
    // TODO we also should show an Icon for what they want to do $$$

    Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
    CanOrderDrink& cod = entity.get<CanOrderDrink>();
    {
        IsBank& bank = sophie.get<IsBank>();
        bank.deposit_with_tip(cod.tab_cost, cod.tip);

        // i dont think we need to do this, but just in case
        cod.tab_cost = 0;
        cod.tip = 0;
    }

    WIQ_leave_line(reg, entity);

    next_job(entity, JobType::Leaving);

    reset_job_component<AICloseTab>(entity);
}

inline void process_jukebox_play(Entity& entity, float dt) {
    if (entity.is_missing<AIPlayJukebox>()) return;
    if (entity.is_missing<CanOrderDrink>()) return;

    AIPlayJukebox& ai_play_jukebox = entity.get<AIPlayJukebox>();

    const auto _find_available_jukebox = [&]() -> bool {
        if (ai_play_jukebox.has_available_target()) {
            return true;
        }
        OptEntity best_jukebox =
            EntityQuery()
                .whereType(EntityType::Jukebox)
                .whereHasComponent<HasWaitingQueue>()
                .whereLambda([](const Entity& entity) {
                    // Exclude full jukeboxs
                    const HasWaitingQueue& hwq = entity.get<HasWaitingQueue>();
                    if (hwq.is_full()) return false;
                    return true;
                })
                // Find the jukebox with the least people on it
                .orderByLambda([](const Entity& r1, const Entity& r2) {
                    const HasWaitingQueue& hwq1 = r1.get<HasWaitingQueue>();
                    int rpos1 = hwq1.get_next_pos();
                    const HasWaitingQueue& hwq2 = r2.get<HasWaitingQueue>();
                    int rpos2 = hwq2.get_next_pos();
                    return rpos1 < rpos2;
                })
                .gen_first();

        // We probably dont have a jukebox, so just ignore this for now
        // go back to ordering
        if (!best_jukebox) {
            ai_play_jukebox.reset();
            return false;
        }

        // We were the last person to put on a song, so we dont need to change
        // it (yet...)
        if (best_jukebox->get<HasLastInteractedCustomer>().customer_id ==
            entity.id) {
            return false;
        }

        ai_play_jukebox.set_target(best_jukebox->id);
        ai_play_jukebox.position =
            WIQ_add_to_queue_and_get_position(best_jukebox.asE(), entity);
        return true;
    };

    bool found = _find_available_jukebox();
    if (!found) {
        _set_customer_next_order(entity);
        reset_job_component<AIPlayJukebox>(entity);
        return;
    }

    bool reached = entity.get<CanPathfind>().travel_toward(
        ai_play_jukebox.position, get_speed_for_entity(entity) * dt);
    if (!reached) return;

    ai_play_jukebox.pass_time(dt);
    if (!ai_play_jukebox.ready()) return;

    OptEntity opt_reg = EntityHelper::getEntityForID(ai_play_jukebox.id());
    if (!opt_reg) {
        log_warn("got an invalid jukebox");
        ai_play_jukebox.unset_target();
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

            ai_play_jukebox.reset();
            return;
        }
        // otherwise walk up one spot
        ai_play_jukebox.position =
            reg.get<Transform>().tile_infront(spot_in_line);
        return;
    } else {
        ai_play_jukebox.position = reg.get<Transform>().tile_infront(1);

        if (ai_play_jukebox.findSongTime == -1) {
            // TODO make into a config?
            float song_time = 5.f;
            ai_play_jukebox.set_findSong_time(song_time);
        }
    }

    // Now we should be at the front of the line

    bool completed = ai_play_jukebox.findSong(dt);
    if (!completed) {
        return;
    }

    // TODO implement jukebox song change
    {
        Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
        IsBank& bank = sophie.get<IsBank>();
        bank.deposit(10);
    }

    // TODO it woud be nice to show the customer's face above the entity
    reg.get<HasLastInteractedCustomer>().customer_id = entity.id;

    WIQ_leave_line(reg, entity);

    _set_customer_next_order(entity);

    reset_job_component<AIPlayJukebox>(entity);
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
        case Paying:
            process_ai_paying(entity, dt);
            break;
        case PlayJukebox:
            process_jukebox_play(entity, dt);
            break;
        case NoJob:
        case Wait:
        case Wandering:
        case EnterStore:
        case WaitInQueueForPickup:
        case MAX_JOB_TYPE:
            break;
    }
}

}  // namespace ai
}  // namespace system_manager
