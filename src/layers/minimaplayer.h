
#pragma once

#include "../components/can_be_ghost_player.h"
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

struct MinimapLayer : public Layer {
    std::shared_ptr<GameCam> cam;

    MinimapLayer()
        : Layer(strings::menu::GAME), cam(std::make_shared<GameCam>()) {
        cam->updateToTarget({0, -90, 0});
        cam->free_distance_min_clamp = -10.0f;
        cam->free_distance_max_clamp = 200.0f;
    }

    virtual ~MinimapLayer() {}

    virtual void onUpdate(float) override {}

    virtual void onDraw(float dt) override {
        TRACY_ZONE_SCOPED;
        if (!MenuState::s_in_game()) return;

        auto map_ptr = GLOBALS.get_ptr<Map>(strings::globals::MAP);
        if (!map_ptr) return;

        // Only show minimap during lobby
        if (!map_ptr->showMinimap) return;

        raylib::BeginMode3D((*cam).get());
        {
            raylib::rlTranslatef(-5, 0, 7.f);
            float scale = 0.10f;
            raylib::rlScalef(scale, scale, scale);
            raylib::DrawPlane((vec3){0.0f, -TILESIZE, 0.0f},
                              (vec2){40.0f, 40.0f}, DARKGRAY);
            map_ptr->onDraw(dt);
        }
        raylib::EndMode3D();
    }
};
