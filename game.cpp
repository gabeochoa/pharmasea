
#include "raylib.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/random.hpp>
#include <glm/gtc/type_ptr.hpp>

constexpr int WIN_H = 1080;
constexpr int WIN_W = 1920 / 2;

constexpr int MAP_H = 33;
constexpr int MAP_W = 12;
constexpr int TILESIZE = (WIN_H * 0.95) / MAP_H;
constexpr float WIN_RATIO = WIN_W * 1.f / WIN_H;

struct Tile {
    glm::vec2 position;
    glm::vec4 color;

    void render() {
        auto size = glm::vec2{TILESIZE};
        auto pos = (position * size) + (size / 2.f);
        auto darker = color / 2.f;
        pos += (size * 0.1f);
        size *= 0.8;
    }
};


int main(void)
{
    InitWindow(800, 450, "raylib [core] example - basic window");

    while (!WindowShouldClose())
    {
        BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawText("Congrats! You created your first window!", 190, 200, 20, LIGHTGRAY);
        EndDrawing();
    }

    CloseWindow();

    return 0;
}
