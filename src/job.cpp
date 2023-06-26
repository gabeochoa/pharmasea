
#include "job.h"

#include "components/has_waiting_queue.h"
#include "engine/assert.h"
#include "entity.h"
#include "entityhelper.h"
#include "system/logging_system.h"

HasWaitingQueue& HasWaitingQueue::add_customer(
    const std::shared_ptr<Entity>& customer) {
    log_warn("hi we are adding {} {} to the line in position {}", customer->id,
             customer->get<DebugName>().name(), next_line_position);
    ppl_in_line[next_line_position++] = customer;
    return *this;
}

bool HasWaitingQueue::matching_person(int id, int i) const {
    std::shared_ptr<Entity> person = ppl_in_line[i];
    VALIDATE(person, "person in line is messed up");
    return person->id == id;
}

inline Job* create_wandering_job(std::shared_ptr<Entity> entity) {
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
    return new WanderingJob(entity->get<Transform>().as2(), target);
}

Job* Job::create_job_of_type(const std::shared_ptr<Entity>& entity, float dt,
                             JobType job_type) {
    Job* job = nullptr;
    switch (job_type) {
        case Wandering:
            job = create_wandering_job(entity);
            break;
        case WaitInQueue:
            job = new WaitInQueueJob();
            break;
        case Wait:
            job = new WaitJob(entity->get<Transform>().as2(),
                                  entity->get<Transform>().as2(), 1.f);
            break;
        case None:
            job = new WaitJob(entity->get<Transform>().as2(),
                                  entity->get<Transform>().as2(), 1.f);
            break;
        default:
            log_warn(
                "Trying to replace job with type {}({}) but doesnt have it",
                magic_enum::enum_name(job_type), job_type);
            break;
    }
    return job;
}

bool Job::is_at_position(const std::shared_ptr<Entity>& entity, vec2 position) {
    return vec::distance(entity->get<Transform>().as2(), position) <
           (TILESIZE / 2.f);
}

Job::State Job::run_state_heading_to_start(
    const std::shared_ptr<Entity>& entity, float dt) {
    travel_to_position(entity, dt, start);
    return (is_at_position(entity, start) ? Job::State::WorkingAtStart
                                          : Job::State::HeadingToStart);
}

Job::State Job::run_state_heading_to_end(const std::shared_ptr<Entity>& entity,
                                         float dt) {
    travel_to_position(entity, dt, end);
    return (is_at_position(entity, end) ? Job::State::WorkingAtEnd
                                        : Job::State::HeadingToEnd);
}

