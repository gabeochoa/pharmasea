
#include "job_system.h"

#include "../components/can_be_ghost_player.h"
#include "../components/can_highlight_others.h"
#include "../components/can_hold_furniture.h"
#include "../components/can_order_drink.h"
#include "../components/can_pathfind.h"
#include "../components/can_perform_job.h"
#include "../components/custom_item_position.h"
#include "../components/has_base_speed.h"
#include "../components/has_timer.h"
#include "../components/transform.h"
#include "../entity.h"
#include "../entity_helper.h"
#include "../vendor_include.h"
#include "logging_system.h"

namespace system_manager {

namespace job_system {

void render_job_visual(const Entity& entity, float) {
    if (entity.is_missing<CanPathfind>()) return;
    const float box_size = TILESIZE / 10.f;
    entity.get<CanPathfind>().for_each_path_location([box_size](vec2 location) {
        DrawCube(vec::to3(location), box_size, box_size, box_size, BLUE);
    });
}

void ensure_has_job(Entity& entity, CanPerformJob& cpj, float) {
    if (cpj.has_job()) return;

    // TODO handle employee ai
    // TODO handle being angry or something

    // IF the store is closed then leave
    Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);

    const Transform& transform = entity.get<Transform>();
    const auto pos = transform.as2();

    // TODO if the store is closed skip the rest of orders?
    if (sophie.get<HasTimer>().store_is_closed()) {
        // Since there are non customer ais (roomba)
        // we need to only send them home if they are a customer
        // this is a reasonable way for now
        if (entity.has<CanOrderDrink>()) {
            cpj.update(Job::create_job_of_type(
                pos, vec2{GATHER_SPOT, GATHER_SPOT}, JobType::Leaving));
            return;
        }
    }

    auto& personal_queue = cpj.job_queue();
    if (personal_queue.empty()) {
        // No job and nothing in the queue? grab the next default one then
        cpj.update(Job::create_job_of_type(pos, pos, cpj.get_next_job_type()));
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

Job::State run_state_heading_to(const std::shared_ptr<Job>& job,
                                Job::State begin, Entity& entity, float dt) {
    Job::State complete = begin == Job::State::HeadingToStart
                              ? Job::State::WorkingAtStart
                              : Job::State::WorkingAtEnd;
    vec2 goal = begin == Job::State::HeadingToStart ? job->start : job->end;

    return entity.get<CanPathfind>().travel_toward(
               goal, get_speed_for_entity(entity) * dt)
               ? complete
               : begin;
}

void run_job_tick(Entity& entity, CanPerformJob& cpj, float dt) {
    std::shared_ptr<Job> current_job = cpj.get_current_job();
    current_job->before_each_job_tick(entity, dt);

    Job::State state = current_job->state;
    Job::State new_state = state;
    switch (state) {
        case Job::State::Initialize:
            new_state = current_job->run_state_initialize(entity, dt);
            break;
        case Job::State::HeadingToStart:
        case Job::State::HeadingToEnd: {
            new_state = run_state_heading_to(current_job, state, entity, dt);
        } break;
        case Job::State::WorkingAtStart:
            new_state = current_job->run_state_working_at_start(entity, dt);
            break;
        case Job::State::WorkingAtEnd:
            new_state = current_job->run_state_working_at_end(entity, dt);
            break;
        case Job::State::Completed:
            break;
    }
    current_job->state = new_state;
}

void in_round_update(Entity& entity, float dt) {
    if (entity.is_missing<CanPerformJob>()) return;

    CanPerformJob& cpj = entity.get<CanPerformJob>();
    cpj.cleanup_if_completed();

    ensure_has_job(entity, cpj, dt);
    run_job_tick(entity, cpj, dt);
}

}  // namespace job_system

}  // namespace system_manager
