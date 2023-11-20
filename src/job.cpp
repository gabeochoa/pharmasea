
#include "job.h"

#include "components/can_order_drink.h"
#include "components/can_pathfind.h"
#include "components/can_perform_job.h"
#include "components/has_base_speed.h"
#include "components/has_patience.h"
#include "components/has_speech_bubble.h"
#include "components/has_timer.h"
#include "components/has_waiting_queue.h"
#include "components/is_bank.h"
#include "components/is_drink.h"
#include "components/is_progression_manager.h"
#include "components/is_round_settings_manager.h"
#include "components/is_toilet.h"
#include "engine/assert.h"
#include "engine/bfs.h"
#include "engine/pathfinder.h"
#include "entity.h"
#include "entity_helper.h"
#include "entity_query.h"
#include "globals.h"
#include "system/logging_system.h"

float get_remaining_time_in_round() {
    const Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
    const HasTimer& hasTimer = sophie.get<HasTimer>();
    return hasTimer.remaining_time_in_round();
}

inline Job* create_wandering_job(vec2 _start) {
    // TODO add cooldown so that not all time is spent here
    int max_tries = 10;
    int range = 20;
    bool walkable = false;
    int i = 0;
    vec2 target;
    while (!walkable) {
        target =
            (vec2){1.f * randIn(-range, range), 1.f * randIn(-range, range)};
        walkable = EntityHelper::isWalkable(target);
        i++;
        if (i > max_tries) {
            return nullptr;
        }
    }
    return new WanderingJob(_start, target);
}

inline Job* create_drinking_job(vec2 _start) {
    // TODO add cooldown so that not all time is spent here
    // TODO find only places inside the place
    int max_tries = 10;
    int range = 20;
    bool walkable = false;
    int i = 0;
    vec2 target;
    while (!walkable) {
        target =
            (vec2){1.f * randIn(-range, range), 1.f * randIn(-range, range)};
        walkable = EntityHelper::isWalkable(target);
        i++;
        if (i > max_tries) {
            return nullptr;
        }
    }
    return new DrinkingJob(_start, target, randIn(1, 5));
}

Job* Job::create_job_of_type(vec2 _start, vec2 _end, JobType job_type) {
    Job* job = nullptr;
    switch (job_type) {
        case Wandering:
            job = create_wandering_job(_start);
            break;
        case WaitInQueue:
            job = new WaitInQueueJob();
            break;
        case Wait:
            job = new WaitJob(_start, _end, 1.f);
            break;
            // TODO do we even need the NoneJob?
        case NoJob:
            job = new WaitJob(_start, _end, 1.f);
            break;
        case Leaving:
            job = new LeavingJob(_start, _end);
            break;
        case Mopping:
            job = new MoppingJob();
            break;
        case Bathroom:
            job = new BathroomJob();
            break;
            // TODO remove this so we get the non-exhausted warning
        default:
            log_warn(
                "Trying to replace job with type {}({}) but doesnt have it",
                magic_enum::enum_name(job_type), job_type);
            break;
    }
    return job;
}

bool Job::is_at_position(const Entity& entity, vec2 position) {
    return vec::distance(entity.get<Transform>().as2(), position) <
           (TILESIZE / 2.f);
}

Entity& Job::get_and_validate_entity(int id) {
    OptEntity opt_e = EntityHelper::getEntityForID(id);
    VALIDATE(opt_e, "entity with id did not exist");
    return opt_e.asE();
}

