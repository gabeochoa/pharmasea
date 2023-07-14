
#include "job.h"

#include "components/can_have_ailment.h"
#include "components/can_hold_item.h"
#include "components/can_perform_job.h"
#include "components/has_base_speed.h"
#include "components/has_speech_bubble.h"
#include "components/has_waiting_queue.h"
#include "engine/assert.h"
#include "engine/astar.h"
#include "entity.h"
#include "entityhelper.h"
#include "globals.h"
#include "system/logging_system.h"

HasWaitingQueue& HasWaitingQueue::add_customer(Entity& customer) {
    log_info("we are adding {} {} to the line in position {}", customer.id,
             customer.get<DebugName>().name(), next_line_position);
    ppl_in_line[next_line_position] = {.id = customer.id};
    next_line_position++;

    VALIDATE(has_matching_person(customer.id) >= 0,
             "Customer should be in line somewhere");
    return *this;
}

void HasWaitingQueue::dump_contents() const {
    log_info("dumping contents of ppl_in_line");
    for (int i = 0; i < MAX_QUEUE_SIZE; i++) {
        log_info("index: {}, set? {}, id {}", i, !!ppl_in_line[i],
                 ppl_in_line[i]);
    }
}

bool HasWaitingQueue::matching_id(int id, int i) const {
    return ppl_in_line[i] == id;
}

bool HasWaitingQueue::has_matching_person(int id) const {
    for (int i = 0; i < MAX_QUEUE_SIZE; i++) {
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
        default:
            log_warn(
                "Trying to replace job with type {}({}) but doesnt have it",
                magic_enum::enum_name(job_type), job_type);
            break;
    }
    return job;
}

