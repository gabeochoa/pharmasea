#pragma once

#include "../camera.h"
#include "../engine/layer.h"

struct Entity;

struct GameLayer : public Layer {
    std::shared_ptr<Entity> active_player;
    std::unique_ptr<GameCam> cam;
    raylib::Model bag_model;
    raylib::RenderTexture2D game_render_texture;

    GameLayer() : Layer(strings::menu::GAME), cam(std::make_unique<GameCam>()) {
        GLOBALS.set(strings::globals::GAME_CAM, cam.get());
        game_render_texture = raylib::LoadRenderTexture(WIN_W(), WIN_H());
    }

    virtual ~GameLayer();

    bool onGamepadAxisMoved(GamepadAxisMovedEvent&) override;
    bool onGamepadButtonPressed(GamepadButtonPressedEvent& event) override;
    bool onKeyPressed(KeyPressedEvent& event) override;
    bool onMouseButtonUp(Mouse::MouseButtonUpEvent& event) override;
    bool onMouseButtonDown(Mouse::MouseButtonDownEvent& event) override;
    void play_music();

    virtual void onUpdate(float dt) override;

    void draw_world(float dt);
    virtual void onDraw(float dt) override;
};
