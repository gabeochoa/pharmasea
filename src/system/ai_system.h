
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

inline bool validate_drink_order(const Entity& customer, Drink orderedDrink,
                                 Item& madeDrink) {
    const Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
    const IsRoundSettingsManager& irsm = sophie.get<IsRoundSettingsManager>();
    // TODO how many ingredients have to be correct?
    // as people get more drunk they should care less and less
    //
    bool all_ingredients_match =
        madeDrink.get<IsDrink>().matches_drink(orderedDrink);

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
            std::string(magic_enum::enum_name(orderedDrink)));

        IngredientBitSet orderedDrinkSet = recipe.ingredients;
        IngredientBitSet madeDrinkSet = madeDrink.get<IsDrink>().ing();

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
            // TODO this return is what keeps us from being able to support
            // both upgrades at the same time (if we wanted that)
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

    if (irsm.has_upgrade_unlocked(UpgradeClass::CantEvenTell)) {
        const CanOrderDrink& cod = customer.get<CanOrderDrink>();
        size_t num_alc_drank = cod.num_alcoholic_drinks_drank();

        Recipe recipe = RecipeLibrary::get().get(
            std::string(magic_enum::enum_name(orderedDrink)));
        IngredientBitSet orderedDrinkSet = recipe.ingredients;
        IngredientBitSet madeDrinkSet = madeDrink.get<IsDrink>().ing();

        auto xorbits = orderedDrinkSet ^ madeDrinkSet;
        size_t num_messed_up = xorbits.count();

        // You messed up less ingredients than
        // the number of drinks they had
        //
        // So they cant tell :)
        //
        if (num_messed_up < num_alc_drank) {
            return true;
        }
    }

    return false;
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

        int denom = randIn(1, std::max(1, cha.num_alcoholic_drinks_drank()));
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
        bool gotta_go = (cod.get_drinks_in_bladder() >= bladder_size);
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

    bool found = aiwait.target.find_if_missing(
        entity,
        [](const Entity& toilet) {
            return toilet.get<HasWaitingQueue>().has_space();
        },
        [&](Entity& best_register) {
            aiwait.line_wait.add_to_queue(best_register, entity);
        });
    if (!found) {
        return;
    }

    bool reached = entity.get<CanPathfind>().travel_toward(
        aiwait.line_wait.position, get_speed_for_entity(entity) * dt);
    if (!reached) return;

    aiwait.pass_time(dt);
    if (!aiwait.ready()) return;

    OptEntity opt_reg = EntityHelper::getEntityForID(aiwait.target.id());
    if (!opt_reg) {
        log_warn("got an invalid register");
        return;
    }
    Entity& reg = opt_reg.asE();
    entity.get<Transform>().turn_to_face_pos(reg.get<Transform>().as2());

    bool reached_front = aiwait.line_wait.try_to_move_closer(
        reg, entity, get_speed_for_entity(entity) * dt);
    if (!reached_front) {
        return;
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

    Item& drink = regCHI.item();
    if (!check_if_drink(drink)) {
        log_info("this isnt a drink");
        aiwait.reset();
        return;
    }

    log_info("i got **A** drink ");

    Drink orderdDrink = canOrderDrink.order();
    bool was_drink_correct = validate_drink_order(entity, orderdDrink, drink);
    if (!was_drink_correct) {
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
        float base_price = get_base_price_for_drink(canOrderDrink.get_order());

        float speakeasy_multiplier = 1.f;
        if (irsm.has_upgrade_unlocked(UpgradeClass::Speakeasy)) {
            speakeasy_multiplier +=
                (0.01f * entity.get<CanPathfind>().get_max_length());
        }

        float cost_multiplier = irsm.get<float>(ConfigKey::DrinkCostMultiplier);

        float price_float = cost_multiplier * speakeasy_multiplier * base_price;
        int price = static_cast<int>(price_float);
        canOrderDrink.increment_tab(price);

        log_info(
            "Drink price was {} (base_price({}) * speakeasy({}) * "
            "cost_mult({}) => {})",
            price, base_price, speakeasy_multiplier, cost_multiplier,
            price_float);

        const HasPatience& hasPatience = entity.get<HasPatience>();
        int tip = (int) fmax(0, ceil(price * 0.8f * hasPatience.pct()));
        canOrderDrink.increment_tip(tip);

        // If the drink has any "fancy" ingredients or other multipliers
        canOrderDrink.apply_tip_multiplier(
            drink.get<IsDrink>().get_tip_multiplier());
    }

    CanHoldItem& ourCHI = entity.get<CanHoldItem>();
    ourCHI.update(EntityHelper::getEntityAsSharedPtr(regCHI.item()), entity.id);
    regCHI.update(nullptr, -1);

    log_info("got it");
    aiwait.line_wait.leave_line(reg, entity);

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
    cod.set_order(progressionManager.get_random_unlocked_drink());

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

    bool found =
        aidrinking.target.find_if_missing(entity, nullptr, [&](Entity&) {
            float drink_time = irsm.get<float>(ConfigKey::MaxDrinkTime);
            drink_time += randfIn(0.1f, 1.f);
            aidrinking.timer.set_time(drink_time);
        });
    if (!found) {
        return;
    }

    // We have a target
    OptEntity opt_drink_pos =
        EntityHelper::getEntityForID(aidrinking.target.id());

    bool reached = entity.get<CanPathfind>().travel_toward(
        opt_drink_pos.asE().get<Transform>().as2(),
        get_speed_for_entity(entity) * dt);
    if (!reached) return;

    bool completed = aidrinking.timer.pass_time(dt);
    if (!completed) {
        return;
    }

    // Done with my drink, delete it
    CanHoldItem& chi = entity.get<CanHoldItem>();
    chi.item().cleanup = true;
    chi.update(nullptr, -1);

    // Mark our current order finished
    cod.on_order_finished();

    // TODO :MAKE_DURING_FIND: because we made an entity during find_target,
    // we need to delete it before we unset
    opt_drink_pos.asE().cleanup = true;

    aidrinking.target.unset();

    //
    // Do we want another drink?
    //

    bool want_another = cod.wants_more_drinks();

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

    bool found_target = aiclean.target.find_if_missing(entity);
    if (!found_target) {
        return;
    }

    // We have a target
    OptEntity vomit = EntityHelper::getEntityForID(aiclean.target.id());

    if (!vomit) {
        aiclean.target.unset();
        return;
    }

    bool reached = entity.get<CanPathfind>().travel_toward(
        vomit->get<Transform>().as2(), get_speed_for_entity(entity) * dt);
    if (!reached) return;

    HasWork& vomWork = vomit->get<HasWork>();
    vomWork.call(vomit.asE(), entity, dt);
    // check if we did it
    if (vomit->cleanup) {
        aiclean.target.unset();
        return;
    }
}

