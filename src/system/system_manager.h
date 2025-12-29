
#pragma once

#include <functional>
#include <vector>

#include "../ah.h"
#include "../engine/keymap.h"
#include "../engine/singleton.h"
#include "../entity.h"

using afterhours::Entities;

namespace system_manager {

void move_player_SERVER_ONLY(Entity& entity, game::State location);
void fix_container_item_type(Entity& entity);

}  // namespace system_manager

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

    void for_each_old(const std::function<void(Entity&)>& cb);
    void for_each_old(const std::function<void(Entity&)>& cb) const;

    bool is_bar_open() const;
    bool is_bar_closed() const;

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

    void process_state_change(const Entities& entities, float dt);
    void every_frame_update(const Entities& entity_list, float dt);
};
