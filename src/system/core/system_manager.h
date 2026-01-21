
#pragma once

#include <functional>
#include <vector>

#include "../../ah.h"
#include "../../engine/keymap.h"
#include "../../engine/singleton.h"
#include "../../entity.h"

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
    mutable afterhours::SystemManager input_systems;

    SystemManager() {
        // Register afterhours systems
        register_afterhours_systems();
        register_input_systems();
    }

    void render_entities(const Entities& entities, float dt) const;
    void render_ui(const Entities& entities, float dt) const;

    void update_local_players(const Entities& players, float dt);
    void update_remote_players(const Entities& players, float dt);
    void update_all_entities(const Entities& players, float dt);

    void process_inputs(const Entities& entities, const UserInputs& inputs);

    bool is_bar_open() const;
    bool is_bar_closed() const;

   private:
    void register_afterhours_systems();
    void register_sixtyfps_systems();
    void register_gamelike_systems();
    void register_modeltest_systems();
    void register_inround_systems();
    void register_planning_systems();
    void register_render_systems();
    void register_day_night_transition_systems();
    void register_input_systems();

    void every_frame_update(const Entities& entity_list, float dt);
};
