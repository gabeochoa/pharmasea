
#pragma once

#include "event.h"
#include "external_include.h"
#include "globals.h"
#include "input.h"
#include "layer.h"

struct App {
    LayerStack layerstack;

    App() {
        InitWindow(WIN_W, WIN_H, "pharmasea");
        // Disable global esc to close window
        SetExitKey(KEY_NULL);
    }

    void pushLayer(Layer* layer) { layerstack.push(layer); }
    void pushOverlay(Layer* layer) { layerstack.pushOverlay(layer); }

    void processEvent(Event& e) {
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

    void run(float dt) {
        Input::get().onUpdate(dt);

        for (Layer* layer : layerstack) {
            layer->onUpdate(dt);
        }
        BeginDrawing();
        for (Layer* layer : layerstack) {
            layer->onDraw(dt);
        }
        EndDrawing();

        for (int key : Input::get().pressedSinceLast) {
            KeyPressedEvent* event = new KeyPressedEvent(key, 0);
            this->processEvent(*event);
            delete event;
        }
    }
};
