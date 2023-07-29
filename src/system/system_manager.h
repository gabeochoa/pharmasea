
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
void process_grabber_filter(std::shared_ptr<Entity> entity, float);

template<typename I>
void backfill_empty_container(std::shared_ptr<Entity> entity);

void process_is_container_and_should_backfill_item(std::shared_ptr<Entity>,
                                                   float);
void process_is_container_and_should_update_item(std::shared_ptr<Entity>,
                                                 float);
void process_is_indexed_container_holding_incorrect_item(
    std::shared_ptr<Entity>, float);

void handle_autodrop_furniture_when_exiting_planning(
    const std::shared_ptr<Entity> entity);

void delete_held_items_when_leaving_inround(
    const std::shared_ptr<Entity> entity);

void delete_customers_when_leaving_inround(
    const std::shared_ptr<Entity> entity);

void reset_customer_orders_when_leaving_inround(
    const std::shared_ptr<Entity> entity);

void reset_max_gen_when_after_deletion(const std::shared_ptr<Entity> entity);

void refetch_dynamic_model_names(const std::shared_ptr<Entity> entity, float);

void process_squirter(const std::shared_ptr<Entity> entity, float dt);
void process_trash(const std::shared_ptr<Entity> entity, float dt);
void process_pnumatic_pipe_pairing(const std::shared_ptr<Entity> entity,
                                   float dt);
void process_pnumatic_pipe_movement(const std::shared_ptr<Entity> entity,
                                    float dt);
void process_trigger_area(const std::shared_ptr<Entity> entity, float dt);

// TODO maybe we could pull out all the singleton boiz into their own update
// loop / entity thing or mayeb all the server only ones?
void process_spawner(const std::shared_ptr<Entity> entity, float dt);

void reset_empty_work_furniture(const std::shared_ptr<Entity> entity, float dt);

void run_timer(const std::shared_ptr<Entity> entity, float dt);
void sophie(const std::shared_ptr<Entity> entity, float dt);
void increment_day_count(const std::shared_ptr<Entity> entity, float dt);
void process_has_rope(const std::shared_ptr<Entity> entity, float dt);
}  // namespace system_manager

SINGLETON_FWD(SystemManager)
struct SystemManager {
    SINGLETON(SystemManager)

    // TODO add type for EntityID
    int firstPlayerID = -1;
    Entities oldAll;

    SystemManager() {
        // Register state manager
        GameState::get().register_on_change(
            std::bind(&SystemManager::on_game_state_change, this,
                      std::placeholders::_1, std::placeholders::_2));
    }

    bool state_transitioned_round_to_planning = false;
    bool state_transitioned_planning_to_round = false;

    void on_game_state_change(game::State new_state, game::State old_state);

    void update(const Entities& entities, float dt);

    void update_all_entities(const Entities& players, float dt);

    // TODO for some reason this cant go into the cpp
    // if it does then the game inifite loops once you join the lobby
    void update_local_players(const Entities& players, float dt) {
        for (auto& entity : players) {
            system_manager::input_process_manager::collect_user_input(entity,
                                                                      dt);
        }
    }

    void render_all_entities(const Entities&, float dt) const;

    void render_all_ui(const Entities&, float dt) const;

    void process_inputs(const Entities& entities, const UserInputs& inputs);

    void render_entities(const Entities& entities, float dt) const;

    void render_ui(const Entities& entities, float dt) const;

   private:
    void process_state_change(
        const std::vector<std::shared_ptr<Entity>>& entities, float dt);

    void always_update(const std::vector<std::shared_ptr<Entity>>& entity_list,
                       float dt);

    void in_round_update(
        const std::vector<std::shared_ptr<Entity>>& entity_list, float dt);

    void planning_update(
        const std::vector<std::shared_ptr<Entity>>& entity_list, float dt);

    void render_normal(const std::vector<std::shared_ptr<Entity>>& entity_list,
                       float dt) const;

    void render_debug(const std::vector<std::shared_ptr<Entity>>& entity_list,
                      float dt) const;
};
