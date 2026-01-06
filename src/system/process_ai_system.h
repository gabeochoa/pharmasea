#pragma once

#include "../components/can_pathfind.h"
#include "../components/is_ai_controlled.h"
#include "ai_tags.h"

namespace system_manager {

// TODO should we have a separate system for each job type?
//
// Note: afterhours tag filtering currently only applies on Apple platforms
// (see vendor/afterhours/src/core/system.h). We still guard at runtime on other
// platforms.
struct ProcessAiSystem
    : public afterhours::System<
          IsAIControlled, CanPathfind,
          afterhours::tags::None<afterhours::tags::AITag::AITransitionPending,
                                 afterhours::tags::AITag::AINeedsResetting>> {
    virtual bool should_run(const float) override;

    virtual void for_each_with(Entity& entity, IsAIControlled& ctrl,
                               [[maybe_unused]] CanPathfind&, float dt) override;

   private:
    // Helper functions
    void request_next_state(Entity& entity, IsAIControlled& ctrl,
                            IsAIControlled::State s,
                            bool override_existing = false);
    void wander_pause(Entity& e, IsAIControlled::State resume);
    void set_new_customer_order(Entity& entity);
    void enter_bathroom(Entity& entity, IsAIControlled::State return_to);

    // State processing functions
    void process_state_wander(Entity& entity, IsAIControlled& ctrl, float dt);
    void process_state_queue_for_register(Entity& entity, float dt);
    void process_state_at_register_wait_for_drink(Entity& entity, float dt);
    void process_state_drinking(Entity& entity, float dt);
    void process_state_pay(Entity& entity, float dt);
    void process_state_play_jukebox(Entity& entity, float dt);
    void process_state_bathroom(Entity& entity, float dt);
    void process_state_clean_vomit(Entity& entity, float dt);
    void process_state_leave(Entity& entity, float dt);
};

}  // namespace system_manager
