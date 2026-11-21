#pragma once

#include "../camera.h"
#include "../engine/layer.h"
#include "../engine/runtime_globals.h"
#include "ah.h"

struct GameLayer : public Layer {
    std::shared_ptr<afterhours::Entity> active_player;
    std::unique_ptr<GameCam> cam;
    raylib::Model bag_model;
    raylib::RenderTexture2D game_render_texture;

    GameLayer() : Layer(strings::menu::GAME), cam(std::make_unique<GameCam>()) {
        globals::set_game_cam(cam.get());
        game_render_texture = raylib::LoadRenderTexture(WIN_W(), WIN_H());
    }

    virtual ~GameLayer();

    // Polling-based input handling (replaces event handlers)
    void handle_input();

    void play_music();

    virtual void onUpdate(float dt) override;

    void draw_world(float dt);
    virtual void onDraw(float dt) override;
};
