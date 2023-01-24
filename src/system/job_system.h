

#pragma once

#include "../components/can_be_ghost_player.h"
#include "../components/can_highlight_others.h"
#include "../components/can_hold_furniture.h"
#include "../components/can_perform_job.h"
#include "../components/custom_item_position.h"
#include "../components/transform.h"
#include "../customer.h"
#include "../entity.h"
#include "../entityhelper.h"
#include "../furniture.h"
#include "logging_system.h"

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
            logging_manager::announce(
                entity, fmt::format("reached local point {} : {} ", me,
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
            logging_manager::announce(
                entity,
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
                logging_manager::announce(entity,
                                          "starting a new wandering job");
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
                logging_manager::announce(entity, "starting a new wait job");
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

        const auto get_next_queue_position =
            [](std::shared_ptr<Entity> reg,
               std::shared_ptr<AIPerson> customer) -> vec2 {
            M_ASSERT(customer,
                     "entity passed to register queue should not be null");
            M_ASSERT(reg->has<HasWaitingQueue>(),
                     "Trying to get-next-queue-pos for entity which doesnt "
                     "have a waiting queue ");
            HasWaitingQueue& hasWaitingQueue = reg->get<HasWaitingQueue>();
            hasWaitingQueue.ppl_in_line.push_back(customer);
            // the place the customers stand is 1 tile infront of the register
            auto front =
                reg->tile_infront((hasWaitingQueue.next_line_position + 1) * 2);
            hasWaitingQueue.next_line_position++;
            return front;
        };

        const auto position_in_line =
            [](std::shared_ptr<Entity> reg,
               std::shared_ptr<Entity> customer) -> int {
            M_ASSERT(customer,
                     "entity passed to position-in-line should not be null");
            M_ASSERT(reg->has<HasWaitingQueue>(),
                     "Trying to pos-in-line for entity which doesnt "
                     "have a waiting queue ");
            const auto& ppl_in_line = reg->get<HasWaitingQueue>().ppl_in_line;

            for (int i = 0; i < (int) ppl_in_line.size(); i++) {
                if (customer->id == ppl_in_line[i]->id) return i;
            }
            log_warn("Cannot find customer {}", customer->id);
            return -1;
        };

        const auto leave_line = [position_in_line](
                                    std::shared_ptr<Entity> reg,
                                    std::shared_ptr<Entity> customer) {
            M_ASSERT(customer,
                     "entity passed to leave-line should not be null");
            M_ASSERT(reg->has<HasWaitingQueue>(),
                     "Trying to leave-line for entity which doesnt "
                     "have a waiting queue ");
            auto& ppl_in_line = reg->get<HasWaitingQueue>().ppl_in_line;
            int pos = position_in_line(reg, customer);
            if (pos == -1) return;
            if (pos == 0) {
                ppl_in_line.pop_front();
                return;
            }
            ppl_in_line.erase(ppl_in_line.begin() + pos);
        };

        // TODO what was this used for?
        // const auto is_in_line = [position_in_line](
        // std::shared_ptr<Entity> reg,
        // std::shared_ptr<Entity> customer) -> bool {
        // return position_in_line(reg, customer) != -1;
        // };

        // TODO what was this used for
        // const auto has_space_in_queue =
        // [](std::shared_ptr<Entity> reg) -> bool {
        // M_ASSERT(reg->has<HasWaitingQueue>(),
        // "Trying to has-space-in-queue for entity which doesnt "
        // "have a waiting queue ");
        // const HasWaitingQueue& hasWaitingQueue =
        // reg->get<HasWaitingQueue>();
        // return hasWaitingQueue.next_line_position <
        // hasWaitingQueue.max_queue_size;
        // };

        const auto can_move_up = [](std::shared_ptr<Entity> reg,
                                    std::shared_ptr<Entity> customer) -> bool {
            M_ASSERT(customer,
                     "entity passed to can-move-up should not be null");
            M_ASSERT(reg->has<HasWaitingQueue>(),
                     "Trying to can-move-up for entity which doesnt "
                     "have a waiting queue ");
            const auto& ppl_in_line = reg->get<HasWaitingQueue>().ppl_in_line;
            return customer->id == ppl_in_line.front()->id;
        };

        switch (job->state) {
            case Job::State::Initialize: {
                logging_manager::announce(entity,
                                          "starting a new wait in queue job");

                // Figure out which register to go to...

                // TODO replace with finding the one with the least people
                // in it
                std::shared_ptr<Furniture> closest_target =
                    EntityHelper::getClosestMatchingEntity<Furniture>(
                        entity->get<Transform>().as2(), TILESIZE * 100.f,
                        [](auto&&) { return true; });

                if (!closest_target) {
                    // TODO we need some kinda way to save this job,
                    // and come back to it later
                    // i think just putting a Job* unfinished in Job is
                    // probably enough
                    logging_manager::announce(
                        entity, "Could not find a valid register");
                    job->state = Job::State::Initialize;
                    wait_and_return();
                    return;
                }

                job->data["register"] = closest_target.get();
                job->start = get_next_queue_position(
                    dynamic_pointer_cast<Entity>(closest_target),
                    dynamic_pointer_cast<AIPerson>(entity));
                job->end = closest_target->tile_infront(1);
                job->spot_in_line = position_in_line(closest_target, entity);
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
                auto reg = std::shared_ptr<Entity>(
                    static_cast<Entity*>(job->data["register"]));
                int cur_spot_in_line = position_in_line(reg, entity);

                if (cur_spot_in_line == job->spot_in_line ||
                    !can_move_up(reg, entity)) {
                    // We didnt move so just wait a bit before trying again
                    logging_manager::announce(
                        entity,
                        fmt::format("im just going to wait a bit longer"));

                    // Add the current job to the queue,
                    // and then add the waiting job
                    job->state = Job::State::WorkingAtStart;
                    wait_and_return();
                    return;
                }

                // if our spot did change, then move forward
                logging_manager::announce(
                    entity,
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
                auto reg = std::shared_ptr<Entity>(
                    static_cast<Entity*>(job->data["register"]));

                CanHoldItem& regCHI = reg->get<CanHoldItem>();

                if (regCHI.empty()) {
                    logging_manager::announce(entity, "my rx isnt ready yet");
                    wait_and_return();
                    return;
                }

                auto bag = reg->get<CanHoldItem>().asT<Bag>();
                if (!bag) {
                    logging_manager::announce(entity,
                                              "this isnt my rx (not a bag)");
                    wait_and_return();
                    return;
                }

                if (bag->empty()) {
                    logging_manager::announce(entity, "this bag is empty...");
                    wait_and_return();
                    return;
                }

                // TODO eventually migrate item to ECS
                // auto pill_bottle =
                // bag->get<CanHoldItem>().asT<PillBottle>();
                auto pill_bottle =
                    dynamic_pointer_cast<PillBottle>(bag->held_item);
                if (!pill_bottle) {
                    logging_manager::announce(entity,
                                              "this bag doesnt have my pills");
                    wait_and_return();
                    return;
                }

                CanHoldItem& ourCHI = entity->get<CanHoldItem>();
                ourCHI.update(regCHI.item());
                regCHI.update(nullptr);

                logging_manager::announce(entity, "got it");
                leave_line(reg, entity);
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
