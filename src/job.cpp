
#include "job.h"

#include "components/can_order_drink.h"
#include "components/can_perform_job.h"
#include "components/has_base_speed.h"
#include "components/has_speech_bubble.h"
#include "components/has_waiting_queue.h"
#include "components/is_drink.h"
#include "components/is_progression_manager.h"
#include "engine/assert.h"
#include "engine/astar.h"
#include "entity.h"
#include "entity_helper.h"
#include "globals.h"
#include "system/logging_system.h"

[[nodiscard]] std::shared_ptr<Entity> HasWaitingQueue::person(int i) {
    EntityID id = ppl_in_line[i];
    return EntityHelper::getEntityPtrForID(id);
}

[[nodiscard]] const std::shared_ptr<Entity> HasWaitingQueue::person(
    size_t i) const {
    EntityID id = ppl_in_line[i];
    return EntityHelper::getEntityPtrForID(id);
}

HasWaitingQueue& HasWaitingQueue::add_customer(const Entity& customer) {
    log_info("we are adding {} {} to the line in position {}", customer.id,
             customer.get<DebugName>().name(), next_line_position);
    ppl_in_line[next_line_position] = customer.id;
    next_line_position++;

    VALIDATE(has_matching_person(customer.id) >= 0,
             "Customer should be in line somewhere");
    return *this;
}

void HasWaitingQueue::dump_contents() const {
    log_info("dumping contents of ppl_in_line");
    for (int i = 0; i < max_queue_size; i++) {
        log_info("index: {}, set? {}, id {}", i, has_person_in_position(i),
                 has_person_in_position(i) ? ppl_in_line[i] : -1);
    }
}

bool HasWaitingQueue::matching_id(int id, int i) const {
    return has_person_in_position(i) ? ppl_in_line[i] == id : false;
}

// TODO rename this or fix return type to work in other contexts better
int HasWaitingQueue::has_matching_person(int id) const {
    for (int i = 0; i < max_queue_size; i++) {
        if (matching_id(id, i)) return i;
    }
    log_warn("Cannot find customer {} in line", id);
    return -1;
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
    VALIDATE(valid(opt_e), "entity with id did not exist");
    return asE(opt_e);
}

Job::State Job::run_state_heading_to_start(Entity& entity, float dt) {
    // log_info("heading to start job {}", magic_enum::enum_name(type));
    travel_to_position(entity, dt, start);
    return (is_at_position(entity, start) ? Job::State::WorkingAtStart
                                          : Job::State::HeadingToStart);
}

Job::State Job::run_state_heading_to_end(Entity& entity, float dt) {
    // log_info("heading to end job {}", magic_enum::enum_name(type));
    travel_to_position(entity, dt, end);
    return (is_at_position(entity, end) ? Job::State::WorkingAtEnd
                                        : Job::State::HeadingToEnd);
}

