#pragma once

#include "../ah.h"
#include "../components/is_trigger_area.h"
#include "../entity_helper.h"
#include "system_manager.h"

namespace system_manager {

// Forward declarations for helper functions in system_manager.cpp
void update_dynamic_trigger_area_settings(Entity& entity, float dt);
void count_all_possible_trigger_area_entrants(Entity& entity, float dt);
void count_in_building_trigger_area_entrants(Entity& entity, float dt);
void count_trigger_area_entrants(Entity& entity, float dt);
void update_trigger_area_percent(Entity& entity, float dt);
void trigger_cb_on_full_progress(Entity& entity, float dt);

struct UpdateDynamicTriggerAreaSettingsSystem
    : public afterhours::System<IsTriggerArea> {
    virtual bool should_run(const float) override { return true; }

    virtual void for_each_with(Entity& entity, IsTriggerArea&,
                               float dt) override {
        update_dynamic_trigger_area_settings(entity, dt);
    }
};

struct CountAllPossibleTriggerAreaEntrantsSystem
    : public afterhours::System<IsTriggerArea> {
    virtual bool should_run(const float) override { return true; }

    virtual void for_each_with(Entity& entity, IsTriggerArea&,
                               float dt) override {
        count_all_possible_trigger_area_entrants(entity, dt);
    }
};

struct CountInBuildingTriggerAreaEntrantsSystem
    : public afterhours::System<IsTriggerArea> {
    virtual bool should_run(const float) override { return true; }

    virtual void for_each_with(Entity& entity, IsTriggerArea&,
                               float dt) override {
        count_in_building_trigger_area_entrants(entity, dt);
    }
};

struct CountTriggerAreaEntrantsSystem
    : public afterhours::System<IsTriggerArea> {
    virtual bool should_run(const float) override { return true; }

    virtual void for_each_with(Entity& entity, IsTriggerArea&,
                               float dt) override {
        count_trigger_area_entrants(entity, dt);
    }
};

struct UpdateTriggerAreaPercentSystem
    : public afterhours::System<IsTriggerArea> {
    virtual bool should_run(const float) override { return true; }

    virtual void for_each_with(Entity& entity, IsTriggerArea&,
                               float dt) override {
        update_trigger_area_percent(entity, dt);
    }
};

struct TriggerCbOnFullProgressSystem
    : public afterhours::System<IsTriggerArea> {
    virtual bool should_run(const float) override { return true; }

    virtual void for_each_with(Entity& entity, IsTriggerArea&,
                               float dt) override {
        trigger_cb_on_full_progress(entity, dt);
    }
};

}  // namespace system_manager
