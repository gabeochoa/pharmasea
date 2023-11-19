
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
float get_speed_for_entity(Entity& entity) {
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

Job::State Job::run_state_heading_to_(Job::State begin, Entity& entity,
                                      float dt) {
    Job::State complete = begin == Job::State::HeadingToStart
                              ? Job::State::WorkingAtStart
                              : Job::State::WorkingAtEnd;
    vec2 goal = begin == Job::State::HeadingToStart ? start : end;

    return entity.get<CanPathfind>().travel_toward(
               goal, get_speed_for_entity(entity) * dt)
               ? complete
               : begin;
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

    canOrderDrink.order_state = CanOrderDrink::OrderState::DrinkingNow;

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
        entity.get<HasSpeechBubble>().off();
        entity.get<HasPatience>().disable();
        entity.get<HasPatience>().reset();
    }

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
    // turn off order visible
    entity.get<HasSpeechBubble>().off();

    CanOrderDrink& cod = entity.get<CanOrderDrink>();
    cod.order_state = CanOrderDrink::OrderState::DrinkingNow;
    // TODO should this be here
    entity.get<HasSpeechBubble>().on();

    timePassedInCurrentState += dt;

    // not enough time has passed, continue "drinking"
    if (timePassedInCurrentState < timeToComplete) {
        return (Job::State::WorkingAtEnd);
    }
    CanHoldItem& chi = entity.get<CanHoldItem>();

    // Because we might come back here after going ot the bathroom or something
    // we need to know if this was the first time we came here or not
    // the best way is to just see if we have a drink in our hand
    bool first_time = (chi.item() != nullptr);

    // Done with my drink, delete it
    if (first_time) {  // We have this check because if we went to the bathroom
                       // then we would've put down our drink in the previous
                       // frame
        chi.item()->cleanup = true;
        chi.update(nullptr, -1);

        // mark order complete
        cod.on_order_finished();
    }

    Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
    const IsRoundSettingsManager& irsm = sophie.get<IsRoundSettingsManager>();

    bool bathroom_unlocked = irsm.get<bool>(ConfigKey::UnlockedToilet);
    if (bathroom_unlocked) {
        int bladder_size = irsm.get<int>(ConfigKey::BladderSize);
        bool gotta_go = (cod.drinks_in_bladder >= bladder_size);

        // Needs to go to the bathroom?
        if (gotta_go) {
            system_manager::logging_manager::announce(
                entity, "i gotta go to the bathroom");

            CanPerformJob& cpj = entity.get<CanPerformJob>();
            // Add the current job to the queue,
            // and then add the bathroom job

            vec2 pos = entity.get<Transform>().as2();
            float piss_timer = irsm.get<float>(ConfigKey::PissTimer);
            cpj.push_and_reset(new BathroomJob(pos, pos, piss_timer));

            // Doing working at end since we still gotta do the below
            return (Job::State::WorkingAtEnd);
        }
    }

    // dont need to go anymore. do we want another drink?

    std::shared_ptr<Job> jshared;

    if (cod.num_orders_rem > 0) {
        system_manager::logging_manager::announce(entity,
                                                  "im getting another drink");
        // get next order
        {
            const IsProgressionManager& progressionManager =
                sophie.get<IsProgressionManager>();

            // TODO make a function set_order()
            cod.current_order = progressionManager.get_random_unlocked_drink();
            cod.order_state = CanOrderDrink::OrderState::Ordering;
        }

        vec2 start = entity.get<Transform>().as2();
        jshared.reset(create_job_of_type(start, start, JobType::WaitInQueue));
        entity.get<CanPerformJob>().push_onto_queue(jshared);
    } else {
        // Now we are fully done so lets pay.
        system_manager::logging_manager::announce(entity, "im done drinking");

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

        jshared.reset(new WaitJob(
            // TODO they go back to the register before leaving becausd
            // start here..
            start,
            // TODO create a global so they all leave to the same spot
            vec2{GATHER_SPOT, GATHER_SPOT}, get_remaining_time_in_round()));
    }

    entity.get<CanPerformJob>().push_onto_queue(jshared);
    return (Job::State::Completed);
}

Job::State MoppingJob::run_state_initialize(Entity& entity, float) {
    log_trace("starting a new mop job");

    // Find the closest vomit
    OptEntity closest =
        EntityHelper::getClosestOfType(entity, EntityType::Vomit);

    if (!closest) {
        log_trace("Could not find any vomit");
        // TODO make this function name more generic / obvious its shared
        vec2 wait_position = entity.get<Transform>().as2();
        WIQ_wait_and_return(entity, wait_position, wait_position);
        return Job::State::Initialize;
    }

    vom_id = closest->id;
    OptEntity vom = EntityHelper::getEntityForID(vom_id);
    VALIDATE(vom, "underlying job should contain vomit now");

    start = vom->get<Transform>().as2();
    end = start;

    system_manager::logging_manager::announce(
        entity, fmt::format("heading to vomit {}", start));

    return Job::State::HeadingToStart;
}