void Job::travel_to_position(Entity& entity, float dt, vec2 goal) {
    // we just call this again cause its fun, be we could merge the two in
    // the future
    if (is_at_position(entity, goal)) {
        system_manager::logging_manager::announce(
            entity,
            fmt::format("no need to travel we are already at the goal {}", 1));
        return;
    }

    const auto _grab_path_to_goal = [this, goal](const Entity& entity) {
        if (!path_empty()) return;

        vec2 me = entity.get<Transform>().as2();

        {
            auto new_path = astar::find_path(
                me, goal,
                std::bind(EntityHelper::isWalkable, std::placeholders::_1));
            update_path(new_path);
            system_manager::logging_manager::announce(
                entity, fmt::format("gen path from {} to {} with {} steps", me,
                                    goal, p_size()));
        }

        // TODO For now we are just going to let the customer noclip
        if (path_empty()) {
            log_warn("Forcing customer {} to noclip in order to get valid path",
                     entity.id);
            auto new_path =
                astar::find_path(me, goal, [](auto&&) { return true; });
            update_path(new_path);
            system_manager::logging_manager::announce(
                entity, fmt::format("gen path from {} to {} with {} steps", me,
                                    goal, p_size()));
        }
        // what happens if we get here and the path is still empty?
        VALIDATE(!path_empty(), "path should no longer be empty");
    };

    const auto _grab_local_target = [this](const Entity& entity) {
        // Either we dont yet have a local target
        // or we already reached the one we had

        if (has_local_target()) {
            if (!is_at_position(entity, local.value())) {
                return;
            }
        }

        local = path_front();
        path_pop_front();

        VALIDATE(has_local_target(), "job should have a local target");
    };

    const auto _move_toward_local_target = [this, dt](Entity& entity) {
        // TODO forcing get<HasBaseSpeed> to crash here
        float base_speed = entity.get<HasBaseSpeed>().speed();

        // TODO Does OrderDrink hold stagger information?
        // or should it live in another component?
        if (entity.has<CanOrderDrink>()) {
            // const CanOrderDrink& cha = entity.get<CanOrderDrink>();
            // float speed_multiplier = cha.ailment().speed_multiplier();
            // if (speed_multiplier != 0) base_speed *= speed_multiplier;

            // TODO Turning off stagger; couple problems
            // - configuration is hard to reason about and mess with
            // - i really want it to cause them to move more, maybe we place
            // this in the path generation or something isntead?
            //
            // float stagger_multiplier = cha.ailment().stagger(); if
            // (stagger_multiplier != 0) base_speed *= stagger_multiplier;

            base_speed = fmaxf(1.f, base_speed);
            // log_info("multiplier {} {} {}", speed_multiplier,
            // stagger_multiplier, base_speed);
        }

        float speed = base_speed * dt;

        Transform& transform = entity.get<Transform>();

        vec2 new_pos = transform.as2();

        vec2 tar = local.value();
        if (tar.x > transform.raw().x) new_pos.x += speed;
        if (tar.x < transform.raw().x) new_pos.x -= speed;

        if (tar.y > transform.raw().z) new_pos.y += speed;
        if (tar.y < transform.raw().z) new_pos.y -= speed;

        // TODO do we need to unr the whole person_update...() function with
        // collision?

        transform.update(vec::to3(new_pos));
    };

    _grab_path_to_goal(entity);
    _grab_local_target(entity);
    _move_toward_local_target(entity);
}

inline void WIQ_wait_and_return(Entity& entity) {
    CanPerformJob& cpj = entity.get<CanPerformJob>();
    // Add the current job to the queue,
    // and then add the waiting job
    cpj.push_and_reset(new WaitJob(cpj.job_start(), cpj.job_start(), 1.f));
    return;
}

