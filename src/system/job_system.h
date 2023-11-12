

#pragma once

#include "../components/can_be_ghost_player.h"
#include "../components/can_highlight_others.h"
#include "../components/can_hold_furniture.h"
#include "../components/can_order_drink.h"
#include "../components/can_pathfind.h"
#include "../components/can_perform_job.h"
#include "../components/custom_item_position.h"
#include "../components/has_timer.h"
#include "../components/transform.h"
#include "../entity.h"
#include "../entity_helper.h"
#include "../vendor_include.h"
#include "logging_system.h"

namespace system_manager {

namespace job_system {

/*
 // TODO eventually turn this back on

inline void handle_job_holder_pushed(std::shared_ptr<Entity> entity, float) {
    if (entity->is_missing<CanPerformJob>()) return;
    CanPerformJob& cpf = entity->get<CanPerformJob>();
    if (!cpf.has_job()) return;
    auto job = cpf.job();

    const CanBePushed& cbp = entity->get<CanBePushed>();

    if (cbp.pushed_force().x != 0.0f || cbp.pushed_force().z != 0.0f) {
        job->path.clear();
        job->local = {};
        SoundLibrary::get().play(strings::sounds::ROBLOX);
    }
}

*/

inline void render_job_visual(const Entity& entity, float) {
    if (entity.is_missing<CanPathfind>()) return;
    const float box_size = TILESIZE / 10.f;
    entity.get<CanPathfind>().for_each_path_location([box_size](vec2 location) {
        DrawCube(vec::to3(location), box_size, box_size, box_size, BLUE);
    });
}

inline void ensure_has_job(Entity& entity, float) {
    if (entity.is_missing<CanPerformJob>()) return;
    CanPerformJob& cpj = entity.get<CanPerformJob>();
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

inline void run_job_tick(Entity& entity, float dt) {
    if (entity.is_missing<CanPerformJob>()) return;
    entity.get<CanPerformJob>().run_tick(entity, dt);
}

inline void cleanup_completed_job(Entity& entity, float) {
    // TODO probably can just live in 'in_round_update'?
    if (entity.is_missing<CanPerformJob>()) return;
    entity.get<CanPerformJob>().cleanup_if_completed();
}

inline void in_round_update(Entity& entity, float dt) {
    cleanup_completed_job(entity, dt);
    ensure_has_job(entity, dt);
    run_job_tick(entity, dt);
}

}  // namespace job_system

}  // namespace system_manager