inline void process_ai_use_bathroom(Entity& entity, float dt) {
    if (entity.is_missing<AIUseBathroom>()) return;
    AIUseBathroom& aibathroom = entity.get<AIUseBathroom>();

    Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
    const IsRoundSettingsManager& irsm = sophie.get<IsRoundSettingsManager>();
    const CanOrderDrink& cod = entity.get<CanOrderDrink>();

    int bladder_size = irsm.get<int>(ConfigKey::BladderSize);
    bool gotta_go = (cod.get_drinks_in_bladder() >= bladder_size);
    if (!gotta_go) return;

    aibathroom.pass_time(dt);
    if (!aibathroom.ready()) return;

    bool found = aibathroom.target.find_if_missing(
        entity,
        [](const Entity& toilet) {
            return toilet.get<HasWaitingQueue>().has_space();
        },
        [&](Entity& best_toilet) {
            aibathroom.line_wait.add_to_queue(best_toilet, entity);

            // TODO setting?
            // right now just set it to 5 seconds in line until you go on the
            // floor
            aibathroom.floor_timer.set_time(5.f);
        });
    if (!found) return;

    OptEntity opt_toilet = EntityHelper::getEntityForID(aibathroom.target.id());
    if (!opt_toilet) {
        log_warn("got an invalid toilet");
        return;
    }
    Entity& toilet = opt_toilet.asE();
    entity.get<Transform>().turn_to_face_pos(toilet.get<Transform>().as2());
    IsToilet& istoilet = toilet.get<IsToilet>();

    const auto _onFinishedGoing = [&]() {
        aibathroom.line_wait.leave_line(toilet, entity);

        aibathroom.target.unset();
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
    };

    // We are now in line, so start the floor timer
    bool floor_complete = aibathroom.floor_timer.pass_time(dt);
    if (floor_complete) {
        // ive been in line so long, im just gonna pee on the ground
        auto& vom = EntityHelper::createEntity();
        furniture::make_vomit(vom,
                              SpawnInfo{
                                  .location = entity.get<Transform>().as2(),
                                  .is_first_this_round = false,
                              });
        // then empty bladder and end job
        _onFinishedGoing();
        return;
    }

    bool reached = entity.get<CanPathfind>().travel_toward(
        aibathroom.line_wait.position, get_speed_for_entity(entity) * dt);
    if (!reached) return;

    bool reached_front = aibathroom.line_wait.try_to_move_closer(
        toilet, entity, get_speed_for_entity(entity) * dt);
    if (!reached_front) {
        return;
    }

    // Now we are at the front of the line

    // Either needs cleaning or someone else is using it
    bool not_me = !istoilet.available() && !istoilet.is_user(entity.id);
    if (not_me) {
        return;
    }

    bool we_are_using_it = istoilet.is_user(entity.id);
    if (!we_are_using_it) {
        float piss_timer = irsm.get<float>(ConfigKey::PissTimer);
        aibathroom.timer.set_time(piss_timer);
        istoilet.start_use(entity.id);
    }

    bool completed = aibathroom.timer.pass_time(dt);
    if (completed) {
        _onFinishedGoing();
    }
}

