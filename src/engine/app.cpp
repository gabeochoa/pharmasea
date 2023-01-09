
#include "app.h"

//
#include "globals_register.h"
#include "keymap.h"
#include "raylib.h"
//

#include "log.h"
#include "profile.h"
#include "settings.h"
#include "shader_library.h"

void App::start_post_processing() {
    if (!Settings::get().data.enable_postprocessing) return;
    auto post_processing_shader = ShaderLibrary::get().get("post_processing");
    raylib::BeginShaderMode(post_processing_shader);
}

void App::end_post_processing() {
    if (!Settings::get().data.enable_postprocessing) return;
    raylib::EndShaderMode();
}

App::App(const AppSettings& settings) {
    // This goes above Init since that loves to spew errors
    raylib::SetTraceLogLevel(settings.logLevel);
    raylib::SetTargetFPS(settings.fps);
    //
    width = settings.width;
    height = settings.height;

    raylib::InitWindow(width, height, settings.title);

    KeyMap::create();

    mainRT = raylib::LoadRenderTexture(width, height);
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
    log_info("Got Window Resize Event: {}, {}", event.width, event.height);

    width = event.width;
    height = event.height;

    raylib::SetWindowSize(width, height);

    UnloadRenderTexture(mainRT);
    mainRT = raylib::LoadRenderTexture(width, height);
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
    while (running && !raylib::WindowShouldClose()) {
        float dt = raylib::GetFrameTime();
        this->loop(dt);
    }
    raylib::CloseWindow();
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
    raylib::BeginTextureMode(mainRT);
    for (Layer* layer : layerstack) {
        layer->onDraw(dt);
    }
    raylib::EndTextureMode();
}

void App::render_to_screen() {
    raylib::BeginDrawing();
    {
        App::start_post_processing();
        {
            DrawTextureRec(mainRT.texture, {0, 0, width * 1.f, -height * 1.f},
                           {0, 0}, WHITE);
        }
        App::end_post_processing();
    }
    raylib::EndDrawing();
}
