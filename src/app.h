
#pragma once

#include "external_include.h"
//
#include "event.h"
#include "globals.h"
#include "keymap.h"
#include "layer.h"
#include "preload.h"
#include "raylib.h"
#include "singleton.h"

SINGLETON_FWD(App)
struct App {
    SINGLETON(App)

    LayerStack layerstack;

    App() {
        raylib::InitWindow(WIN_W, WIN_H, "pharmasea");
        // Has to happen after init window due to font requirements
        Preload::get();
        KeyMap::get();
    }

    ~App() {}

    void pushLayer(Layer* layer) { layerstack.push(layer); }
    void pushOverlay(Layer* layer) { layerstack.pushOverlay(layer); }

    void onEvent(Event& event) {
        EventDispatcher dispatcher(event);
        dispatcher.dispatch<WindowResizeEvent>(
            std::bind(&App::onWindowResize, this, std::placeholders::_1));
    }

    bool onWindowResize(WindowResizeEvent event) {
        std::cout << "Got Window Resize Event: " << event.width << ", "
                  << event.height << std::endl;
        raylib::SetWindowSize(event.width, event.height);
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

    void run() {
        while (!raylib::WindowShouldClose()) {
            float dt = raylib::GetFrameTime();
            this->loop(dt);
        }
        raylib::CloseWindow();
    }

    void check_input() {
        KeyMap::get().forEachInputInMap(
            std::bind(&App::processEvent, this, std::placeholders::_1));

        KeyMap::get().forEachCharTyped(
            std::bind(&App::processEvent, this, std::placeholders::_1));
    }

    void loop(float dt) {
        for (Layer* layer : layerstack) {
            layer->onUpdate(dt);
        }
        raylib::BeginDrawing();
        for (Layer* layer : layerstack) {
            layer->onDraw(dt);
        }
        raylib::EndDrawing();

        check_input();
    }
};