inline vec2 WIQ_add_to_queue_and_get_position(Entity& reg, Entity& customer) {
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

inline int WIQ_position_in_line(Entity& reg, Entity& customer) {
    // VALIDATE(customer, "entity passed to position-in-line should not be
    // null"); VALIDATE(reg, "entity passed to position-in-line should not be
    // null");
    VALIDATE(
        reg.has<HasWaitingQueue>(),
        "Trying to pos-in-line for entity which doesnt have a waiting queue ");
    const HasWaitingQueue& hwq = reg.get<HasWaitingQueue>();
    return hwq.has_matching_person(customer.id);
}

inline void WIQ_leave_line(Entity& reg, Entity& customer) {
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
    if (valid(reg)) {
        entity.get<Transform>().turn_to_face_pos(
            asE(reg).get<Transform>().as2());
    }
}

Job::State WaitInQueueJob::run_state_initialize(Entity& entity, float) {
    log_warn("starting a new wait in queue job");

    // Figure out which register to go to...

    std::vector<std::shared_ptr<Entity>> all_registers =
        EntityHelper::getAllWithComponent<HasWaitingQueue>();

    // Find the register with the least people on it
    std::shared_ptr<Entity> best_target;
    int best_pos = -1;
    for (auto r : all_registers) {
        HasWaitingQueue& hwq = r->get<HasWaitingQueue>();
        if (hwq.is_full()) continue;
        int rpos = hwq.get_next_pos();
        if (best_pos == -1 || rpos < best_pos) {
            best_target = r;
            best_pos = rpos;
        }
    }

    if (!best_target) {
        log_warn("Could not find a valid register");
        WIQ_wait_and_return(entity);
        return Job::State::Initialize;
    }

    VALIDATE(best_target, "underlying job should contain a register now");

    // Store into our job data
    reg_id = best_target->id;

    start = WIQ_add_to_queue_and_get_position(*best_target, entity);
    end = best_target->get<Transform>().tile_infront(1);

    spot_in_line = WIQ_position_in_line(*best_target, entity);

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

    if (cur_spot_in_line == spot_in_line || !WIQ_can_move_up(reg, entity)) {
        // We didnt move so just wait a bit before trying again
        system_manager::logging_manager::announce(
            entity, fmt::format("im just going to wait a bit longer"));

        log_info("wait and return");

        // Add the current job to the queue,
        // and then add the waiting job
        WIQ_wait_and_return(entity);
        return (Job::State::WorkingAtStart);
    }

    // if our spot did change, then move forward
    system_manager::logging_manager::announce(
        entity, fmt::format("im moving up to {}", cur_spot_in_line));

    spot_in_line = cur_spot_in_line;

    if (spot_in_line == 0) {
        return (Job::State::HeadingToEnd);
    }

    // otherwise walk up one spot
    start = reg.get<Transform>().tile_infront(spot_in_line);
    return (Job::State::WorkingAtStart);
}

Job::State WaitInQueueJob::run_state_working_at_end(Entity& entity, float) {
    VALIDATE(reg_id != -1, "workingatend job should contain register");
    Entity& reg = get_and_validate_entity(reg_id);

    // TODO safer way to do it?
    // we are at the front so turn it on
    entity.get<HasSpeechBubble>().on();
    CanOrderDrink& canOrderDrink = entity.get<CanOrderDrink>();

    if (canOrderDrink.order_state == CanOrderDrink::OrderState::NeedsReset) {
        system_manager::logging_manager::announce(
            entity, "I havent decided what i want yet");
        return (Job::State::WorkingAtEnd);
    }

    VALIDATE(canOrderDrink.has_order(), "I should have an order");

    CanHoldItem& regCHI = reg.get<CanHoldItem>();

    if (regCHI.empty()) {
        system_manager::logging_manager::announce(entity,
                                                  "my drink isnt ready yet");
        WIQ_wait_and_return(entity);
        return (Job::State::WorkingAtEnd);
    }

    std::shared_ptr<Item> drink = reg.get<CanHoldItem>().item();
    if (!drink || !check_type(*drink, EntityType::Drink)) {
        system_manager::logging_manager::announce(entity, "this isnt a drink");
        WIQ_wait_and_return(entity);
        return (Job::State::WorkingAtEnd);
    }

    system_manager::logging_manager::announce(entity, "i got **A** drink ");

    // TODO how many ingredients have to be correct?
    // as people get more drunk they should care less and less

    bool all_ingredients_match =
        drink->get<IsDrink>().matches_recipe(canOrderDrink.recipe());
    if (!all_ingredients_match) {
        system_manager::logging_manager::announce(entity,
                                                  "this isnt what i ordered");
        WIQ_wait_and_return(entity);
        return (Job::State::WorkingAtEnd);
    }

    // I'm relatively happy with my drink

    canOrderDrink.order_state = CanOrderDrink::OrderState::DrinkingNow;

    CanHoldItem& ourCHI = entity.get<CanHoldItem>();
    ourCHI.update(regCHI.item());
    regCHI.update(nullptr);

    system_manager::logging_manager::announce(entity, "got it");
    WIQ_leave_line(reg, entity);

    // Now that we are done and got our item, time to leave the store
    {
        std::shared_ptr<Job> jshared;
        jshared.reset(create_drinking_job(entity.get<Transform>().as2()));
        entity.get<CanPerformJob>().push_onto_queue(jshared);
    }
    entity.get<HasSpeechBubble>().off();
    return (Job::State::Completed);
}

Job::State WaitJob::run_state_working_at_end(Entity& entity, float dt) {
    timePassedInCurrentState += dt;
    if (timePassedInCurrentState >= timeToComplete) {
        return (Job::State::Completed);
    }
    system_manager::logging_manager::announce(
        entity, fmt::format("waiting a little longer: {} => {} ",
                            timePassedInCurrentState, timeToComplete));

    return (Job::State::WorkingAtEnd);
}

Job::State LeavingJob::run_state_working_at_end(Entity& entity, float) {
    // Now that we are done and got our item, time to leave the store
    {
        vec2 start = entity.get<Transform>().as2();
        std::shared_ptr<Job> jshared = std::make_shared<WaitJob>(
            start,
            // TODO create a global so they all leave to the same spot
            vec2{GATHER_SPOT, GATHER_SPOT},
            // TODO replace with remaining round time so they dont come back
            90.f);
        entity.get<CanPerformJob>().push_onto_queue(jshared);
    }
    return (Job::State::Completed);
}

Job::State DrinkingJob::run_state_working_at_end(Entity& entity, float dt) {
    CanOrderDrink& cod = entity.get<CanOrderDrink>();
    cod.order_state = CanOrderDrink::OrderState::DrinkingNow;
    entity.get<HasSpeechBubble>().on();

    timePassedInCurrentState += dt;
    if (timePassedInCurrentState >= timeToComplete) {
        // Done with my drink, delete it

        CanHoldItem& chi = entity.get<CanHoldItem>();
        chi.item()->cleanup = true;
        chi.update(nullptr);

        entity.get<HasSpeechBubble>().off();

        cod.num_orders_rem--;
        cod.num_orders_had++;

        if (cod.num_orders_rem > 0) {
            cod.order_state = CanOrderDrink::OrderState::Ordering;

            // get next order
            {
                std::shared_ptr<Entity> sophie =
                    EntityHelper::getFirstWithComponent<IsProgressionManager>();
                VALIDATE(sophie, "sophie should exist for sure");

                const IsProgressionManager& progressionManager =
                    sophie->get<IsProgressionManager>();
                cod.current_order = progressionManager.get_random_drink();
            }

            vec2 start = entity.get<Transform>().as2();
            std::shared_ptr<Job> jshared;
            jshared.reset(
                create_job_of_type(start, start, JobType::WaitInQueue));
            entity.get<CanPerformJob>().push_onto_queue(jshared);
        } else {
            // TODO why do they go back to the register before leaving?
            cod.order_state = CanOrderDrink::OrderState::DoneDrinking;
            std::shared_ptr<Job> jshared = std::make_shared<WaitJob>(
                start,
                // TODO create a global so they all leave to the same spot
                vec2{GATHER_SPOT, GATHER_SPOT},
                // TODO replace with remaining round time so they dont come back
                90.f);
            entity.get<CanPerformJob>().push_onto_queue(jshared);
        }
        return (Job::State::Completed);
    }
    return (Job::State::WorkingAtEnd);
}

Job::State MoppingJob::run_state_initialize(Entity& entity, float) {
    log_warn("starting a new mop job");

    // Find the closest vomit
    std::shared_ptr<Entity> closest =
        EntityHelper::getClosestOfType(entity, EntityType::Vomit);

    if (!closest) {
        log_warn("Could not find any vomit");
        // TODO make this function name more generic / obvious its shared
        WIQ_wait_and_return(entity);
        return Job::State::Initialize;
    }

    VALIDATE(closest, "underlying job should contain vomit now");
    vom_id = closest->id;
    OptEntity opt_vom = EntityHelper::getEntityForID(vom_id);
    VALIDATE(valid(opt_vom), "underlying job should contain vomit now");
    Entity& vom = asE(opt_vom);

    start = vom.get<Transform>().as2();
    end = start;

    system_manager::logging_manager::announce(
        entity, fmt::format("heading to vomit {}", start));

    return Job::State::HeadingToStart;
}

Job::State MoppingJob::run_state_working_at_start(Entity& entity, float dt) {
    VALIDATE(vom_id != -1, "underlying job should contain vomit now");

    OptEntity opt_vom = EntityHelper::getEntityForID(vom_id);
    if (!valid(opt_vom)) {
        system_manager::logging_manager::announce(
            entity, fmt::format("seems like someone beat me to it"));
        return (Job::State::HeadingToEnd);
    }
    Entity& vom = asE(opt_vom);
    HasWork& vomWork = vom.get<HasWork>();

    // do some work
    vomWork.call(vom, entity, dt);

    // check if we did it
    bool cleaned_up = vom.cleanup;

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
