
#pragma once

#include "external_include.h"
#include "layer.h"
#include "globals.h"
#include "input.h"

struct App {
    LayerStack layerstack;

    App() {
        InitWindow(WIN_W, WIN_H, "pharmasea");
        // Disable global esc to close window
        SetExitKey(KEY_NULL);
    }

    void pushLayer(Layer* layer) { layerstack.push(layer); }
    void pushOverlay(Layer* layer) { layerstack.pushOverlay(layer); }

    void run(float dt) {
        input.onUpdate(dt);
        for (Layer* layer : layerstack) {
            layer->onUpdate(dt);
        }

        for (auto event: input.pressedSinceLast) {
            for (Layer* layer : layerstack) {
                bool resp = layer->onEvent(event);
                if(resp){
                    // we ate the event
                    // go to next event
                    break; 
                }
            }
        }
    }
};
