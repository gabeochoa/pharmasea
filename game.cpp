
#include "external_include.h"
#include "globals.h"

constexpr int WIN_H = 1080 / 2;
constexpr int WIN_W = 1920 / 2;

constexpr int MAP_H = 33;
// constexpr int MAP_W = 12;
// constexpr float WIN_RATIO = WIN_W * 1.f / WIN_H;
constexpr int TILESIZE = (WIN_H * 0.95) / MAP_H;

struct Cube {
    vec3 position;
    Color color;

    Cube(vec3 p, Color c) : position(p), color(c) {}

    void render() {
        DrawCube(position, 2.0f, 2.0f, 2.0f, RED);
        DrawCubeWires(position, 2.0f, 2.0f, 2.0f, MAROON);
    }
};

void debug_ui() { DrawFPS(0, 0); }

int main(void) {
    InitWindow(WIN_W, WIN_H, "pharmasea");

    // Define the camera to look into our 3d world
    Camera3D camera = {0};
    camera.position = (vec3){0.0f, 10.0f, 10.0f};  // Camera position
    camera.target = (vec3){0.0f, 0.0f, 0.0f};      // Camera looking at point
    camera.up = (vec3){0.0f, 1.0f, 0.0f};          // Camera up vector (rotation towards target)
    camera.fovy = 45.0f;                           // Camera field-of-view Y
    camera.projection = CAMERA_PERSPECTIVE;        // Camera mode type

    vec3 cubePosition = {0.0f, 0.0f, 0.0f};

    Cube c(cubePosition, (Color){255, 0, 0, 255});

    SetCameraMode(camera, CAMERA_FREE);

    while (!WindowShouldClose()) {
        UpdateCamera(&camera);
        BeginDrawing();
        {
            ClearBackground(RAYWHITE);
            BeginMode3D(camera);
            {
                c.render();
                DrawGrid(10, 1.0f);
            }
            EndMode3D();

            DrawText("Congrats! You created your first window!", 190, 200, 20, LIGHTGRAY);
            debug_ui();
        }
        EndDrawing();
    }

    CloseWindow();

    return 0;
}
