

#pragma once

#include "../components/can_perform_job.h"
#include "../entity.h"

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

void render_job_visual(const Entity& entity, float);
void ensure_has_job(Entity& entity, CanPerformJob& cpj, float);
void in_round_update(Entity& entity, float dt);

}  // namespace job_system

}  // namespace system_manager
