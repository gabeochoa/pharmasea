
#pragma once

#include "external_include.h"
//
#include "event.h"
#include "globals_register.h"
#include "keymap.h"
#include "layer.h"
#include "profile.h"
#include "raylib.h"
#include "singleton.h"

struct AppSettings {
    int width;
    int height;
    const char* title;
    std::function<void(void)> onCreate = {};
};

SINGLETON_FWD(App)
struct App {
    SINGLETON_PARAM(App, AppSettings)

    bool running = false;
    LayerStack layerstack;
    int width;
    int height;

    // TODO create a render texture library?
    RenderTexture2D mainRT;

    explicit App(const AppSettings& settings) {
        width = settings.width;
        height = settings.height;

        InitWindow(width, height, settings.title);

        if (settings.onCreate) settings.onCreate();

        KeyMap::get();

        mainRT = LoadRenderTexture(width, height);
        GLOBALS.set("mainRT", &mainRT);
    }

    ~App() {
        UnloadRenderTexture(mainRT);
        // TODO do we need to / can we remove mainRT from globals
    }

    void pushLayer(Layer* layer) { layerstack.push(layer); }
    void pushOverlay(Layer* layer) { layerstack.pushOverlay(layer); }

    void onEvent(Event& event) {
        EventDispatcher dispatcher(event);
        dispatcher.dispatch<WindowResizeEvent>(
            std::bind(&App::onWindowResize, this, std::placeholders::_1));
    }

    bool onWindowResize(WindowResizeEvent event) {
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

    void processEvent(Event& e) {
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

    void close() { running = false; }

    void run() {
        running = true;
        while (running && !WindowShouldClose()) {
            float dt = GetFrameTime();
            this->loop(dt);
        }
        CloseWindow();
    }

    void loop(float dt) {
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

   private:
    void draw_all_to_texture(float dt) {
        BeginTextureMode(mainRT);
        for (Layer* layer : layerstack) {
            layer->onDraw(dt);
        }
        EndTextureMode();
    }

    void start_post_processing();
    void end_post_processing();

    void render_to_screen() {
        BeginDrawing();
        {
            start_post_processing();
            {
                DrawTextureRec(mainRT.texture,
                               {0, 0, width * 1.f, -height * 1.f}, {0, 0},
                               WHITE);
            }
            end_post_processing();
        }
        EndDrawing();
    }
};
