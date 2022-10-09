
#include "external_include.h"

///
#include "globals.h"
///

#include "camera.h"
#include "entity.h"

void debug_ui() {
    DrawFPS(0, 0);
    DrawText("Congrats! You created your first window!", 190, 200, 20,
             LIGHTGRAY);
}

void world() {
    std::shared_ptr<AIPerson> aiperson;
    aiperson.reset(
        new AIPerson((vec3){-TILESIZE, 0.0f, -TILESIZE}, (Color){255, 0, 0, 255}));

    std::shared_ptr<Player> player;
    player.reset(new Player());

    EntityHelper::addEntity(aiperson);
    EntityHelper::addEntity(player);
}

int main(void) {
    InitWindow(WIN_W, WIN_H, "pharmasea");

    Cam cam;
    world();

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // update
        UpdateCamera(cam.get_ptr());

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
                DrawGrid(10, 1.0f);
            }
            EndMode3D();

            debug_ui();
        }
        EndDrawing();
    }

    CloseWindow();

    return 0;
}
