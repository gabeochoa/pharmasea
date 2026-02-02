#pragma once

#include "../../../ah.h"
#include "../../../components/transform.h"
#include "../../../engine/runtime_globals.h"

struct RenderEntitySystem : public ::afterhours::System<Transform> {
    mutable bool debug_mode_on = false;

    virtual bool should_run(const float) override { return true; }

    virtual void once(const float) const override {
        debug_mode_on = globals::debug_ui_enabled();
    }

    virtual void for_each_with(const Entity& entity, const Transform&,
                               float dt) const override {
        render_manager::render(entity, dt, debug_mode_on);
    }
};