void Job::travel_to_position(const std::shared_ptr<Entity>& entity, float dt,
                             vec2 goal) {
    // we just call this again cause its fun, be we could merge the two in
    // the future
    if (is_at_position(entity, goal)) {
        system_manager::logging_manager::announce(
            entity,
            fmt::format("no need to travel we are already at the goal {}", 1));
        return;
    }

    const auto _grab_path_to_goal = [this, goal, entity]() {
        if (!path_empty()) return;

        vec2 me = entity->get<Transform>().as2();

        auto new_path = astar::find_path(
            me, goal,
            std::bind(EntityHelper::isWalkable, std::placeholders::_1));

        update_path(new_path);

        system_manager::logging_manager::announce(
            entity, fmt::format("gen path from {} to {} with {} steps", me,
                                goal, p_size()));

        // what happens if we get here and the path is still empty?
        VALIDATE(!path_empty(), "path should no longer be empty");
    };

    const auto _grab_local_target = [this, entity]() {
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

    const auto _move_toward_local_target = [this, entity, dt]() {
        // TODO forcing get<HasBaseSpeed> to crash here
        float speed = entity->get<HasBaseSpeed>().speed() * dt;

        // TODO Turning off stagger stuff for easier development
        // handle these once normal movement is working
        if (0) {
            float speed_multiplier = 1.f;
            float stagger_multiplier = 0.f;
            if (entity->has<CanHaveAilment>()) {
                const CanHaveAilment& cha = entity->get<CanHaveAilment>();
                stagger_multiplier = cha.ailment()->stagger();
                speed_multiplier = cha.ailment()->speed_multiplier();
            }

            if (speed_multiplier != 0) speed *= speed_multiplier;
            if (stagger_multiplier != 0) speed *= stagger_multiplier;
        }

        Transform& transform = entity->get<Transform>();

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

inline void WIQ_wait_and_return(const std::shared_ptr<Entity>& entity) {
    CanPerformJob& cpj = entity->get<CanPerformJob>();
    const Job& job = entity->get<CanPerformJob>().job();
    // Add the current job to the queue,
    // and then add the waiting job
    cpj.push_and_reset(new WaitJob(job.start, job.start, 1.f));
    return;
}

inline vec2 WIQ_add_to_queue_and_get_position(
    const std::shared_ptr<Entity>& reg,
    const std::shared_ptr<Entity>& customer) {
    VALIDATE(reg->has<HasWaitingQueue>(),
             "Trying to get-next-queue-pos for entity which doesnt have a "
             "waiting queue ");
    VALIDATE(customer, "entity passed to register queue should not be null");

    int next_position = reg->get<HasWaitingQueue>()
                            .add_customer(customer)  //
                            .get_next_pos();

    // the place the customers stand is 1 tile infront of the register
    return reg->get<Transform>().tile_infront_given_player((next_position + 1) *
                                                           2);
}

inline int WIQ_position_in_line(const std::shared_ptr<Entity>& reg,
                                const std::shared_ptr<Entity>& customer) {
    VALIDATE(customer, "entity passed to position-in-line should not be null");
    VALIDATE(reg, "entity passed to position-in-line should not be null");
    VALIDATE(
        reg->has<HasWaitingQueue>(),
        "Trying to pos-in-line for entity which doesnt have a waiting queue ");

    const HasWaitingQueue& hwq = reg->get<HasWaitingQueue>();

    for (int i = 0; i < hwq.get_next_pos(); i++) {
        if (hwq.matching_person(customer->id, i)) return i;
    }
    log_warn("Cannot find customer {}", customer->id);
    return -1;
}

inline void WIQ_leave_line(const std::shared_ptr<Entity>& reg,
                           const std::shared_ptr<Entity>& customer) {
    VALIDATE(customer, "entity passed to leave-line should not be null");
    VALIDATE(reg, "register passed to leave-line should not be null");
    VALIDATE(
        reg->has<HasWaitingQueue>(),
        "Trying to leave-line for entity which doesnt have a waiting queue ");

    int pos = WIQ_position_in_line(reg, customer);
    if (pos == -1) return;

    reg->get<HasWaitingQueue>().erase(pos);
}

inline bool WIQ_can_move_up(const std::shared_ptr<Entity>& reg,
                            const std::shared_ptr<Entity>& customer) {
    VALIDATE(customer, "entity passed to can-move-up should not be null");
    VALIDATE(reg->has<HasWaitingQueue>(),
             "Trying to can-move-up for entity which doesnt "
             "have a waiting queue ");
    return reg->get<HasWaitingQueue>().matching_person(customer->id, 0);
}

Job::State WaitInQueueJob::run_state_initialize(
    const std::shared_ptr<Entity>& entity, float) {
    log_warn("starting a new wait in queue job");

    // Figure out which register to go to...

    // TODO replace with finding the one with the least people in it
    std::shared_ptr<Furniture> closest_target =
        EntityHelper::getClosestWithComponent<HasWaitingQueue>(
            entity, TILESIZE * 100.f);

    // TODO when you place a register we need to make sure you cant
    // start the game until it has an empty spot infront of it
    //
    if (!closest_target) {
        // TODO we need some kinda way to save this job,
        // and come back to it later
        // i think just putting a Job* unfinished in Job is
        // probably enough
        log_warn("Could not find a valid register");

        return Job::State::Completed;
        // TODO right now this doesnt work, the job queue sucks
        // cpj.update_job_state(Job::State::Initialize);
        // WIQ_wait_and_return(entity);
        // return Job::State::Initialize;
    }

    reg = closest_target;
    VALIDATE(reg, "underlying job should contain a register now");

    start = WIQ_add_to_queue_and_get_position(closest_target, entity);
    end = closest_target->get<Transform>().tile_infront_given_player(1);

    spot_in_line = WIQ_position_in_line(closest_target, entity);

    VALIDATE(spot_in_line >= 0, "customer should be in line right now");

    return Job::State::HeadingToStart;
}

Job::State WaitInQueueJob::run_state_working_at_start(
    const std::shared_ptr<Entity>& entity, float) {
    if (spot_in_line == 0) {
        return (Job::State::HeadingToEnd);
    }

    VALIDATE(reg, "workingatstart job should contain register");
    // Check the spot in front of us
    int cur_spot_in_line = WIQ_position_in_line(reg, entity);

    if (cur_spot_in_line == spot_in_line || !WIQ_can_move_up(reg, entity)) {
        // We didnt move so just wait a bit before trying again
        system_manager::logging_manager::announce(
            entity, fmt::format("im just going to wait a bit longer"));

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
    start = reg->get<Transform>().tile_infront_given_player(spot_in_line);
    return (Job::State::WorkingAtStart);
}

Job::State WaitInQueueJob::run_state_working_at_end(
    const std::shared_ptr<Entity>& entity, float dt) {
    VALIDATE(reg, "workingatend job should contain register");

    CanHoldItem& regCHI = reg->get<CanHoldItem>();

    if (regCHI.empty()) {
        system_manager::logging_manager::announce(entity,
                                                  "my rx isnt ready yet");
        WIQ_wait_and_return(entity);
        return (Job::State::WorkingAtEnd);
    }

    auto bag = reg->get<CanHoldItem>().asT<Bag>();
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

    CanHoldItem& ourCHI = entity->get<CanHoldItem>();
    ourCHI.update(regCHI.item());
    regCHI.update(nullptr);

    system_manager::logging_manager::announce(entity, "got it");
    WIQ_leave_line(reg, entity);
    return (Job::State::Completed);
}

Job::State WaitJob::run_state_working_at_end(const std::shared_ptr<Entity>& entity,
                                          float dt) {
    timePassedInCurrentState += dt;
    if (timePassedInCurrentState >= timeToComplete) {
        return (Job::State::Completed);
    }
    // announce(fmt::format("waiting a little longer: {} => {} ",
    // job->timePassedInCurrentState,
    // job->timeToComplete));
    return (Job::State::WorkingAtEnd);
}
