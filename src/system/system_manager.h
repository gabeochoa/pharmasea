
#pragma once

#include "../engine/singleton.h"
#include "../entity.h"

SINGLETON_FWD(SystemManager)
struct SystemManager {
    SINGLETON(SystemManager)

    // TODO fix this if we have more than one local player
    EntityID firstPlayerID = -1;
    Entities oldAll;

    SystemManager() {
        // Register state manager
        GameState::get().register_on_change(
            std::bind(&SystemManager::on_game_state_change, this,
                      std::placeholders::_1, std::placeholders::_2));
    }

    void render_entities(const Entities& entities, float dt) const;
    void render_ui(const Entities& entities, float dt) const;

    void update_local_players(const Entities& players, float dt);
    void update_all_entities(const Entities& players, float dt);

    void process_inputs(const Entities& entities, const UserInputs& inputs);

   private:
    bool state_transitioned_round_to_planning = false;
    bool state_transitioned_planning_to_round = false;

    void on_game_state_change(game::State new_state, game::State old_state);

    void for_each(
        Entities& entities, float dt,
        const std::function<void(std::shared_ptr<Entity>, float)>& cb) {
        std::ranges::for_each(entities,
                              [cb, dt](std::shared_ptr<Entity> entity) {
                                  if (!entity) return;
                                  cb(entity, dt);
                              });
    }

    void for_each(
        const Entities& entities, float dt,
        const std::function<void(std::shared_ptr<Entity>, float)>& cb) const {
        std::ranges::for_each(std::as_const(entities),
                              [cb, dt](std::shared_ptr<Entity> entity) {
                                  if (!entity) return;
                                  cb(entity, dt);
                              });
    }

    void process_state_change(const Entities& entities, float dt);
    void always_update(const Entities& entity_list, float dt);
    void game_like_update(const Entities& entity_list, float dt);
    void in_round_update(const Entities& entity_list, float dt);
    void planning_update(const Entities& entity_list, float dt);
    void progression_update(const Entities& entity_list, float dt);
};
