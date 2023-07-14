
#pragma once

#include "input_process_manager.h"
#include "job_system.h"
#include "rendering_system.h"

namespace system_manager {

void transform_snapper(Entity& entity, float);

void update_held_furniture_position(Entity& entity, float);

void update_held_item_position(Entity& entity, float);

void reset_highlighted(Entity& entity, float);

void highlight_facing_furniture(Entity& entity, float);
void move_entity_based_on_push_force(Entity& entity, float, vec3& new_pos_x,
                                     vec3& new_pos_z);
void process_conveyer_items(Entity& entity, float dt);

void process_grabber_items(Entity& entity, float);

template<typename I>
void backfill_empty_container(Entity& entity);

void process_is_container_and_should_backfill_item(Entity&, float);
void process_is_container_and_should_update_item(Entity&, float);

void handle_autodrop_furniture_when_exiting_planning(Entity& entity);

void delete_held_items_when_leaving_inround(Entity& entity);

void refetch_dynamic_model_names(Entity& entity, float);

void process_trigger_area(Entity& entity, float dt);

// TODO maybe we could pull out all the singleton boiz into their own update
// loop / entity thing or mayeb all the server only ones?
void process_spawner(Entity& entity, float dt);

void run_timer(Entity& entity, float dt);
void sophie(Entity& entity, float dt);

}  // namespace system_manager

SINGLETON_FWD(SystemManager)
struct SystemManager {
    SINGLETON(SystemManager)

    Entities sm_players;

    SystemManager() {
        // Register state manager
        GameState::get().register_on_change(
            std::bind(&SystemManager::on_game_state_change, this,
                      std::placeholders::_1, std::placeholders::_2));
    }

    bool state_transitioned_round_to_planning = false;
    bool state_transitioned_planning_to_round = false;

    void on_game_state_change(game::State new_state, game::State old_state) {
        // log_warn("system manager on gamestate change from {} to {}",
        // old_state, new_state);

        if (old_state == game::State::InRound &&
            new_state == game::State::Planning) {
            state_transitioned_round_to_planning = true;
        }

        if (old_state == game::State::Planning &&
            new_state == game::State::InRound) {
            state_transitioned_planning_to_round = true;
        }
    }

    void update(Entities& entities, float dt) {
        // TODO add num entities to debug overlay
        // log_info("num entities {}", entities.size());
        // TODO do we run game updates during paused?

        if (GameState::s_in_round()) {
            in_round_update(entities, dt);
        } else {
            planning_update(entities, dt);
        }

        always_update(entities, dt);
        process_state_change(entities, dt);
    }

    void update_all_entities(Entities& players, float dt) {
        // TODO speed?

        update(players, dt);

        sm_players.clear();
        sm_players.reserve(players.size());
        sm_players.insert(sm_players.end(),
                          std::make_move_iterator(players.begin()),
                          std::make_move_iterator(players.end()));

        update(EntityHelper::get_entities(), dt);

        players.clear();
        players.reserve(sm_players.size());
        players.insert(players.end(),
                       std::make_move_iterator(sm_players.begin()),
                       std::make_move_iterator(sm_players.end()));
    }

    void process_inputs_for_entity(Entity& entity, const UserInputs& inputs) {
        if (entity.is_missing<RespondsToUserInput>()) return;
        for (auto input : inputs) {
            system_manager::input_process_manager::process_input(entity, input);
        }
    }

    void process_inputs(Entities& entities, const UserInputs& inputs) {
        for (auto& entity : entities) {
            process_inputs_for_entity(entity, inputs);
        }
    }

    void render_entities(const Entities& entities, float dt) const {
        const auto debug_mode_on =
            GLOBALS.get_or_default<bool>("debug_ui_enabled", false);
        if (debug_mode_on) {
            render_debug(entities, dt);
        }
        render_normal(entities, dt);
    }

    // TODO const
    void render_ui(Entities& entities, float dt) {
        const auto debug_mode_on =
            GLOBALS.get_or_default<bool>("debug_ui_enabled", false);
        if (debug_mode_on) {
            system_manager::ui::render_debug(entities, dt);
        }
        system_manager::ui::render_normal(entities, dt);
    }

    // TODO const
    void render_items(Items items, float) {
        for (auto i : items) {
            if (!i) {
                log_warn("we have invalid items");
                continue;
            }
            if (i->cleanup) continue;
            i->render();
        }
    }

   private:
    void process_state_change(Entities& entities, float dt) {
        if (state_transitioned_round_to_planning) {
            state_transitioned_round_to_planning = false;
            for (auto& entity : entities) {
                system_manager::delete_held_items_when_leaving_inround(entity);
            }
        }

        if (state_transitioned_planning_to_round) {
            state_transitioned_planning_to_round = false;
            for (auto& entity : entities) {
                system_manager::handle_autodrop_furniture_when_exiting_planning(
                    entity);
            }
        }

        // All transitions
        for (auto& entity : entities) {
            system_manager::refetch_dynamic_model_names(entity, dt);
        }
    }

    void always_update(Entities& entity_list, float dt) {
        for (auto& entity : entity_list) {
            system_manager::reset_highlighted(entity, dt);
            // TODO should be just planning + lobby?
            // maybe a second one for highlighting items?
            system_manager::highlight_facing_furniture(entity, dt);
            system_manager::transform_snapper(entity, dt);
            system_manager::input_process_manager::collect_user_input(entity,
                                                                      dt);
            system_manager::update_held_item_position(entity, dt);

            system_manager::process_trigger_area(entity, dt);

            // TODO this is in the render manager but its not really a
            // render thing but at the same time it kinda is idk This could
            // run only in lobby if we wanted to distinguish
            system_manager::render_manager::update_character_model_from_index(
                entity, dt);

            system_manager::run_timer(entity, dt);

            // TODO these eventually should move into their own functions but
            // for now >:)
            // TODO switch to using match_name()
            if (entity.get<DebugName>().name() == strings::entity::SOPHIE)
                system_manager::sophie(entity, dt);
        }
    }

    void in_round_update(Entities& entity_list, float dt) {
        for (auto& entity : entity_list) {
            system_manager::job_system::in_round_update(entity, dt);
            system_manager::process_grabber_items(entity, dt);
            system_manager::process_conveyer_items(entity, dt);
            // TODO you should be able to put items back in the container
            system_manager::process_is_container_and_should_backfill_item(
                entity, dt);
            system_manager::process_is_container_and_should_update_item(entity,
                                                                        dt);
            system_manager::process_spawner(entity, dt);
        }
    }

    void planning_update(Entities& entity_list, float dt) {
        for (auto& entity : entity_list) {
            system_manager::update_held_furniture_position(entity, dt);
        }
    }

    void render_normal(const Entities& entity_list, float dt) const {
        for (auto& entity : entity_list) {
            // TODO extract render normal into system facign functions
            system_manager::render_manager::render_normal(entity, dt);
            system_manager::render_manager::render_floating_name(entity, dt);
            system_manager::render_manager::render_progress_bar(entity, dt);
        }
    }

    void render_debug(const Entities& entity_list, float dt) const {
        for (auto& entity : entity_list) {
            system_manager::render_manager::render_debug(entity, dt);
        }
    }
};
