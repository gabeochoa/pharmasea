#pragma once

#include "../../../ah.h"
#include "../../../engine/frustum.h"

#define LOG_RENDER_ENT_COUNT 0

#if LOG_RENDER_ENT_COUNT
static size_t num_ents_drawn = 0;
#endif

// Forward declare the static frustum variable from rendering_system.cpp
extern Frustum frustum;

struct OnFrameStartSystem : public ::afterhours::System<> {
    virtual bool should_run(const float) override { return true; }

    virtual void once(const float) override {
#if LOG_RENDER_ENT_COUNT
        log_warn("num entities drawn: {}", num_ents_drawn);
        num_ents_drawn = 0;
#endif
        ::frustum.fetch_data();
    }
};