inline void process_ai_leaving(Entity& entity, float dt) {
    // TODO check the return value here and if true, stop running the
    // pathfinding
    // ^ does this mean just dynamically remove CanPathfind from the customer
    // entity?
    //
    // I noticed this during profiling :)
    //
    (void) entity.get<CanPathfind>().travel_toward(
        vec2{GATHER_SPOT, GATHER_SPOT}, get_speed_for_entity(entity) * dt);
}

inline void process_ai_paying(Entity& entity, float dt) {
    if (entity.is_missing<AICloseTab>()) return;
    if (entity.is_missing<CanOrderDrink>()) return;

    AICloseTab& aiclosetab = entity.get<AICloseTab>();

    bool found_target = aiclosetab.target.find_if_missing(
        entity,
        [](const Entity& reg) {
            return reg.get<HasWaitingQueue>().has_space();
        },
        [&](Entity& best_register) {
            aiclosetab.line_wait.add_to_queue(best_register, entity);
        });
    if (!found_target) {
        return;
    }

    bool reached = entity.get<CanPathfind>().travel_toward(
        aiclosetab.line_wait.position, get_speed_for_entity(entity) * dt);
    if (!reached) return;

    aiclosetab.pass_time(dt);
    if (!aiclosetab.ready()) return;

    OptEntity opt_reg = EntityHelper::getEntityForID(aiclosetab.target.id());
    if (!opt_reg) {
        log_warn("got an invalid register");
        aiclosetab.target.unset();
        return;
    }
    Entity& reg = opt_reg.asE();
    entity.get<Transform>().turn_to_face_pos(reg.get<Transform>().as2());

    bool reached_front = aiclosetab.line_wait.try_to_move_closer(
        reg, entity, get_speed_for_entity(entity) * dt, [&]() {
            if (!aiclosetab.timer.initialized) {
                // TODO make into a config?
                float pay_process_time = 1.f;
                aiclosetab.timer.set_time(pay_process_time);
            }
        });
    if (!reached_front) {
        return;
    }

    // TODO show an icon cause right now it just looks like they are standing
    // there
    bool completed = aiclosetab.timer.pass_time(dt);
    if (!completed) {
        return;
    }

    // TODO just copy the time stuff from the other ai so its not instant

    // Now we should be at the front of the line
    // TODO i would like for the player to have to go over and "work" to process
    // their payment

    Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
    CanOrderDrink& cod = entity.get<CanOrderDrink>();
    {
        IsBank& bank = sophie.get<IsBank>();
        bank.deposit_with_tip(cod.get_current_tab(), cod.get_current_tip());

        // i dont think we need to do this, but just in case
        cod.clear_tab_and_tip();
    }

    aiclosetab.line_wait.leave_line(reg, entity);

    next_job(entity, JobType::Leaving);

    reset_job_component<AICloseTab>(entity);
}

inline void process_jukebox_play(Entity& entity, float dt) {
    if (entity.is_missing<AIPlayJukebox>()) return;
    if (entity.is_missing<CanOrderDrink>()) return;

    AIPlayJukebox& ai_play_jukebox = entity.get<AIPlayJukebox>();

    bool found = ai_play_jukebox.target.find_if_missing(
        entity,
        [&](const Entity& best_jukebox) -> bool {
            // We were the last person to put on a song, so we dont need to
            // change it (yet...)
            if (best_jukebox.get<HasLastInteractedCustomer>().customer_id ==
                entity.id) {
                return false;
            }
            return true;
        },
        [&](Entity& best_jukebox) -> void {
            ai_play_jukebox.line_wait.add_to_queue(best_jukebox, entity);
        });

    if (!found) {
        _set_customer_next_order(entity);
        reset_job_component<AIPlayJukebox>(entity);
        return;
    }

    bool reached = entity.get<CanPathfind>().travel_toward(
        ai_play_jukebox.line_wait.position, get_speed_for_entity(entity) * dt);
    if (!reached) return;

    ai_play_jukebox.pass_time(dt);
    if (!ai_play_jukebox.ready()) return;

    OptEntity opt_reg =
        EntityHelper::getEntityForID(ai_play_jukebox.target.id());
    if (!opt_reg) {
        log_warn("got an invalid jukebox");
        ai_play_jukebox.target.unset();
        return;
    }
    Entity& reg = opt_reg.asE();
    entity.get<Transform>().turn_to_face_pos(reg.get<Transform>().as2());

    bool reached_front = ai_play_jukebox.line_wait.try_to_move_closer(
        reg, entity, get_speed_for_entity(entity) * dt, [&]() {
            if (!ai_play_jukebox.timer.initialized) {
                // TODO make into a config?
                float song_time = 5.f;
                ai_play_jukebox.timer.set_time(song_time);
            }
        });
    if (!reached_front) {
        return;
    }

    // Now we should be at the front of the line

    bool completed = ai_play_jukebox.timer.pass_time(dt);
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

    ai_play_jukebox.line_wait.leave_line(reg, entity);

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