Job::State MoppingJob::run_state_working_at_start(Entity& entity, float dt) {
    VALIDATE(vom_id != -1, "underlying job should contain vomit now");

    OptEntity vom = EntityHelper::getEntityForID(vom_id);
    if (!vom) {
        system_manager::logging_manager::announce(
            entity, fmt::format("seems like someone beat me to it"));
        return (Job::State::HeadingToEnd);
    }
    HasWork& vomWork = vom->get<HasWork>();

    // do some work
    vomWork.call(vom.asE(), entity, dt);

    // check if we did it
    bool cleaned_up = vom->cleanup;

    if (cleaned_up) {
        system_manager::logging_manager::announce(
            entity, fmt::format("i cleaned it up, see ya"));
        return (Job::State::HeadingToEnd);
    }

    // otherwise keep working
    system_manager::logging_manager::announce(entity,
                                              fmt::format("im cleaning "));
    return (Job::State::WorkingAtStart);
}

Job::State BathroomJob::run_state_initialize(Entity& entity, float) {
    log_warn("starting a new bathroom job");

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

    // TODO after a couple loops maybe you just go on the floor :(
    if (!best_target) {
        system_manager::logging_manager::announce(
            entity, "toilet missing available toilet");
        // TODO find a better spot to wait?
        WIQ_wait_and_return(entity, vec2{0, 0}, vec2{0, 0});
        return Job::State::Initialize;
    }

    VALIDATE(best_target, "underlying job should contain a toilet now");

    // Store into our job data
    toilet_id = best_target->id;

    start = best_target.asE().get<Transform>().as2();
    end = start;

    system_manager::logging_manager::announce(entity,
                                              "toilet heading to start");

    return Job::State::HeadingToStart;
}

Job::State BathroomJob::run_state_working_at_start(Entity& entity, float dt) {
    OptEntity opt_toilet = EntityHelper::getEntityForID(toilet_id);
    if (!opt_toilet) {
        log_warn(
            "toilet in the bathroom job not found, just gonna mark this "
            "complete");
        return (Job::State::Completed);
    }

    Entity& toilet = opt_toilet.asE();
    IsToilet& istoilet = toilet.get<IsToilet>();

    bool we_are_using_it = istoilet.is_user(entity.id);
    bool occupied = istoilet.occupied() && !we_are_using_it;

    // TODO after a couple loops maybe you just go on the floor :(
    if (occupied) {
        system_manager::logging_manager::announce(entity,
                                                  "someones already in there");
        WIQ_wait_and_return(entity, toilet.get<Transform>().tile_infront(2),
                            toilet.get<Transform>().tile_infront(1));
        return Job::State::WorkingAtStart;
    }

    // we are using it
    if (we_are_using_it) {
        timePassedInCurrentState += dt;
        if (timePassedInCurrentState >= timeToComplete) {
            system_manager::logging_manager::announce(entity, "i finished");
            return (Job::State::HeadingToEnd);
        }
        // system_manager::logging_manager::announce(
        // entity, fmt::format("waiting a little longer: {} => {} ",
        // timePassedInCurrentState, timeToComplete));
        return Job::State::WorkingAtStart;
    }

    // ?TODO right now we dont mark occupied until the person gets there
    // obv this is like real life where two people just gotta go
    // at the same time.
    //
    // this means that its possible there is a free toilet on the other side of
    // the map and people are still using this one because they all grabbed at
    // the same time
    //
    // this might be frustrating as a player since you are like "why are they so
    // dumb"
    //
    // instead of the wait above, maybe do a wait and search?
    istoilet.start_use(entity.id);
    return (Job::State::WorkingAtStart);
}
Job::State BathroomJob::run_state_working_at_end(Entity& entity, float) {
    entity.get<CanOrderDrink>().empty_bladder();

    OptEntity opt_toilet = EntityHelper::getEntityForID(toilet_id);
    if (!opt_toilet) {
        log_error("toilet in the bathroom job not found");
        return (Job::State::Completed);
    }

    Entity& toilet = opt_toilet.asE();
    IsToilet& istoilet = toilet.get<IsToilet>();
    istoilet.end_use();

    return (Job::State::Completed);
}
