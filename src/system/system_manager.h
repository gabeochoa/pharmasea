
#pragma once

#include "../engine/keymap.h"
#include "../engine/singleton.h"
#include "../entity.h"

using afterhours::Entities;

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
}  // namespace job_system
}  // namespace system_manager

namespace system_manager {

void move_player_SERVER_ONLY(Entity& entity, game::State location);

}

SINGLETON_FWD(SystemManager)
struct SystemManager {
    SINGLETON(SystemManager)

    Entities local_players;
    Entities remote_players;
    Entities oldAll;

    // so that we run the first time always
    float timePassed = 0.016f;

    SystemManager() {
        // Register state manager
        GameState::get().register_on_change(
            std::bind(&SystemManager::on_game_state_change, this,
                      std::placeholders::_1, std::placeholders::_2));
    }

    void render_entities(const Entities& entities, float dt) const;
    void render_ui(const Entities& entities, float dt) const;

    void update_local_players(const Entities& players, float dt);
    void update_remote_players(const Entities& players, float dt);
    void update_all_entities(const Entities& players, float dt);

    void process_inputs(const Entities& entities, const UserInputs& inputs);

    void for_each_old(const std::function<void(Entity&)>& cb) {
        std::ranges::for_each(oldAll, [cb](std::shared_ptr<Entity> entity) {
            if (!entity) return;
            cb(*entity);
        });
    }

    void for_each_old(const std::function<void(Entity&)>& cb) const {
        std::ranges::for_each(oldAll, [cb](std::shared_ptr<Entity> entity) {
            if (!entity) return;
            cb(*entity);
        });
    }

    bool is_daytime() const;
    bool is_nighttime() const;

    bool is_some_player_near(vec2 spot, float distance = 4.f) const;

   private:
    using Transition = std::pair<game::State, game::State>;
    std::vector<Transition> transitions;
    void on_game_state_change(game::State new_state, game::State old_state);

    // TODO this probably shouldnt be const but it can be since it holds
    // shared_ptrs
    void for_each(const Entities& entities, float dt,
                  const std::function<void(Entity&, float)>& cb) {
        std::ranges::for_each(entities,
                              [cb, dt](std::shared_ptr<Entity> entity) {
                                  if (!entity) return;
                                  cb(*entity, dt);
                              });
    }

    void for_each(const Entities& entities, float dt,
                  const std::function<void(const Entity&, float)>& cb) const {
        std::ranges::for_each(std::as_const(entities),
                              [cb, dt](std::shared_ptr<Entity> entity) {
                                  if (!entity) return;
                                  cb(*entity, dt);
                              });
    }

    void process_state_change(const Entities& entities, float dt);
    void sixty_fps_update(const Entities& entity_list, float dt);
    void every_frame_update(const Entities& entity_list, float dt);
    void game_like_update(const Entities& entity_list, float dt);
    void in_round_update(const Entities& entity_list, float dt);
    void model_test_update(const Entities& entity_list, float dt);
    void planning_update(const Entities& entity_list, float dt);
    void progression_update(const Entities& entity_list, float dt);
    void store_update(const Entities&, float);
};