bool Job::is_at_position(Entity& entity, vec2 position) {
    return vec::distance(entity.get<Transform>().as2(), position) <
           (TILESIZE / 2.f);
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

    auto _grab_path_to_goal = [this, goal, entity]() {
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

    auto _grab_local_target = [this, &entity]() {
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

    auto _move_toward_local_target = [this, &entity, dt]() {
        // TODO forcing get<HasBaseSpeed> to crash here
        float base_speed = entity.get<HasBaseSpeed>().speed();

        if (entity.has<CanHaveAilment>()) {
            const CanHaveAilment& cha = entity.get<CanHaveAilment>();

            float speed_multiplier = cha.ailment()->speed_multiplier();
            if (speed_multiplier != 0) base_speed *= speed_multiplier;

            // TODO Turning off stagger; couple problems
            // - configuration is hard to reason about and mess with
            // - i really want it to cause them to move more, maybe we place
            // this in the path generation or something isntead?
            //
            // float stagger_multiplier = cha.ailment()->stagger(); if
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

    _grab_path_to_goal();
    _grab_local_target();
    _move_toward_local_target();
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

inline bool WIQ_can_move_up(Entity& reg, Entity& customer) {
    // VALIDATE(customer, "entity passed to can-move-up should not be null");
    VALIDATE(reg.has<HasWaitingQueue>(),
             "Trying to can-move-up for entity which doesnt "
             "have a waiting queue ");
    return reg.get<HasWaitingQueue>().matching_id(customer.id, 0);
}

void WaitInQueueJob::before_each_job_tick(Entity& entity, float) {
    // This runs before init so its possible theres no register at all

    // TODO support fetching from regid
    const auto reg = EntityHelper::findEntity(reg_id);
    if (!valid(reg)) return;
    entity.get<Transform>().turn_to_face_pos(asE(reg).get<Transform>().as2());
}

Job::State WaitInQueueJob::run_state_initialize(Entity& entity, float) {
    log_warn("starting a new wait in queue job");

    // Figure out which register to go to...

    auto all_registers = EntityHelper::getAllWithComponent<HasWaitingQueue>();

    // Find the register with the least people on it
    OptEntity best_target;
    int best_pos = -1;
    for (OptEntity r : all_registers) {
        if (!valid(r)) continue;
        auto& hwq = asE(r).get<HasWaitingQueue>();
        if (hwq.is_full()) continue;
        auto rpos = hwq.get_next_pos();
        if (best_pos == -1 || rpos < best_pos) {
            best_target = r;
            best_pos = rpos;
        }
    }

    if (!valid(best_target)) {
        log_warn("Could not find a valid register");
        WIQ_wait_and_return(entity);
        return Job::State::Initialize;
    }

    VALIDATE(valid(best_target),
             "underlying job should contain a register now");

    auto& reg = asE(best_target);

    // This is very important as everything is looked up later on in the job
    // based on this
    reg_id = reg.id;

    start = WIQ_add_to_queue_and_get_position(reg, entity);
    end = reg.get<Transform>().tile_infront(1);

    spot_in_line = WIQ_position_in_line(reg, entity);

    VALIDATE(spot_in_line >= 0, "customer should be in line right now");

    return Job::State::HeadingToStart;
}

Job::State WaitInQueueJob::run_state_working_at_start(Entity& entity, float) {
    if (spot_in_line == 0) {
        return (Job::State::HeadingToEnd);
    }

    auto opt_reg = EntityHelper::findEntity(reg_id);
    VALIDATE(valid(opt_reg), "workingatstart job should contain register");
    auto& reg = asE(opt_reg);
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
    auto opt_reg = EntityHelper::findEntity(reg_id);
    VALIDATE(valid(opt_reg), "workingatend job should contain register");
    auto& reg = asE(opt_reg);

    // TODO safer way to do it?
    // we are at the front so turn it on
    entity.get<HasSpeechBubble>().on();

    CanHoldItem& regCHI = reg.get<CanHoldItem>();

    if (regCHI.empty()) {
        system_manager::logging_manager::announce(entity,
                                                  "my rx isnt ready yet");
        WIQ_wait_and_return(entity);
        return (Job::State::WorkingAtEnd);
    }

    auto bag = reg.get<CanHoldItem>().asT<Bag>();
    if (!bag) {
        system_manager::logging_manager::announce(
            entity, "this isnt my rx (not a bag)");
        WIQ_wait_and_return(entity);
        return (Job::State::WorkingAtEnd);
    }

    if (bag->empty()) {
        system_manager::logging_manager::announce(entity,
                                                  "this bag is empty...");
        WIQ_wait_and_return(entity);
        return (Job::State::WorkingAtEnd);
    }

    // TODO eventually migrate item to ECS
    // auto pill_bottle =
    // bag->get<CanHoldItem>().asT<PillBottle>();

    auto pill_bottle = dynamic_pointer_cast<PillBottle>(bag->held_item);
    if (!pill_bottle) {
        system_manager::logging_manager::announce(
            entity, "this bag doesnt have my pills");
        WIQ_wait_and_return(entity);
        return (Job::State::WorkingAtEnd);
    }

    system_manager::logging_manager::announce(entity, "got the pill bottle ");

    auto pill = dynamic_pointer_cast<Pill>(pill_bottle->held_item);
    if (!pill) {
        system_manager::logging_manager::announce(
            entity, "this bottle doesnt have any pills");
        WIQ_wait_and_return(entity);
        return (Job::State::WorkingAtEnd);
    }

    // TODO Validate the actual underlying pill

    CanHoldItem& ourCHI = entity.get<CanHoldItem>();
    ourCHI.update(regCHI.item());
    regCHI.update(nullptr);

    system_manager::logging_manager::announce(entity, "got it");
    WIQ_leave_line(reg, entity);

    // Now that we are done and got our item, time to leave the store
    {
        std::shared_ptr<Job> jshared;
        jshared.reset(Job::create_job_of_type(
            entity.get<Transform>().as2(),
            // TODO just find the customer spawner and go back there...
            vec2{GATHER_SPOT, GATHER_SPOT}, JobType::Leaving));
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
        auto start = entity.get<Transform>().as2();
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
