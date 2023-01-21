
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
#include "../furniture/register.h"

namespace system_manager {

inline void transform_snapper(std::shared_ptr<Entity> entity, float) {
    if (!entity->has<Transform>()) return;

    Transform& transform = entity->get<Transform>();
    if (entity->is_snappable()) {
        transform.position = transform.snap_position();
    } else {
        transform.position = transform.raw_position;
    }
}

inline void update_held_furniture_position(std::shared_ptr<Entity> entity,
                                           float) {
    if (!entity->has<CanHoldFurniture>()) return;
    CanHoldFurniture& can_hold_furniture = entity->get<CanHoldFurniture>();

    // TODO explicity commenting this out so that we get an error
    // if (!entity->has<Transform>()) return;
    Transform& transform = entity->get<Transform>();

    // TODO if cannot be placed in this spot make it obvious to the user

    if (can_hold_furniture.empty()) return;

    auto new_pos = transform.position;
    if (transform.face_direction & Transform::FrontFaceDirection::FORWARD) {
        new_pos.z += TILESIZE;
    }
    if (transform.face_direction & Transform::FrontFaceDirection::RIGHT) {
        new_pos.x += TILESIZE;
    }
    if (transform.face_direction & Transform::FrontFaceDirection::BACK) {
        new_pos.z -= TILESIZE;
    }
    if (transform.face_direction & Transform::FrontFaceDirection::LEFT) {
        new_pos.x -= TILESIZE;
    }

    can_hold_furniture.furniture()->update_position(new_pos);
}

inline void update_held_item_position(std::shared_ptr<Entity> entity, float) {
    if (!entity->has<CanHoldItem>()) return;
    CanHoldItem& can_hold_item = entity->get<CanHoldItem>();
    if (can_hold_item.empty()) return;

    if (!entity->has<Transform>()) return;
    Transform& transform = entity->get<Transform>();

    if (entity->has<CustomHeldItemPosition>()) {
        CustomHeldItemPosition& custom_item_position =
            entity->get<CustomHeldItemPosition>();

        if (custom_item_position.mutator) {
            can_hold_item.item()->update_position(
                custom_item_position.mutator(transform));
        } else {
            log_warn(
                "Entity has custom held item position but didnt initalize the "
                "component");
        }
        return;
    }

    // Default

    auto new_pos = transform.position;
    if (transform.face_direction & Transform::FrontFaceDirection::FORWARD) {
        new_pos.z += TILESIZE;
    }
    if (transform.face_direction & Transform::FrontFaceDirection::RIGHT) {
        new_pos.x += TILESIZE;
    }
    if (transform.face_direction & Transform::FrontFaceDirection::BACK) {
        new_pos.z -= TILESIZE;
    }
    if (transform.face_direction & Transform::FrontFaceDirection::LEFT) {
        new_pos.x -= TILESIZE;
    }
    can_hold_item.item()->update_position(new_pos);
}

inline void update_job_information(std::shared_ptr<Entity> entity, float dt) {
    if (!entity->has<CanPerformJob>()) return;

    CanPerformJob& cpj = entity->get<CanPerformJob>();
    std::shared_ptr<Job> job = cpj.job();

    // User has no active job
    if (!job) return;

    const auto travel_on_path = [entity](vec2 me) {
        auto job = entity->get<CanPerformJob>().job();
        // did not arrive
        if (job->path.empty()) return;
        // Grab next local point to go to
        if (!job->local.has_value()) {
            job->local = job->path.front();
            job->path.pop_front();
        }
        // Go to local point
        if (job->local.value() == me) {
            job->local.reset();
            // announce(fmt::format("reached local point {} : {} ", me,
            // job->path.size()));
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
        if (job->path.empty()) {
            job->path = astar::find_path(
                me, goal,
                std::bind(EntityHelper::isWalkable, std::placeholders::_1));
            // announce(fmt::format("generated path from {} to {} with {}
            // steps", me, goal, job->path.size()));
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
                if (job->path.size() == 0) {
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
        auto job = entity->get<CanPerformJob>().job();
        auto personal_queue = entity->get<CanPerformJob>().job_queue();

        auto wait_and_return = [&]() {
            // Add the current job to the queue,
            // and then add the waiting job
            personal_queue.push(job);
            job.reset(new Job({
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

inline void render_simple_highlighted(std::shared_ptr<Entity> entity, float) {
    if (!entity->has<Transform>()) return;
    Transform& transform = entity->get<Transform>();
    if (!entity->has<SimpleColoredBoxRenderer>()) return;
    SimpleColoredBoxRenderer& renderer =
        entity->get<SimpleColoredBoxRenderer>();

    Color f = ui::color::getHighlighted(renderer.face_color);
    Color b = ui::color::getHighlighted(renderer.base_color);
    // TODO replace size with Bounds component when it exists
    DrawCubeCustom(transform.raw_position, transform.size.x, transform.size.y,
                   transform.size.z,
                   transform.FrontFaceDirectionMap.at(transform.face_direction),
                   f, b);
}

inline void render_simple_normal(std::shared_ptr<Entity> entity, float) {
    if (!entity->has<Transform>()) return;
    Transform& transform = entity->get<Transform>();
    if (!entity->has<SimpleColoredBoxRenderer>()) return;
    SimpleColoredBoxRenderer& renderer =
        entity->get<SimpleColoredBoxRenderer>();
    DrawCubeCustom(transform.raw_position, transform.size.x, transform.size.y,
                   transform.size.z,
                   transform.FrontFaceDirectionMap.at(transform.face_direction),
                   renderer.face_color, renderer.base_color);
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

inline void render_debug(std::shared_ptr<Entity> entity, float dt) {
    // Ghost player only render during debug mode
    if (entity->has<CanBeGhostPlayer>()) {
        if (entity->get<CanBeGhostPlayer>().is_not_ghost()) {
        } else {
            render_simple_normal(entity, dt);
        }
        return;
    }

    if (entity->has<CanBeHighlighted>() &&
        entity->get<CanBeHighlighted>().is_highlighted) {
        render_simple_highlighted(entity, dt);
        return;
    }

    render_job_visual(entity, dt);
}

inline bool render_model_highlighted(std::shared_ptr<Entity> entity, float) {
    if (!entity->has<ModelRenderer>()) return false;
    if (!entity->has<CanBeHighlighted>()) return false;

    ModelRenderer& renderer = entity->get<ModelRenderer>();
    if (!renderer.has_model()) return false;

    if (!entity->has<Transform>()) return false;
    Transform& transform = entity->get<Transform>();

    ModelInfo model_info = renderer.model_info().value();
    Color base = ui::color::getHighlighted(WHITE /*this->base_color*/);

    float rotation_angle =
        // TODO make this api better
        180.f + static_cast<int>(transform.FrontFaceDirectionMap.at(
                    transform.face_direction));

    DrawModelEx(renderer.model(),
                {
                    transform.position.x + model_info.position_offset.x,
                    transform.position.y + model_info.position_offset.y,
                    transform.position.z + model_info.position_offset.z,
                },
                vec3{0.f, 1.f, 0.f}, rotation_angle,
                transform.size * model_info.size_scale, base);

    return true;
}

inline bool render_model_normal(std::shared_ptr<Entity> entity, float) {
    if (!entity->has<ModelRenderer>()) return false;

    ModelRenderer& renderer = entity->get<ModelRenderer>();
    if (!renderer.has_model()) return false;

    if (!entity->has<Transform>()) return false;
    Transform& transform = entity->get<Transform>();

    ModelInfo model_info = renderer.model_info().value();

    float rotation_angle =
        // TODO make this api better
        180.f + static_cast<int>(transform.FrontFaceDirectionMap.at(
                    transform.face_direction));

    raylib::DrawModelEx(
        renderer.model(),
        {
            transform.position.x + model_info.position_offset.x,
            transform.position.y + model_info.position_offset.y,
            transform.position.z + model_info.position_offset.z,
        },
        vec3{0, 1, 0}, model_info.rotation_angle + rotation_angle,
        transform.size * model_info.size_scale, WHITE /*this->base_color*/);

    return true;
}

inline void render_normal(std::shared_ptr<Entity> entity, float dt) {
    // Ghost player cant render during normal mode
    if (entity->has<CanBeGhostPlayer>() &&
        entity->get<CanBeGhostPlayer>().is_ghost()) {
        return;
    }

    if (entity->has<CanBeHighlighted>() &&
        entity->get<CanBeHighlighted>().is_highlighted) {
        bool used = render_model_highlighted(entity, dt);
        if (!used) {
            render_simple_highlighted(entity, dt);
        }
        return;
    }

    bool used = render_model_normal(entity, dt);
    if (!used) {
        render_simple_normal(entity, dt);
    }
}

inline void reset_highlighted(std::shared_ptr<Entity> entity, float) {
    if (!entity->has<CanBeHighlighted>()) return;
    CanBeHighlighted& cbh = entity->get<CanBeHighlighted>();
    cbh.is_highlighted = false;
}

inline void highlight_facing_furniture(std::shared_ptr<Entity> entity, float) {
    if (!entity->has<CanHighlightOthers>()) return;
    CanHighlightOthers& cho = entity->get<CanHighlightOthers>();

    // TODO explicity commenting this out so that we get an error
    // if (!entity->has<Transform>()) return;
    Transform& transform = entity->get<Transform>();

    // TODO this is impossible to read, what can we do to fix this while
    // keeping it configurable
    auto match = EntityHelper::getMatchingEntityInFront<Furniture>(
        // TODO add a player reach component
        transform.as2(), cho.reach(), transform.face_direction,
        [](std::shared_ptr<Furniture>) { return true; });
    if (!match) return;
    if (!match->has<CanBeHighlighted>()) return;
    match->get<CanBeHighlighted>().is_highlighted = true;
}

}  // namespace system_manager

struct SystemManager {
    void update(float dt) {
        // TODO eventually this shouldnt exist
        for (auto e : EntityHelper::get_entities()) {
            if (e) e->update(dt);
        }

        always_update(dt);

        // TODO do we run game updates during paused?
        // TODO rename game/nongame to in_round inplanning
        if (GameState::get().is(game::State::InRound)) {
            in_round_update(dt);
        } else {
            planning_update(dt);
        }
    }

    void render(float dt) const {
        const auto debug_mode_on =
            GLOBALS.get_or_default<bool>("debug_ui_enabled", false);
        if (debug_mode_on) {
            render_debug(dt);
        } else {
            render_normal(dt);
        }
    }

   private:
    void always_update(float dt) {
        EntityHelper::forEachEntity([dt](std::shared_ptr<Entity> entity) {
            system_manager::reset_highlighted(entity, dt);
            system_manager::transform_snapper(entity, dt);
            system_manager::update_held_item_position(entity, dt);
            return EntityHelper::ForEachFlow::None;
        });
    }

    void in_round_update(float dt) {
        EntityHelper::forEachEntity([dt](std::shared_ptr<Entity> entity) {
            system_manager::update_job_information(entity, dt);
            return EntityHelper::ForEachFlow::None;
        });
    }

    void planning_update(float dt) {
        EntityHelper::forEachEntity([dt](std::shared_ptr<Entity> entity) {
            system_manager::highlight_facing_furniture(entity, dt);
            system_manager::update_held_furniture_position(entity, dt);
            return EntityHelper::ForEachFlow::None;
        });
    }

    void render_normal(float dt) const {
        EntityHelper::forEachEntity([dt](std::shared_ptr<Entity> entity) {
            system_manager::render_normal(entity, dt);
            return EntityHelper::ForEachFlow::None;
        });
    }

    void render_debug(float dt) const {
        EntityHelper::forEachEntity([dt](std::shared_ptr<Entity> entity) {
            system_manager::render_debug(entity, dt);
            return EntityHelper::ForEachFlow::None;
        });
    }
};
