
#include "external_include.h"

constexpr int WIN_H = 1080 / 2;
constexpr int WIN_W = 1920 / 2;

constexpr int MAP_H = 33;
// constexpr int MAP_W = 12;
// constexpr float WIN_RATIO = WIN_W * 1.f / WIN_H;
constexpr int TILESIZE = (WIN_H * 0.95) / MAP_H;

#include "camera.h"
#include "entity.h"
#include "globals.h"

void debug_ui() { DrawFPS(0, 0); }

int main(void) {
    InitWindow(WIN_W, WIN_H, "pharmasea");

    Cam cam;
    Cube c((vec3){0.0f, 0.0f, 0.0f}, (Color){255, 0, 0, 255});

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // update 
        UpdateCamera(cam.get_ptr());
        player.update(dt);

        // draw 
        BeginDrawing();
        {
            ClearBackground(RAYWHITE);
            BeginMode3D(cam.get());
            {
                c.render();
                player.render();
                DrawGrid(10, 1.0f);
            }
            EndMode3D();

            DrawText("Congrats! You created your first window!", 190, 200, 20,
                     LIGHTGRAY);
            debug_ui();
        }
        EndDrawing();
    }

    CloseWindow();

    return 0;
}
