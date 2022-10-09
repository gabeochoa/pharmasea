#pragma once

#include "camera.h"
#include "external_include.h"
#include "layer.h"
#include "world.h"
#include "menu.h"
#include "input.h"

struct GameLayer : public Layer {
    World world;
    GameCam cam;

    GameLayer() : Layer("Game") { minimized = false; }
    virtual ~GameLayer() {}
    virtual void onAttach() override {}
    virtual void onDetach() override {}

    virtual bool onEvent(int key) override {
        if (key == KEY_ESCAPE) {
            Menu::get().state = Menu::State::Root;
            return true;
        }
        return false;
    }

    virtual void onUpdate(float dt) override {
        if (Menu::get().state != Menu::State::Game) return;
        // Dont quit window on escape
        SetExitKey(KEY_NULL);

        /// 

        // TODO replace passing Player with passing Player*
        // update
        cam.updateToTarget(GLOBALS.get<Player>("player"));
        cam.updateCamera();

        EntityHelper::forEachEntity([&](auto entity) {
            entity->update(dt);
            return EntityHelper::ForEachFlow::None;
        });

        // draw
        BeginDrawing();
        {
            ClearBackground(RAYWHITE);
            DrawText(Menu::get().tostring(), 19, 20, 20, LIGHTGRAY);
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
            }
            EndMode3D();
        }
        EndDrawing();
    }
};
