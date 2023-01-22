

#pragma once

#include "../components/can_be_ghost_player.h"
#include "../components/can_highlight_others.h"
#include "../components/can_hold_furniture.h"
#include "../components/can_perform_job.h"
#include "../components/custom_item_position.h"
#include "../components/transform.h"
#include "../entity.h"
#include "../entityhelper.h"
#include "../furniture.h"
#include "../furniture/register.h"
#include "../person.h"

namespace system_manager {

namespace job_system {

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

inline void replace_job_with_job_type(std::shared_ptr<Entity> entity, float,
                                      JobType job_type) {
    if (!entity->has<CanPerformJob>()) return;
    CanPerformJob& cpj = entity->get<CanPerformJob>();

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
        default:
            log_warn("Trying to replace job with type {} but doesnt have it",
                     job_type);
            break;
    }

    cpj.update(job);
}

inline void find_new_job(std::shared_ptr<Entity> entity, float dt) {
    if (!entity->has<CanPerformJob>()) return;
    CanPerformJob& cpj = entity->get<CanPerformJob>();

    auto personal_queue = cpj.job_queue();
    if (personal_queue.empty()) {
        replace_job_with_job_type(entity, dt, cpj.get_next_job_type());
        return;
    }
    cpj.job() = personal_queue.top();
    personal_queue.pop();
}

inline void replace_finished_job(std::shared_ptr<Entity> entity, float dt) {
    if (!entity->has<CanPerformJob>()) return;
    CanPerformJob& cpj = entity->get<CanPerformJob>();
    std::shared_ptr<Job> job = cpj.job();

    if (job->state == Job::State::Completed) {
        if (job->on_cleanup)
            job->on_cleanup(dynamic_pointer_cast<AIPerson>(entity).get(),
                            job.get());
        job.reset();
        find_new_job(entity, dt);
        return;
    }
}

inline void update_job_information(std::shared_ptr<Entity> entity, float dt) {
    const auto travel_on_path = [entity](vec2 me) {
        auto job = entity->get<CanPerformJob>().job();
        // did not arrive
        if (job->path_empty()) return;
        // Grab next local point to go to
        if (!job->local.has_value()) {
            job->local = job->path_front();
            job->path_pop_front();
        }
        // Go to local point
        if (job->local.value() == me) {
            job->local.reset();
            entity->announce(fmt::format("reached local point {} : {} ", me,
                                         job->path_size));
        }
        return;
    };

    const auto navigate_to = [entity, travel_on_path](vec2 goal) -> bool {
        auto job = entity->get<CanPerformJob>().job();
        // if (!entity->has<Transform>()) return;
        Transform& transform = entity->get<Transform>();
        vec2 me = transform.as2();
        if (me == goal) {
            // announce("reached goal");
            return true;
        }
        if (job->path_empty()) {
            job->update_path(astar::find_path(
                me, goal,
                std::bind(EntityHelper::isWalkable, std::placeholders::_1)));
            entity->announce(
                fmt::format("generated path from {} to {} with {} steps", me,
                            goal, job->path_size));
        }
        travel_on_path(me);
        return false;
    };

    const auto wandering = [entity, navigate_to]() {
        CanPerformJob& cpj = entity->get<CanPerformJob>();
        std::shared_ptr<Job> job = cpj.job();
        auto personal_queue = entity->get<CanPerformJob>().job_queue();
        switch (job->state) {
            case Job::State::Initialize: {
                entity->announce("starting a new wandering job");
                job->state = Job::State::HeadingToStart;
                return;
            }
            case Job::State::HeadingToStart: {
                bool arrived = navigate_to(job->start);
                // TODO we cannot mutate this->job inside navigate because the
                // `job->state` below will change the new job and not the old
                // one this foot-gun might be solvable by passing in the global
                // job to the job processing function, then it wont change until
                // the next call
                if (job->path_size == 0) {
                    personal_queue.push(job);
                    job.reset(new Job({
                        .type = Wandering,
                    }));
                    return;
                }
                job->state = arrived ? Job::State::WorkingAtStart
                                     : Job::State::HeadingToStart;
                return;
            }
            case Job::State::WorkingAtStart: {
                job->state = Job::State::HeadingToEnd;
                return;
            }
            case Job::State::HeadingToEnd: {
                bool arrived = navigate_to(job->end);
                job->state = arrived ? Job::State::WorkingAtEnd
                                     : Job::State::HeadingToEnd;
                return;
            }
            case Job::State::WorkingAtEnd: {
                job->state = Job::State::Completed;
                return;
            }
            case Job::State::Completed: {
                job->state = Job::State::Completed;
                return;
            }
        }
    };

    const auto wait = [entity, navigate_to](float dt) {
        auto job = entity->get<CanPerformJob>().job();
        auto personal_queue = entity->get<CanPerformJob>().job_queue();
        switch (job->state) {
            case Job::State::Initialize: {
                entity->announce("starting a new wait job");
                job->state = Job::State::HeadingToStart;
                return;
            }
            case Job::State::HeadingToStart: {
                job->state = Job::State::WorkingAtStart;
                return;
            }
            case Job::State::WorkingAtStart: {
                job->state = Job::State::HeadingToEnd;
                return;
            }
            case Job::State::HeadingToEnd: {
                bool arrived = navigate_to(job->end);
                job->state = arrived ? Job::State::WorkingAtEnd
                                     : Job::State::HeadingToEnd;
                return;
            }
            case Job::State::WorkingAtEnd: {
                job->timePassedInCurrentState += dt;
                if (job->timePassedInCurrentState >= job->timeToComplete) {
                    job->state = Job::State::Completed;
                    return;
                }
                // announce(fmt::format("waiting a little longer: {} => {} ",
                // job->timePassedInCurrentState,
                // job->timeToComplete));
                job->state = Job::State::WorkingAtEnd;
                return;
            }
            case Job::State::Completed: {
                job->state = Job::State::Completed;
                return;
            }
        }
    };

    const auto wait_in_queue = [entity, navigate_to](float) {
        CanPerformJob& cpj = entity->get<CanPerformJob>();
        auto job = entity->get<CanPerformJob>().job();
        auto personal_queue = entity->get<CanPerformJob>().job_queue();

        auto wait_and_return = [&]() {
            // Add the current job to the queue,
            // and then add the waiting job
            cpj.push_and_reset(new Job({
                .type = Wait,
                .timeToComplete = 1.f,
                .start = job->start,
                .end = job->start,
            }));
            return;
        };

        switch (job->state) {
            case Job::State::Initialize: {
                entity->announce("starting a new wait in queue job");

                // Figure out which register to go to...

                // TODO replace with finding the one with the least people
                // in it
                std::shared_ptr<Register> closest_target =
                    EntityHelper::getClosestMatchingEntity<Register>(
                        entity->get<Transform>().as2(), TILESIZE * 100.f,
                        [](auto&&) { return true; });

                if (!closest_target) {
                    // TODO we need some kinda way to save this job,
                    // and come back to it later
                    // i think just putting a Job* unfinished in Job is
                    // probably enough
                    entity->announce("Could not find a valid register");
                    job->state = Job::State::Initialize;
                    wait_and_return();
                    return;
                }

                job->data["register"] = closest_target.get();
                Customer* me = (Customer*) entity.get();
                job->start = closest_target->get_next_queue_position(me);
                job->end = closest_target->tile_infront(1);
                job->spot_in_line = closest_target->position_in_line(me);
                job->state = Job::State::HeadingToStart;
                return;
            }
            case Job::State::HeadingToStart: {
                bool arrived = navigate_to(job->start);
                job->state = arrived ? Job::State::WorkingAtStart
                                     : Job::State::HeadingToStart;
                return;
            }
            case Job::State::WorkingAtStart: {
                if (job->spot_in_line == 0) {
                    job->state = Job::State::HeadingToEnd;
                    return;
                }

                // Check the spot in front of us
                Register* reg = static_cast<Register*>(job->data["register"]);
                Customer* me = (Customer*) entity.get();
                int cur_spot_in_line = reg->position_in_line(me);

                if (cur_spot_in_line == job->spot_in_line ||
                    !reg->can_move_up(me)) {
                    // We didnt move so just wait a bit before trying again
                    entity->announce(
                        fmt::format("im just going to wait a bit longer"));

                    // Add the current job to the queue,
                    // and then add the waiting job
                    job->state = Job::State::WorkingAtStart;
                    wait_and_return();
                    return;
                }

                // if our spot did change, then move forward
                entity->announce(
                    fmt::format("im moving up to {}", cur_spot_in_line));

                job->spot_in_line = cur_spot_in_line;

                if (job->spot_in_line == 0) {
                    job->state = Job::State::HeadingToEnd;
                    return;
                }

                // otherwise walk up one spot
                job->start = reg->tile_infront(job->spot_in_line);
                job->state = Job::State::WorkingAtStart;
                return;
            }
            case Job::State::HeadingToEnd: {
                bool arrived = navigate_to(job->end);
                job->state = arrived ? Job::State::WorkingAtEnd
                                     : Job::State::HeadingToEnd;
                return;
            }
            case Job::State::WorkingAtEnd: {
                Register* reg = (Register*) job->data["register"];

                CanHoldItem& regCHI = reg->get<CanHoldItem>();

                if (regCHI.empty()) {
                    entity->announce("my rx isnt ready yet");
                    wait_and_return();
                    return;
                }

                auto bag = reg->get<CanHoldItem>().asT<Bag>();
                if (!bag) {
                    entity->announce("this isnt my rx (not a bag)");
                    wait_and_return();
                    return;
                }

                if (bag->empty()) {
                    entity->announce("this bag is empty...");
                    wait_and_return();
                    return;
                }

                // TODO eventually migrate item to ECS
                // auto pill_bottle =
                // bag->get<CanHoldItem>().asT<PillBottle>();
                auto pill_bottle =
                    dynamic_pointer_cast<PillBottle>(bag->held_item);
                if (!pill_bottle) {
                    entity->announce("this bag doesnt have my pills");
                    wait_and_return();
                    return;
                }

                CanHoldItem& ourCHI = entity->get<CanHoldItem>();
                ourCHI.update(regCHI.item());
                regCHI.update(nullptr);

                entity->announce("got it");
                Customer* me = (Customer*) entity.get();
                reg->leave_line(me);
                job->state = Job::State::Completed;
                return;
            }
            case Job::State::Completed: {
                job->state = Job::State::Completed;
                return;
            }
        }
    };

    if (!entity->has<CanPerformJob>()) return;

    CanPerformJob& cpj = entity->get<CanPerformJob>();
    std::shared_ptr<Job> job = cpj.job();

    // User has no active job
    if (cpj.needs_job()) {
        log_info("getting new job");
        find_new_job(entity, dt);
        return;
    };

    switch (job->type) {
        case Wandering:
            wandering();
            break;
        case WaitInQueue:
            wait_in_queue(dt);
            break;
        case Wait:
            wait(dt);
            break;
        default:
            break;
    }
}

inline void handle_job_holder_pushed(std::shared_ptr<Entity> entity, float) {
    if (!entity->has<CanPerformJob>()) return;
    CanPerformJob& cpf = entity->get<CanPerformJob>();
    if (!cpf.has_job()) return;
    auto job = cpf.job();

    CanBePushed& cbp = entity->get<CanBePushed>();

    if (cbp.pushed_force.x != 0.0f || cbp.pushed_force.z != 0.0f) {
        job->path.clear();
        job->local = {};
        SoundLibrary::get().play("roblox");
    }
}

inline void update_position_from_job(std::shared_ptr<Entity> entity, float dt) {
    if (!entity->has<Transform>()) return;
    Transform& transform = entity->get<Transform>();

    if (!entity->has<CanPerformJob>()) return;
    CanPerformJob& cpf = entity->get<CanPerformJob>();
    if (!cpf.has_job()) return;
    auto job = cpf.job();

    if (!job || !job->local.has_value()) {
        return;
    }

    vec2 tar = job->local.value();

    if (!entity->has<HasBaseSpeed>()) return;
    const HasBaseSpeed& hasBaseSpeed = entity->get<HasBaseSpeed>();

    float speed = hasBaseSpeed.speed() * dt;

    float speed_multiplier = 1.f;
    float stagger_multiplier = 0.f;
    if (entity->has<CanHaveAilment>()) {
        const CanHaveAilment& cha = entity->get<CanHaveAilment>();
        stagger_multiplier = cha.ailment()->stagger();
        speed_multiplier = cha.ailment()->speed_multiplier();
    }

    if (speed_multiplier != 0) speed *= speed_multiplier;
    if (stagger_multiplier != 0) speed *= stagger_multiplier;

    // this was moved when doing ecs, idk if its still true anymore
    // TODO we are seeing issues where customers are getting stuck on corners
    // when turning. Before I feel like they were able to slide but it seems
    // like not anymore?

    auto new_pos_x = transform.raw_position;
    if (tar.x > transform.raw_position.x) new_pos_x.x += speed;
    if (tar.x < transform.raw_position.x) new_pos_x.x -= speed;

    auto new_pos_z = transform.raw_position;
    if (tar.y > transform.raw_position.z) new_pos_z.z += speed;
    if (tar.y < transform.raw_position.z) new_pos_z.z -= speed;

    // TODO do we need to unr the whole person_update...() function with
    // collision?

    transform.position.x = new_pos_x.x;
    transform.position.z = new_pos_z.z;
}

inline void render_job_visual(std::shared_ptr<Entity> entity, float) {
    if (!entity->has<CanPerformJob>()) return;
    CanPerformJob& cpf = entity->get<CanPerformJob>();
    if (!cpf.has_job()) return;

    // TODO this doesnt work yet because job->path is not serialized
    const float box_size = TILESIZE / 10.f;
    if (cpf.job() && !cpf.job()->path.empty()) {
        for (auto location : cpf.job()->path) {
            DrawCube(vec::to3(location), box_size, box_size, box_size, BLUE);
        }
    }
}

}  // namespace job_system

}  // namespace system_manager
