#pragma once

#include "camera.h"
#include "external_include.h"
#include "files.h"
#include "globals.h"
#include "input.h"
#include "layer.h"
#include "menu.h"
#include "world.h"

struct GameLayer : public Layer {
    World world;
    GameCam cam;
    Texture2D facetex;

    GameLayer() : Layer("Game") {
        minimized = false;
        facetex = LoadTexture(
            Files::get().fetch_resource_path("images", "face.png").c_str());
    }
    virtual ~GameLayer() {}
    virtual void onAttach() override {}
    virtual void onDetach() override {}

    virtual void onEvent(Event& event) override {
        EventDispatcher dispatcher(event);
        dispatcher.dispatch<KeyPressedEvent>(
            std::bind(&GameLayer::onKeyPressed, this, std::placeholders::_1));
    }

    bool onKeyPressed(KeyPressedEvent& event) {
        if (event.keycode == KEY_ESCAPE) {
            Menu::get().state = Menu::State::Root;
            return true;
        }
        return false;
    }

    virtual void onUpdate(float dt) override {
        if (Menu::get().state != Menu::State::Game) return;
        if (minimized) return;
        // Dont quit window on escape
        SetExitKey(KEY_NULL);

        // TODO replace passing Player with passing Player*
        // update
        cam.updateToTarget(GLOBALS.get<Player>("player"));
        cam.updateCamera();

        EntityHelper::forEachEntity([&](auto entity) {
            entity->update(dt);
            return EntityHelper::ForEachFlow::None;
        });
    }

    virtual void onDraw(float) override {
        if (Menu::get().state != Menu::State::Game) return;
        if (minimized) return;

        ClearBackground(Color{200, 200, 200, 255});
        BeginMode3D(cam.get());
        {
            EntityHelper::forEachEntity([&](auto entity) {
                entity->render();
                return EntityHelper::ForEachFlow::None;
            });

            EntityHelper::forEachItem([&](auto item) {
                item->render();
                return EntityHelper::ForEachFlow::None;
            });

            DrawGrid(40, TILESIZE);

            DrawBillboard(cam.camera, facetex,
                          {
                              1.f,
                              0.f,
                              1.f,
                          },
                          TILESIZE, WHITE);
        }
        EndMode3D();
    }
};
