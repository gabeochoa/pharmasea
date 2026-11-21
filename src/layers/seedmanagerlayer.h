#pragma once

#include "../camera.h"
#include "../engine/layer.h"

struct Map;

struct SeedManagerLayer : public Layer {
    std::unique_ptr<GameCam> cam;
    std::shared_ptr<ui::UIContext> ui_context;
    Map* map_ptr = nullptr;
    // We use a temp string because we dont want to touch the real one until the
    // user says Okay
    std::string tempSeed;

    SeedManagerLayer()
        : Layer(strings::menu::GAME),
          cam(std::make_unique<GameCam>()),
          ui_context(std::make_shared<ui::UIContext>()) {
        cam->updateToTarget({0, -90, 0});
        cam->free_distance_min_clamp = -10.0f;
        cam->free_distance_max_clamp = 200.0f;
    }

    virtual ~SeedManagerLayer() {}

    bool is_user_host();
    void handleInput();
    virtual void onUpdate(float) override;
    void draw_seed_input(float dt);
    void draw_minimap(float dt);
    virtual void onDraw(float dt) override;
};