inline void WIQ_wait_and_return(Entity& entity, vec2 start, vec2 end) {
    // note ^ if you are gonna change this to be WIQ specific please update the
    // callers ...

    CanPerformJob& cpj = entity.get<CanPerformJob>();
    // Add the current job to the queue,
    // and then add the waiting job
    cpj.push_and_reset(new WaitJob(start, end, 1.f));
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

inline bool WIQ_can_move_up(const Entity& reg, const Entity& customer) {
    VALIDATE(reg.has<HasWaitingQueue>(),
             "Trying to can-move-up for entity which doesnt "
             "have a waiting queue ");
    return reg.get<HasWaitingQueue>().matching_id(customer.id, 0);
}

void WaitInQueueJob::before_each_job_tick(Entity& entity, float) {
    OptEntity reg = EntityHelper::getEntityForID(reg_id);
    // This runs before init so its possible theres no register at all
    if (reg) {
        entity.get<Transform>().turn_to_face_pos(reg->get<Transform>().as2());
    }
}

Job::State WaitInQueueJob::run_state_initialize(Entity& entity, float) {
    log_warn("starting a new wait in queue job");

    // Figure out which register to go to...

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
        log_warn("Could not find a valid register");
        vec2 wait_position = entity.get<Transform>().as2();
        WIQ_wait_and_return(entity, wait_position, wait_position);
        return Job::State::Initialize;
    }

    VALIDATE(best_target, "underlying job should contain a register now");

    // Store into our job data
    reg_id = best_target->id;

    start = WIQ_add_to_queue_and_get_position(best_target.asE(), entity);
    // TODO should be be rpos instead of 1?
    end = best_target->get<Transform>().tile_infront(1);

    spot_in_line = WIQ_position_in_line(best_target.asE(), entity);

    VALIDATE(spot_in_line >= 0, "customer should be in line right now");

    return Job::State::HeadingToStart;
}

Job::State WaitInQueueJob::run_state_working_at_start(Entity& entity, float) {
    if (spot_in_line == 0) {
        return (Job::State::HeadingToEnd);
    }

    VALIDATE(reg_id != -1, "workingatstart job should contain register");
    Entity& reg = get_and_validate_entity(reg_id);

    // Check the spot in front of us

    int cur_spot_in_line = WIQ_position_in_line(reg, entity);
    if (cur_spot_in_line == spot_in_line) {
        // We didnt move so just wait a bit before trying again
        system_manager::logging_manager::announce(
            entity, fmt::format("im just going to wait a bit longer"));

        // Add the current job to the queue,
        // and then add the waiting job
        vec2 wait_position = entity.get<Transform>().as2();
        WIQ_wait_and_return(entity, wait_position, wait_position);
        return (Job::State::WorkingAtStart);
    }

    if (!WIQ_can_move_up(reg, entity)) {
        // We cant move so just wait a bit before trying again
        system_manager::logging_manager::announce(
            entity, fmt::format("im just going to wait a bit longer"));

        // Add the current job to the queue,
        // and then add the waiting job
        vec2 wait_position = entity.get<Transform>().as2();
        WIQ_wait_and_return(entity, wait_position, wait_position);
        return (Job::State::WorkingAtStart);
    }

    // if our spot did change, then move forward
    system_manager::logging_manager::announce(
        entity, fmt::format("im moving up to {}", cur_spot_in_line));

    spot_in_line = cur_spot_in_line;

    if (spot_in_line == 0) {
        start = reg.get<Transform>().tile_infront(1);
        return (Job::State::HeadingToEnd);
    }

    // otherwise walk up one spot
    start = reg.get<Transform>().tile_infront(spot_in_line);
    return (Job::State::WorkingAtStart);
}

