

#pragma once

#include "../components/can_be_ghost_player.h"
#include "../components/can_highlight_others.h"
#include "../components/can_hold_furniture.h"
#include "../components/can_perform_job.h"
#include "../components/custom_item_position.h"
#include "../components/transform.h"
#include "../entity.h"
#include "../entityhelper.h"
#include "../vendor_include.h"
#include "logging_system.h"

namespace system_manager {

namespace job_system {

[[nodiscard]] inline bool is_at_position(const std::shared_ptr<Entity>& entity,
                                         vec2 position) {
    return vec::distance(entity->get<Transform>().as2(), position) <
           (TILESIZE / 2.f);
}

inline void travel_to_position(const std::shared_ptr<Entity>& entity, float dt,
                               vec2 goal) {
    // we just call this again cause its fun, be we could merge the two in the
    // future
    if (is_at_position(entity, goal)) {
        logging_manager::announce(
            entity,
            fmt::format("no need to travel we are already at the goal {}", 1));
        return;
    }

    const auto _grab_path_to_goal = [goal, entity]() {
        CanPerformJob& cpj = entity->get<CanPerformJob>();
        if (!cpj.job().path_empty()) return;

        vec2 me = entity->get<Transform>().as2();

        auto path = astar::find_path(
            me, goal,
            std::bind(EntityHelper::isWalkable, std::placeholders::_1));

        logging_manager::announce(
            entity, fmt::format("gen path from {} to {} with {} steps", me,
                                goal, path.size()));

        cpj.mutable_job()->update_path(path);

        // what happens if we get here and the path is still empty?
        VALIDATE(!cpj.job().path_empty(), "path should no longer be empty");
    };

    const auto _grab_local_target = [entity]() {
        CanPerformJob& cpj = entity->get<CanPerformJob>();

        // Either we dont yet have a local target
        // or we already reached the one we had

        if (cpj.has_local_target()) {
            if (!is_at_position(entity, cpj.local_target())) {
                return;
            }
        }

        cpj.grab_job_local_target();

        VALIDATE(cpj.has_local_target(), "job should have a local target");
    };

    const auto _move_toward_local_target = [entity, dt]() {
        CanPerformJob& cpj = entity->get<CanPerformJob>();
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

        vec2 tar = cpj.local_target();
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
    cpj.push_and_reset(new Job({
        .type = Wait,
        .timeToComplete = 1.f,
        .start = job.start,
        .end = job.start,
    }));
    return;
}

inline vec2 WIQ_get_next_queue_position(
    const std::shared_ptr<Entity>& reg,
    const std::shared_ptr<Entity>& customer) {
    VALIDATE(customer, "entity passed to register queue should not be null");
    VALIDATE(reg->has<HasWaitingQueue>(),
             "Trying to get-next-queue-pos for entity which doesnt have a "
             "waiting queue ");
    HasWaitingQueue& hasWaitingQueue = reg->get<HasWaitingQueue>();
    hasWaitingQueue.add_customer(customer);
    // the place the customers stand is 1 tile infront of the register
    auto front = reg->get<Transform>().tile_infront_given_player(
        (hasWaitingQueue.next_line_position + 1) * 2);
    hasWaitingQueue.next_line_position++;
    return front;
}

inline int WIQ_position_in_line(const std::shared_ptr<Entity>& reg,
                                const std::shared_ptr<Entity>& customer) {
    VALIDATE(customer, "entity passed to position-in-line should not be null");
    VALIDATE(
        reg->has<HasWaitingQueue>(),
        "Trying to pos-in-line for entity which doesnt have a waiting queue ");
    const auto& ppl_in_line = reg->get<HasWaitingQueue>().ppl_in_line;

    for (int i = 0; i < (int) ppl_in_line.size(); i++) {
        if (customer->id == ppl_in_line[i]->id) return i;
    }
    log_warn("Cannot find customer {}", customer->id);
    return -1;
}

inline void WIQ_leave_line(const std::shared_ptr<Entity>& reg,
                           const std::shared_ptr<Entity>& customer) {
    VALIDATE(customer, "entity passed to leave-line should not be null");
    VALIDATE(
        reg->has<HasWaitingQueue>(),
        "Trying to leave-line for entity which doesnt have a waiting queue ");
    auto& ppl_in_line = reg->get<HasWaitingQueue>().ppl_in_line;
    int pos = WIQ_position_in_line(reg, customer);
    if (pos == -1) return;
    if (pos == 0) {
        ppl_in_line.pop_front();
        return;
    }
    ppl_in_line.erase(ppl_in_line.begin() + pos);
}

inline bool WIQ_can_move_up(const std::shared_ptr<Entity>& reg,
                            const std::shared_ptr<Entity>& customer) {
    VALIDATE(customer, "entity passed to can-move-up should not be null");
    VALIDATE(reg->has<HasWaitingQueue>(),
             "Trying to can-move-up for entity which doesnt "
             "have a waiting queue ");
    const auto& ppl_in_line = reg->get<HasWaitingQueue>().ppl_in_line;
    return customer->id == ppl_in_line.front()->id;
}

inline void run_job_wait_in_queue(const std::shared_ptr<Entity>& entity,
                                  float dt) {
    CanPerformJob& cpj = entity->get<CanPerformJob>();

    switch (cpj.job().state) {
        case Job::State::Initialize: {
            logging_manager::announce(entity,
                                      "starting a new wait in queue job");

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
                logging_manager::announce(entity,
                                          "Could not find a valid register");

                cpj.update_job_state(Job::State::Completed);
                // TODO right now this doesnt work, the job queue sucks
                // cpj.update_job_state(Job::State::Initialize);
                // WIQ_wait_and_return(entity);
                return;
            }

            std::shared_ptr<Job> mutable_job = cpj.mutable_job();

            mutable_job->data.insert({"register", closest_target.get()});
            VALIDATE(cpj.job().data.contains("register"),
                     "underlying job should contain a register now");
            mutable_job->start =
                WIQ_get_next_queue_position(closest_target, entity);
            mutable_job->end =
                closest_target->get<Transform>().tile_infront_given_player(1);
            mutable_job->spot_in_line =
                WIQ_position_in_line(closest_target, entity);
            cpj.update_job_state(Job::State::HeadingToStart);
            return;
        }
        case Job::State::HeadingToStart: {
            travel_to_position(entity, dt, cpj.job().start);
            cpj.update_job_state(is_at_position(entity, cpj.job().start)
                                     ? Job::State::WorkingAtStart
                                     : Job::State::HeadingToStart);
            return;
        }
        case Job::State::WorkingAtStart: {
            if (cpj.job().spot_in_line == 0) {
                cpj.update_job_state(Job::State::HeadingToEnd);
                return;
            }

            VALIDATE(cpj.job().data.contains("register"),
                     "workingatstart job should contain register");
            // Check the spot in front of us
            auto reg = std::shared_ptr<Entity>(
                static_cast<Entity*>(cpj.job().data.at("register")));
            int cur_spot_in_line = WIQ_position_in_line(reg, entity);

            if (cur_spot_in_line == cpj.job().spot_in_line ||
                !WIQ_can_move_up(reg, entity)) {
                // We didnt move so just wait a bit before trying again
                logging_manager::announce(
                    entity, fmt::format("im just going to wait a bit longer"));

                // Add the current job to the queue,
                // and then add the waiting job
                cpj.update_job_state(Job::State::WorkingAtStart);
                WIQ_wait_and_return(entity);
                return;
            }

            // if our spot did change, then move forward
            logging_manager::announce(
                entity, fmt::format("im moving up to {}", cur_spot_in_line));

            cpj.mutable_job()->spot_in_line = cur_spot_in_line;

            if (cpj.job().spot_in_line == 0) {
                cpj.update_job_state(Job::State::HeadingToEnd);
                return;
            }

            // otherwise walk up one spot
            cpj.mutable_job()->start =
                reg->get<Transform>().tile_infront_given_player(
                    cpj.job().spot_in_line);
            cpj.update_job_state(Job::State::WorkingAtStart);

            return;
        }
        case Job::State::HeadingToEnd: {
            travel_to_position(entity, dt, cpj.job().end);
            cpj.update_job_state(is_at_position(entity, cpj.job().end)
                                     ? Job::State::WorkingAtEnd
                                     : Job::State::HeadingToEnd);
            return;
        }
        case Job::State::WorkingAtEnd: {
            VALIDATE(cpj.job().data.contains("register"),
                     "workingatend job should contain register");
            auto reg = std::shared_ptr<Entity>(
                static_cast<Entity*>(cpj.job().data.at("register")));

            CanHoldItem& regCHI = reg->get<CanHoldItem>();

            if (regCHI.empty()) {
                logging_manager::announce(entity, "my rx isnt ready yet");
                WIQ_wait_and_return(entity);
                return;
            }

            auto bag = reg->get<CanHoldItem>().asT<Bag>();
            if (!bag) {
                logging_manager::announce(entity,
                                          "this isnt my rx (not a bag)");
                WIQ_wait_and_return(entity);
                return;
            }

            if (bag->empty()) {
                logging_manager::announce(entity, "this bag is empty...");
                WIQ_wait_and_return(entity);
                return;
            }

            // TODO eventually migrate item to ECS
            // auto pill_bottle =
            // bag->get<CanHoldItem>().asT<PillBottle>();
            auto pill_bottle = dynamic_pointer_cast<PillBottle>(bag->held_item);
            if (!pill_bottle) {
                logging_manager::announce(entity,
                                          "this bag doesnt have my pills");
                WIQ_wait_and_return(entity);
                return;
            }

            CanHoldItem& ourCHI = entity->get<CanHoldItem>();
            ourCHI.update(regCHI.item());
            regCHI.update(nullptr);

            logging_manager::announce(entity, "got it");
            WIQ_leave_line(reg, entity);
            cpj.update_job_state(Job::State::Completed);
            return;
        }
        case Job::State::Completed: {
            return;
        }
    }
}

/*
inline void handle_job_holder_pushed(std::shared_ptr<Entity> entity, float) {
    if (entity->is_missing<CanPerformJob>()) return;
    CanPerformJob& cpf = entity->get<CanPerformJob>();
    if (!cpf.has_job()) return;
    auto job = cpf.job();

    const CanBePushed& cbp = entity->get<CanBePushed>();

    if (cbp.pushed_force().x != 0.0f || cbp.pushed_force().z != 0.0f) {
        job->path.clear();
        job->local = {};
        SoundLibrary::get().play("roblox");
    }
}

*/

inline void run_job_wait(const std::shared_ptr<Entity>& entity, float dt) {
    CanPerformJob& cpj = entity->get<CanPerformJob>();
    const Job& job = cpj.job();

    switch (job.state) {
        case Job::State::Initialize: {
            logging_manager::announce(entity, "starting a new wait job");
            cpj.update_job_state(Job::State::HeadingToStart);
            return;
        }
        case Job::State::HeadingToStart: {
            cpj.update_job_state(Job::State::WorkingAtStart);
            return;
        }
        case Job::State::WorkingAtStart: {
            cpj.update_job_state(Job::State::HeadingToEnd);
            return;
        }
        case Job::State::HeadingToEnd: {
            travel_to_position(entity, dt, job.end);
            cpj.update_job_state(is_at_position(entity, job.end)
                                     ? Job::State::WorkingAtEnd
                                     : Job::State::HeadingToEnd);
            return;
        }
        case Job::State::WorkingAtEnd: {
            cpj.mutable_job()->timePassedInCurrentState += dt;
            if (cpj.mutable_job()->timePassedInCurrentState >=
                cpj.mutable_job()->timeToComplete) {
                cpj.update_job_state(Job::State::Completed);
                return;
            }
            // announce(fmt::format("waiting a little longer: {} => {} ",
            // job->timePassedInCurrentState,
            // job->timeToComplete));
            cpj.update_job_state(Job::State::WorkingAtEnd);
            return;
        }
        case Job::State::Completed: {
            cpj.update_job_state(Job::State::Completed);
            return;
        }
    }
}

inline void render_job_visual(std::shared_ptr<Entity> entity, float) {
    if (entity->is_missing<CanPerformJob>()) return;
    CanPerformJob& cpf = entity->get<CanPerformJob>();
    if (!cpf.has_job()) return;

    const float box_size = TILESIZE / 10.f;
    if (!cpf.job().path.empty()) {
        for (auto location : cpf.job().path) {
            DrawCube(vec::to3(location), box_size, box_size, box_size, BLUE);
        }
    }
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
    return new Job({.type = Wandering,
                    .start = entity->get<Transform>().as2(),
                    .end = target});
}

inline Job* create_job_of_type(std::shared_ptr<Entity> entity, float,
                               JobType job_type) {
    Job* job = nullptr;
    switch (job_type) {
        case Wandering:
            job = create_wandering_job(entity);
            break;
        case WaitInQueue:
            job = new Job({
                .type = WaitInQueue,
            });
            break;
        case Wait:
            job = new Job({
                .type = Wait,
                .timeToComplete = 1.f,
                .start = entity->get<Transform>().as2(),
                .end = entity->get<Transform>().as2(),
            });
            break;
        case None:
            job = new Job({
                .type = None,
                .timeToComplete = 1.f,
                .start = entity->get<Transform>().as2(),
                .end = entity->get<Transform>().as2(),
            });
            break;
        default:
            log_warn(
                "Trying to replace job with type {}({}) but doesnt have it",
                magic_enum::enum_name(job_type), job_type);
            break;
    }
    return job;
}

inline void ensure_has_job(std::shared_ptr<Entity> entity, float dt) {
    if (entity->is_missing<CanPerformJob>()) return;
    CanPerformJob& cpj = entity->get<CanPerformJob>();
    if (cpj.has_job()) return;

    auto& personal_queue = cpj.job_queue();
    if (personal_queue.empty()) {
        // No job and nothing in the queue? grab the next default one then
        std::shared_ptr<Job> njob;
        cpj.update(create_job_of_type(entity, dt, cpj.get_next_job_type()));

        // TODO i really want to not return right here but the job is
        // nullptr if i do
        return;
    }

    VALIDATE(!personal_queue.empty(),
             "no way personal job queue should be empty");
    // queue should definitely have something by now
    // add the thing from the queue as our job :)
    cpj.update(personal_queue.top().get());
    personal_queue.pop();
}

inline void run_job_wandering(const std::shared_ptr<Entity>& entity, float dt) {
    CanPerformJob& cpj = entity->get<CanPerformJob>();
    const Job& job = cpj.job();

    // logging_manager::announce(entity,
    // fmt::format("wandering job: state {} ",
    // magic_enum::enum_name(job.state)));

    switch (job.state) {
        default:
            log_error("Wandering did not handle state {}",
                      magic_enum::enum_name(job.state));
            return;
        case Job::State::Initialize: {
            logging_manager::announce(
                entity,
                fmt::format("starting wandering job: me {} start {} end {}",
                            entity->get<Transform>().pos(), job.start,
                            job.end));
            cpj.update_job_state(Job::State::HeadingToStart);

            return;
        }
        case Job::State::HeadingToStart: {
            travel_to_position(entity, dt, job.start);
            cpj.update_job_state(is_at_position(entity, job.start)
                                     ? Job::State::WorkingAtStart
                                     : Job::State::HeadingToStart);
            return;
        }
        case Job::State::WorkingAtStart: {
            cpj.update_job_state(Job::State::HeadingToEnd);
            return;
        }
        case Job::State::HeadingToEnd: {
            travel_to_position(entity, dt, job.end);
            cpj.update_job_state(is_at_position(entity, job.end)
                                     ? Job::State::WorkingAtEnd
                                     : Job::State::HeadingToEnd);
            return;
        }
        case Job::State::WorkingAtEnd: {
            cpj.update_job_state(Job::State::Completed);
            return;
        }
        case Job::State::Completed: {
            cpj.update_job_state(Job::State::Completed);
            return;
        }
    }
}

inline void run_job_tick(const std::shared_ptr<Entity>& entity, float dt) {
    if (entity->is_missing<CanPerformJob>()) return;

    const CanPerformJob& cpj = entity->get<CanPerformJob>();
    if (cpj.needs_job()) return;

    switch (cpj.job().type) {
        case None:
            // TODO eventually handle this
            // log_info("you have a guy {} that is doing a none job",
            // entity->id);
            break;
        case Wandering:
            run_job_wandering(entity, dt);
            break;
        case Wait:
            run_job_wait(entity, dt);
            break;
        case WaitInQueue:
            run_job_wait_in_queue(entity, dt);
            break;
        default:
            log_error("you arent handling one of the job types {} {}",
                      ((int) cpj.job().type),
                      magic_enum::enum_name(cpj.job().type));
            break;
    }
}

inline void cleanup_completed_job(const std::shared_ptr<Entity>& entity,
                                  float) {
    if (entity->is_missing<CanPerformJob>()) return;
    CanPerformJob& cpj = entity->get<CanPerformJob>();
    if (cpj.needs_job()) return;
    if (cpj.job().state == Job::State::Completed) {
        cpj.update(nullptr);
        log_warn("cpj, job complete");
    }
}

inline void in_round_update(const std::shared_ptr<Entity>& entity, float dt) {
    cleanup_completed_job(entity, dt);
    ensure_has_job(entity, dt);
    run_job_tick(entity, dt);
}

}  // namespace job_system

}  // namespace system_manager
