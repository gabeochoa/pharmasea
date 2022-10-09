
#include "external_include.h"

///
#include "globals.h"
///

#include "camera.h"
#include "world.h"
#include "app.h"
#include "gamelayer.h"

void debug_ui() {
    DrawFPS(0, 0);
    DrawText("Congrats! You created your first window!", 190, 200, 20,
             LIGHTGRAY);
}

int main(void) {
    InitWindow(WIN_W, WIN_H, "pharmasea");
    App app;

    GameLayer* gamelayer = new GameLayer();
    app.pushLayer(gamelayer);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        app.run(dt);
    }

    CloseWindow();

    return 0;
}