Job::State WaitInQueueJob::run_state_working_at_end(Entity& entity, float) {
    VALIDATE(reg_id != -1, "workingatend job should contain register");
    Entity& reg = get_and_validate_entity(reg_id);

    // TODO this logic likely should move to system
    {
        // TODO safer way to do it?
        // we are at the front so turn it on
        entity.get<HasSpeechBubble>().on();
        entity.get<HasPatience>().enable();
    }

    CanOrderDrink& canOrderDrink = entity.get<CanOrderDrink>();

    if (canOrderDrink.order_state == CanOrderDrink::OrderState::NeedsReset) {
        // TODO this will be an infinite loop if we dont do
        // cod.current_order = progressionManager.get_random_unlocked_drink();
        // cod.order_state = CanOrderDrink::OrderState::Ordering;
        system_manager::logging_manager::announce(
            entity, "I havent decided what i want yet");
        return (Job::State::WorkingAtEnd);
    }

    VALIDATE(canOrderDrink.has_order(), "I should have an order");

    CanHoldItem& regCHI = reg.get<CanHoldItem>();

    if (regCHI.empty()) {
        system_manager::logging_manager::announce(entity,
                                                  "my drink isnt ready yet");
        vec2 wait_position = entity.get<Transform>().as2();
        WIQ_wait_and_return(entity, wait_position, wait_position);
        return (Job::State::WorkingAtEnd);
    }

    std::shared_ptr<Item> drink = reg.get<CanHoldItem>().item();
    if (!drink || !check_if_drink(*drink)) {
        system_manager::logging_manager::announce(entity, "this isnt a drink");
        vec2 wait_position = entity.get<Transform>().as2();
        WIQ_wait_and_return(entity, wait_position, wait_position);
        return (Job::State::WorkingAtEnd);
    }

    system_manager::logging_manager::announce(entity, "i got **A** drink ");

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
        system_manager::logging_manager::announce(entity,
                                                  "this isnt what i ordered");
        vec2 wait_position = entity.get<Transform>().as2();
        WIQ_wait_and_return(entity, wait_position, wait_position);
        return (Job::State::WorkingAtEnd);
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

    system_manager::logging_manager::announce(entity, "got it");
    WIQ_leave_line(reg, entity);

    // Now that we are done and got our item, time to leave the store
    {
        std::shared_ptr<Job> jshared;
        jshared.reset(create_drinking_job(entity.get<Transform>().as2()));
        entity.get<CanPerformJob>().push_onto_queue(jshared);
    }

    // TODO Should move to system
    {
        canOrderDrink.order_state = CanOrderDrink::OrderState::DrinkingNow;
        entity.get<HasSpeechBubble>().off();
        entity.get<HasPatience>().disable();
        entity.get<HasPatience>().reset();
    }

    log_info("leaving line");
    entity.get<CanPerformJob>().current = JobType::Drinking;
    return (Job::State::Completed);
}

Job::State WaitJob::run_state_working_at_end(Entity&, float dt) {
    timePassedInCurrentState += dt;
    if (timePassedInCurrentState >= timeToComplete) {
        return (Job::State::Completed);
    }
    // system_manager::logging_manager::announce(
    // entity, fmt::format("waiting a little longer: {} => {} ",
    // timePassedInCurrentState, timeToComplete));

    return (Job::State::WorkingAtEnd);
}

Job::State LeavingJob::run_state_working_at_end(Entity& entity, float) {
    // Now that we are done and got our item, time to leave the store
    {
        vec2 start = entity.get<Transform>().as2();
        std::shared_ptr<Job> jshared = std::make_shared<WaitJob>(
            start,
            // TODO create a global so they all leave to the same spot
            vec2{GATHER_SPOT, GATHER_SPOT}, get_remaining_time_in_round());
        entity.get<CanPerformJob>().push_onto_queue(jshared);
    }
    return (Job::State::Completed);
}

Job::State DrinkingJob::run_state_working_at_end(Entity& entity, float dt) {
    CanOrderDrink& cod = entity.get<CanOrderDrink>();
    if (cod.order_state != CanOrderDrink::OrderState::DrinkingNow) {
        return (Job::State::Completed);
    }
    return (Job::State::WorkingAtEnd);
}

Job::State MoppingJob::run_state_initialize(Entity&, float) {
    log_trace("starting a new mop job");
    return Job::State::Initialize;
}

Job::State MoppingJob::run_state_working_at_start(Entity&, float) {
    return (Job::State::Completed);
}

Job::State BathroomJob::run_state_initialize(Entity& entity, float) {
    return (Job::State::Completed);
}

Job::State BathroomJob::run_state_working_at_start(Entity& entity, float dt) {
    return (Job::State::Initialize);
}
Job::State BathroomJob::run_state_working_at_end(Entity& entity, float) {
    return (Job::State::Completed);
}
