
#pragma once

#include "../drawing_util.h"
#include "../engine/ui_color.h"
#include "../external_include.h"
//
#include "../globals.h"
//
#include "../camera.h"
#include "../engine.h"
#include "../map.h"
#include "raylib.h"

struct WorkbenchLayer : public Layer {
    WorkbenchLayer() : Layer(strings::menu::GAME) {}

    virtual ~WorkbenchLayer() {}

    virtual void onUpdate(float) override {}

    virtual void onDraw(float) override {
        TRACY_ZONE_SCOPED;
        if (!MenuState::s_in_game()) return;

        auto map_ptr = GLOBALS.get_ptr<Map>(strings::globals::MAP);
        if (!map_ptr) return;
        if (!map_ptr->showMenu) return;

        DrawRectangle(100, 100, 1000, 1000, BLACK);
    }
};
