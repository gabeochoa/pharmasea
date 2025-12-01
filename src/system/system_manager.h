
#pragma once

#include "../ah.h"
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
        SoundLibrary::get().play(strings::sounds::to_name(strings::sounds::SoundId::ROBLOX));
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

    // afterhours SystemManager for entity systems
    // Note: mutable because render() is non-const but render_entities() is
    // const
    mutable afterhours::SystemManager systems;

    SystemManager() {
        // Register state manager
        GameState::get().register_on_change(
            std::bind(&SystemManager::on_game_state_change, this,
                      std::placeholders::_1, std::placeholders::_2));
        // Register afterhours systems
        register_afterhours_systems();
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
    void register_afterhours_systems();
    void register_sixtyfps_systems();
    void register_gamelike_systems();
    void register_modeltest_systems();
    void register_inround_systems();
    void register_planning_systems();
    void register_render_systems();
    void register_day_night_transition_systems();

    // TODO this probably shouldnt be const but it can be since it holds
    // shared_ptrs
    void for_each(const Entities& entities, float dt,
                  const std::function<void(Entity&, float)>& cb) {
        for (const auto& entity : entities) {
            if (!entity) continue;
            // Check if entity is marked for cleanup before processing
            if (entity->cleanup) continue;
            // Additional safety: check if entity ID is valid (should always be
            // >= 0)
            if (entity->id < 0) continue;
            try {
                cb(*entity, dt);
            } catch (...) {
                // Skip entities that cause exceptions (likely
                // invalid/destroyed) Log for debugging but don't crash
                log_warn("Exception processing entity {} in for_each",
                         entity->id);
            }
        }
    }

    void for_each(const Entities& entities, float dt,
                  const std::function<void(const Entity&, float)>& cb) const {
        for (const auto& entity : entities) {
            if (!entity) continue;
            // Check if entity is marked for cleanup before processing
            if (entity->cleanup) continue;
            // Additional safety: check if entity ID is valid (should always be
            // >= 0)
            if (entity->id < 0) continue;
            try {
                cb(*entity, dt);
            } catch (...) {
                // Skip entities that cause exceptions (likely
                // invalid/destroyed) Log for debugging but don't crash
                log_warn("Exception processing entity {} in for_each (const)",
                         entity->id);
            }
        }
    }

    void process_state_change(const Entities& entities, float dt);
    void every_frame_update(const Entities& entity_list, float dt);

    // DEPRECATED: These functions are replaced by afterhours systems
    // - sixty_fps_update -> SixtyFpsUpdateSystem
    // - game_like_update -> GameLikeUpdateSystem
    // - model_test_update -> ModelTestUpdateSystem
    // - in_round_update -> InRoundUpdateSystem
    // - planning_update -> PlanningUpdateSystem
    // These are kept for reference but should not be called
    [[deprecated("Use afterhours systems instead")]] void sixty_fps_update(
        const Entities& entity_list, float dt);
    [[deprecated("Use afterhours systems instead")]] void game_like_update(
        const Entities& entity_list, float dt);
    [[deprecated("Use afterhours systems instead")]] void in_round_update(
        const Entities& entity_list, float dt);
    [[deprecated("Use afterhours systems instead")]] void model_test_update(
        const Entities& entity_list, float dt);
    [[deprecated("Use afterhours systems instead")]] void planning_update(
        const Entities& entity_list, float dt);
    void progression_update(const Entities& entity_list, float dt);
    void store_update(const Entities&, float);
};
