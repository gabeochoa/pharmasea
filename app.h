
#pragma once

#include "external_include.h"
#include "layer.h"

struct App {
    LayerStack layerstack;

    void pushLayer(Layer* layer) { layerstack.push(layer); }
    void pushOverlay(Layer* layer) { layerstack.pushOverlay(layer); }

    void run(float dt){
        for (Layer* layer : layerstack) {
            layer->onUpdate(dt);
        }
    }
};
