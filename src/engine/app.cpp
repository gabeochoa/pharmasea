
#include "app.h"

//
#include "../external_include.h"
#include "globals_register.h"
#include "keymap.h"
#include "raylib.h"
//

#include "../settings.h"
#include "../shader_library.h"
#include "profile.h"

void App::start_post_processing() {
    if (!Settings::get().data.enable_postprocessing) return;
    auto post_processing_shader = ShaderLibrary::get().get("post_processing");
    BeginShaderMode(post_processing_shader);
}

void App::end_post_processing() {
    if (!Settings::get().data.enable_postprocessing) return;
    EndShaderMode();
}

App::App(const AppSettings& settings) {
    // This goes above Init since that loves to spew errors
    SetTraceLogLevel(settings.logLevel);
    SetTargetFPS(settings.fps);
    //
    width = settings.width;
    height = settings.height;

    InitWindow(width, height, settings.title);

    if (settings.onCreate) settings.onCreate();

    KeyMap::get();

    mainRT = LoadRenderTexture(width, height);
    GLOBALS.set("mainRT", &mainRT);
}

App::~App() {
    UnloadRenderTexture(mainRT);
    // TODO do we need to / can we remove mainRT from globals
}

void App::pushLayer(Layer* layer) { layerstack.push(layer); }
void App::pushOverlay(Layer* layer) { layerstack.pushOverlay(layer); }

void App::onEvent(Event& event) {
    EventDispatcher dispatcher(event);
    dispatcher.dispatch<WindowResizeEvent>(
        std::bind(&App::onWindowResize, this, std::placeholders::_1));
}

bool App::onWindowResize(WindowResizeEvent event) {
    // std::cout << "Got Window Resize Event: " << event.width << ", "
    // << event.height << std::endl;

    width = event.width;
    height = event.height;

    SetWindowSize(width, height);

    UnloadRenderTexture(mainRT);
    mainRT = LoadRenderTexture(width, height);
    GLOBALS.set("mainRT", &mainRT);
    return true;
}

void App::processEvent(Event& e) {
    this->onEvent(e);
    if (e.handled) {
        return;
    }

    // Have the top most layers get the event first,
    // if they handle it then no need for the lower ones to get the rest
    // eg imagine UI pause menu blocking game UI elements
    //    we wouldnt want the player to click pass the pause menu
    for (auto it = layerstack.end(); it != layerstack.begin();) {
        (*--it)->onEvent(e);
        if (e.handled) {
            break;
        }
    }
}

void App::close() { running = false; }

void App::run() {
    running = true;
    while (running && !WindowShouldClose()) {
        float dt = GetFrameTime();
        this->loop(dt);
    }
    CloseWindow();
}

void App::loop(float dt) {
    PROFILE();

    for (Layer* layer : layerstack) {
        layer->onUpdate(dt);
    }

    draw_all_to_texture(dt);
    render_to_screen();

    // Check Input
    {
        KeyMap::get().forEachInputInMap(
            std::bind(&App::processEvent, this, std::placeholders::_1));

        KeyMap::get().forEachCharTyped(
            std::bind(&App::processEvent, this, std::placeholders::_1));
    }
}

void App::draw_all_to_texture(float dt) {
    BeginTextureMode(mainRT);
    for (Layer* layer : layerstack) {
        layer->onDraw(dt);
    }
    EndTextureMode();
}

void App::render_to_screen() {
    BeginDrawing();
    {
        start_post_processing();
        {
            DrawTextureRec(mainRT.texture, {0, 0, width * 1.f, -height * 1.f},
                           {0, 0}, WHITE);
        }
        end_post_processing();
    }
    EndDrawing();
}
