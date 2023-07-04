
#pragma once

#include "input_process_manager.h"
#include "job_system.h"
#include "rendering_system.h"

namespace system_manager {

void transform_snapper(std::shared_ptr<Entity> entity, float);

void update_held_furniture_position(std::shared_ptr<Entity> entity, float);

void update_held_item_position(std::shared_ptr<Entity> entity, float);

void reset_highlighted(std::shared_ptr<Entity> entity, float);

void highlight_facing_furniture(std::shared_ptr<Entity> entity, float);
void move_entity_based_on_push_force(std::shared_ptr<Entity> entity, float,
                                     vec3& new_pos_x, vec3& new_pos_z);
void process_conveyer_items(std::shared_ptr<Entity> entity, float dt);

void process_grabber_items(std::shared_ptr<Entity> entity, float);

template<typename I>
void backfill_empty_container(std::shared_ptr<Entity> entity);

void process_is_container_and_should_backfill_item(std::shared_ptr<Entity>,
                                                   float);
void process_is_container_and_should_update_item(std::shared_ptr<Entity>,
                                                 float);

void handle_autodrop_furniture_when_exiting_planning(
    const std::shared_ptr<Entity>& entity);

void delete_held_items_when_leaving_inround(
    const std::shared_ptr<Entity>& entity);

void refetch_dynamic_model_names(const std::shared_ptr<Entity>& entity, float);

void process_trigger_area(const std::shared_ptr<Entity>& entity, float dt);

// TODO maybe we could pull out all the singleton boiz into their own update
// loop / entity thing or mayeb all the server only ones?
void process_spawner(const std::shared_ptr<Entity>& entity, float dt);

}  // namespace system_manager

SINGLETON_FWD(SystemManager)
struct SystemManager {
    SINGLETON(SystemManager)

    Entities oldAll;

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

    void update(const Entities& entities, float dt) {
        // TODO add num entities to debug overlay
        // log_info("num entities {}", entities.size());
        // TODO do we run game updates during paused?

        if (GameState::get().is(game::State::InRound)) {
            in_round_update(entities, dt);
        } else {
            planning_update(entities, dt);
        }

        always_update(entities, dt);
        process_state_change(entities, dt);
    }

    void update_all_entities(const Entities& players, float dt) {
        // TODO speed?
        Entities all;
        Entities ents = EntityHelper::get_entities();

        all.reserve(players.size() + ents.size());

        all.insert(all.end(), players.begin(), players.end());
        all.insert(all.end(), ents.begin(), ents.end());

        oldAll = all;

        update(all, dt);
    }

    void process_inputs(const Entities& entities, const UserInputs& ui) {
        for (auto& entity : entities) {
            if (entity->is_missing<RespondsToUserInput>()) continue;
            for (auto inputs : ui) {
                int i = 0;
                while (i < InputName::Last) {
                    log_info("processing input {}", i);
                    auto input_name = magic_enum::enum_value<InputName>(i);
                    bool was_pressed = inputs.test(i);
                    if (was_pressed) {
                        system_manager::input_process_manager::process_input(
                            entity, input);
                    }

                    i++;
                }
            }
        }

        void render_entities(const Entities& entities, float dt) const {
            const auto debug_mode_on =
                GLOBALS.get_or_default<bool>("debug_ui_enabled", false);
            if (debug_mode_on) {
                render_debug(entities, dt);
                render_normal(entities, dt);
            } else {
                render_normal(entities, dt);
            }
        }

        void render_items(Items items, float) const {
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
        void process_state_change(
            const std::vector<std::shared_ptr<Entity>>& entities, float dt) {
            if (state_transitioned_round_to_planning) {
                state_transitioned_round_to_planning = false;
                for (auto& entity : entities) {
                    system_manager::delete_held_items_when_leaving_inround(
                        entity);
                }
            }

            if (state_transitioned_planning_to_round) {
                state_transitioned_planning_to_round = false;
                for (auto& entity : entities) {
                    system_manager::
                        handle_autodrop_furniture_when_exiting_planning(entity);
                }
            }

            // All transitions
            for (auto& entity : entities) {
                system_manager::refetch_dynamic_model_names(entity, dt);
            }
        }

        void always_update(
            const std::vector<std::shared_ptr<Entity>>& entity_list, float dt) {
            for (auto& entity : entity_list) {
                system_manager::reset_highlighted(entity, dt);
                // TODO should be just planning + lobby?
                // maybe a second one for highlighting items?
                system_manager::highlight_facing_furniture(entity, dt);
                system_manager::transform_snapper(entity, dt);
                system_manager::input_process_manager::collect_user_input(
                    entity, dt);
                system_manager::update_held_item_position(entity, dt);

                system_manager::process_trigger_area(entity, dt);

                // TODO this is in the render manager but its not really a
                // render thing but at the same time it kinda is idk This could
                // run only in lobby if we wanted to distinguish
                system_manager::render_manager::
                    update_character_model_from_index(entity, dt);
            }
        }

        void in_round_update(
            const std::vector<std::shared_ptr<Entity>>& entity_list, float dt) {
            for (auto& entity : entity_list) {
                system_manager::job_system::in_round_update(entity, dt);
                system_manager::process_grabber_items(entity, dt);
                system_manager::process_conveyer_items(entity, dt);
                system_manager::process_is_container_and_should_backfill_item(
                    entity, dt);
                system_manager::process_is_container_and_should_update_item(
                    entity, dt);
                system_manager::process_spawner(entity, dt);
            }
        }

        void planning_update(
            const std::vector<std::shared_ptr<Entity>>& entity_list, float dt) {
            for (auto& entity : entity_list) {
                system_manager::update_held_furniture_position(entity, dt);
            }
        }

        void render_normal(
            const std::vector<std::shared_ptr<Entity>>& entity_list, float dt)
            const {
            for (auto& entity : entity_list) {
                system_manager::render_manager::render_normal(entity, dt);
                system_manager::render_manager::render_floating_name(entity,
                                                                     dt);
                system_manager::render_manager::render_progress_bar(entity, dt);
            }
        }

        void render_debug(
            const std::vector<std::shared_ptr<Entity>>& entity_list, float dt)
            const {
            for (auto& entity : entity_list) {
                system_manager::render_manager::render_debug(entity, dt);
            }
        }
    };
