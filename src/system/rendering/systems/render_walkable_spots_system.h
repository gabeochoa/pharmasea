#pragma once

#include "../../../ah.h"
#include "../../../engine/runtime_globals.h"

struct RenderWalkableSpotsSystem : public ::afterhours::System<> {
    virtual bool should_run(const float) override {
        return globals::debug_ui_enabled();
    }

    virtual void once(const float) override {
        render_manager::render_walkable_spots(0);
    }
};