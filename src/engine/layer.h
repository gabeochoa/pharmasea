
#pragma once

#include "../external_include.h"
#include "event.h"
#include "ui/ui.h"

static std::atomic_int s_layer_id;
struct Layer {
    // TODO it would be great to do this here instead of in every sub layer but
    // theres some problem with including ui.h in here due to app.cpp
    // std::shared_ptr<ui::UIContext> ui_context;
    int id;
    std::string name;

    explicit Layer(const std::string& n = "layer")
        : id(s_layer_id++), name(n) {}
    virtual ~Layer() {}
    virtual void onAttach() {}
    virtual void onDetach() {}
    virtual void onUpdate(float elapsed) = 0;
    virtual void onDraw(float elapsed) = 0;

    void onEvent(Event& event) {
        EventDispatcher dispatcher(event);
        dispatcher.dispatch<KeyPressedEvent>(
            [&](KeyPressedEvent& event) { return this->onKeyPressed(event); });
        dispatcher.dispatch<GamepadButtonPressedEvent>(
            [&](GamepadButtonPressedEvent& event) {
                return this->onGamepadButtonPressed(event);
            });
        dispatcher.dispatch<GamepadAxisMovedEvent>(
            [&](GamepadAxisMovedEvent& event) {
                return this->onGamepadAxisMoved(event);
            });
        dispatcher.dispatch<CharPressedEvent>([&](CharPressedEvent& event) {
            return this->onCharPressedEvent(event);
        });
        dispatcher.dispatch<Mouse::MouseMovedEvent>(
            [&](Mouse::MouseMovedEvent& event) {
                return this->onMouseMoved(event);
            });
        dispatcher.dispatch<Mouse::MouseScrolledEvent>(
            [&](Mouse::MouseScrolledEvent& event) {
                return this->onMouseScrolled(event);
            });
        dispatcher.dispatch<Mouse::MouseButtonPressedEvent>(
            [&](Mouse::MouseButtonPressedEvent& event) {
                return this->onMouseButtonPressed(event);
            });
        dispatcher.dispatch<Mouse::MouseButtonReleasedEvent>(
            [&](Mouse::MouseButtonReleasedEvent& event) {
                return this->onMouseButtonReleased(event);
            });
        dispatcher.dispatch<Mouse::MouseButtonUpEvent>(
            [&](Mouse::MouseButtonUpEvent& event) {
                return this->onMouseButtonUp(event);
            });
        dispatcher.dispatch<Mouse::MouseButtonDownEvent>(
            [&](Mouse::MouseButtonDownEvent& event) {
                return this->onMouseButtonDown(event);
            });
    }

    virtual bool onKeyPressed(KeyPressedEvent&) { return false; }
    virtual bool onGamepadButtonPressed(GamepadButtonPressedEvent&) {
        return false;
    }
    virtual bool onGamepadAxisMoved(GamepadAxisMovedEvent&) { return false; }
    virtual bool onCharPressedEvent(CharPressedEvent&) { return false; }

    virtual bool onMouseMoved(Mouse::MouseMovedEvent&) { return false; }
    virtual bool onMouseScrolled(Mouse::MouseScrolledEvent&) { return false; }
    virtual bool onMouseButtonPressed(Mouse::MouseButtonPressedEvent&) {
        return false;
    }
    virtual bool onMouseButtonReleased(Mouse::MouseButtonReleasedEvent&) {
        return false;
    }
    virtual bool onMouseButtonUp(Mouse::MouseButtonUpEvent&) { return false; }
    virtual bool onMouseButtonDown(Mouse::MouseButtonDownEvent&) {
        return false;
    }

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

    template<size_t N>
    void pushAllLayers(const std::array<Layer*, N>& new_layers) {
        for (auto& layer : new_layers) {
            push(layer);
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
