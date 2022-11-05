
#pragma once

#include "event.h"
#include "external_include.h"

static std::atomic_int s_layer_id;
struct Layer {
    int id;
    std::string name;
    bool minimized;
    bool cleanup = false;

    bool is_minimized() { return this->minimized; }

    Layer(const std::string& n = "layer")
        : id(s_layer_id++), name(n), minimized(false) {}
    virtual ~Layer() {}
    virtual void onAttach() { std::cout << "attached : " << name << std::endl; }
    virtual void onDetach() { std::cout << "detached : " << name << std::endl; }
    virtual void onUpdate(float elapsed) = 0;
    virtual void onDraw(float elapsed) = 0;
    virtual void onEvent(Event&) = 0;
    virtual bool shouldDrawLayersBehind() const { return false; }

    const std::string& getname() const { return name; }
};

struct LayerStack {
    std::vector<Layer*> layers;
    std::vector<Layer*>::iterator insert;

    std::vector<Layer*>::iterator begin() { return layers.begin(); }
    std::vector<Layer*>::iterator end() { return layers.end(); }

    std::vector<Layer*>::reverse_iterator rbegin() { return layers.rbegin(); }
    std::vector<Layer*>::reverse_iterator rend() { return layers.rend(); }

    void clear() { layers.clear(); }

    LayerStack() { insert = layers.begin(); }
    ~LayerStack() {
        for (Layer* layer : layers) {
            delete layer;
        }
    }

    void push(Layer* layer) { layers.push_back(layer); }
    void pop(Layer* layer) {
        auto it = std::find(layers.begin(), layers.end(), layer);
        if (it != layers.end()) {
            layers.erase(it);
            --insert;
        }
    }

    void cleanup_layers() {
        std::erase_if(layers,
                      [](const Layer* layer) { return layer->cleanup; });
    }
};
