
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

void world() {
    std::shared_ptr<Cube> cube;
    cube.reset(new Cube((vec3){0.0f, 0.0f, 0.0f}, (Color){255, 0, 0, 255}));

    std::shared_ptr<Player> player;
    player.reset(new Player());

    EntityHelper::addEntity(cube);
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

            DrawText("Congrats! You created your first window!", 190, 200, 20,
                     LIGHTGRAY);
            debug_ui();
        }
        EndDrawing();
    }

    CloseWindow();

    return 0;
}
