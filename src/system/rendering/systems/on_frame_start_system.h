#pragma once

#include "../../../ah.h"

struct OnFrameStartSystem : public ::afterhours::System<> {
    virtual bool should_run(const float) override { return true; }

    virtual void once(const float) override {
        render_manager::on_frame_start();
    }
};