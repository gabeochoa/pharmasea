
#pragma once

#include "../external_include.h"
#include "event.h"
#include "globals.h"

[[nodiscard]] inline raylib::Rectangle WIN_R() {
    return raylib::Rectangle{0, 0, WIN_WF(), WIN_HF()};
}

static std::atomic_int s_layer_id;
struct Layer {
    // TODO it would be great to do this here instead of in every sub layer but
    // theres some problem with including ui.h in here due to app.cpp
    // std::shared_ptr<ui::UIContext> ui_context;
    int id;
    std::string name;

    Layer(const std::string& n = "layer") : id(s_layer_id++), name(n) {}
    virtual ~Layer() {}
    virtual void onAttach() {}
    virtual void onDetach() {}
    virtual void onUpdate(float elapsed) = 0;
    virtual void onDraw(float elapsed) = 0;

    void onEvent(Event& event) {
        EventDispatcher dispatcher(event);
        dispatcher.dispatch<KeyPressedEvent>(
            std::bind(&Layer::onKeyPressed, this, std::placeholders::_1));
        dispatcher.dispatch<GamepadButtonPressedEvent>(std::bind(
            &Layer::onGamepadButtonPressed, this, std::placeholders::_1));
        dispatcher.dispatch<GamepadAxisMovedEvent>(
            std::bind(&Layer::onGamepadAxisMoved, this, std::placeholders::_1));
        dispatcher.dispatch<CharPressedEvent>(
            std::bind(&Layer::onCharPressedEvent, this, std::placeholders::_1));
    }

    virtual bool onKeyPressed(KeyPressedEvent&) { return false; }
    virtual bool onGamepadButtonPressed(GamepadButtonPressedEvent&) {
        return false;
    }
    virtual bool onGamepadAxisMoved(GamepadAxisMovedEvent&) { return false; }
    virtual bool onCharPressedEvent(CharPressedEvent&) { return false; }

    [[nodiscard]] const std::string& getname() const { return name; }
};

struct LayerStack {
    std::vector<Layer*> layers;
    std::vector<Layer*>::iterator insert;

    std::vector<Layer*>::iterator begin() { return layers.begin(); }
    std::vector<Layer*>::iterator end() { return layers.end(); }
    std::vector<Layer*>::reverse_iterator rbegin() { return layers.rbegin(); }
    std::vector<Layer*>::reverse_iterator rend() { return layers.rend(); }

    LayerStack() { insert = layers.begin(); }
    ~LayerStack() {
        for (Layer* layer : layers) {
            delete layer;
        }
    }

    void push(Layer* layer) {
        if (layers.empty()) {
            layers.push_back(layer);
            insert = layers.begin();
        } else {
            insert = layers.emplace(insert, layer);
        }
    }
    void pop(Layer* layer) {
        auto it = std::find(layers.begin(), layers.end(), layer);
        if (it != layers.end()) {
            layers.erase(it);
            --insert;
        }
    }

    void popOverlay(Layer* layer) { layers.emplace_back(layer); }
    void pushOverlay(Layer* layer) {
        auto it = std::find(layers.begin(), layers.end(), layer);
        if (it != layers.end()) {
            layers.erase(it);
        }
    }
};
