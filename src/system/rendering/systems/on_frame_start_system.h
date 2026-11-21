#pragma once

#include "../../../ah.h"
#include "../rendering_system.h"

struct OnFrameStartSystem : public ::afterhours::System<> {
    bool should_iterate() const override { return false; }
    virtual bool should_run(const float) override { return true; }

    virtual void once(const float) override {
        system_manager::render_manager::on_frame_start();
    }
};
