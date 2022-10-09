
#include "external_include.h"

///
#include "globals.h"
///

#include "camera.h"
#include "world.h"

void debug_ui() {
    DrawFPS(0, 0);
    DrawText("Congrats! You created your first window!", 190, 200, 20,
             LIGHTGRAY);
}

int main(void) {
    InitWindow(WIN_W, WIN_H, "pharmasea");

    // TODO Move these into globals? 
    World world;
    Cam cam;
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

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

            debug_ui();
        }
        EndDrawing();
    }

    CloseWindow();

    return 0;
}
