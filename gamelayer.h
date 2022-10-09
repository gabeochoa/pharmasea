#pragma once 

#include "external_include.h"
#include "layer.h"

#include "world.h"
#include "camera.h"

struct GameLayer : public Layer {
    // TODO Move these into globals? 
    World world;
    Cam cam;

    GameLayer() : Layer("Game") {
    }
    virtual ~GameLayer() {}
    virtual void onAttach() override {}
    virtual void onDetach() override {}

    virtual void onUpdate(float dt) override {
        if(minimized) {
            return;
        }
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
            BeginMode3D(cam.get());
            {
                EntityHelper::forEachEntity([&](auto entity) {
                    entity->render();
                    return EntityHelper::ForEachFlow::None;
                });
                DrawGrid(40, TILESIZE);
            }
            EndMode3D();
        }
        EndDrawing();

    }

};

