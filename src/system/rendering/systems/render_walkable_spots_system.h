#pragma once

#include "../../../ah.h"
#include "../../../drawing_util.h"
#include "../../../engine/runtime_globals.h"
#include "../../../entity_helper.h"

struct RenderWalkableSpotsSystem : public ::afterhours::System<> {
    virtual bool should_run(const float) override {
        return globals::debug_ui_enabled();
    }

    virtual void once(const float) override {
        // TODO For some reason this also triggers the walkable.contains
        // segfault
        return;

        if (!globals::debug_ui_enabled()) return;

        for (int i = -25; i < 25; i++) {
            for (int j = -25; j < 25; j++) {
                vec2 pos2 = {static_cast<float>(i), static_cast<float>(j)};
                bool walkable = EntityHelper::isWalkable(pos2);
                if (walkable) continue;

                DrawCubeCustom(vec::to3(pos2), TILESIZE,
                               TILESIZE + TILESIZE / 10.f, TILESIZE, 0,
                               walkable ? ui::color::transleucent_green
                                        : ui::color::transleucent_red,
                               walkable ? ui::color::transleucent_green
                                        : ui::color::transleucent_red);
            }
        }
    }
};