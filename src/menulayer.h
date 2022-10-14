#pragma once

#include "external_include.h"
#include "layer.h"
#include "menu.h"
#include "raylib.h"
#include "settings.h"
#include "ui.h"
#include "uuid.h"

struct MenuLayer : public Layer {
    std::shared_ptr<ui::UIContext> ui_context;

    MenuLayer() : Layer("Menu") {
        minimized = false;

        ui_context.reset(new ui::UIContext());
        ui_context.get()->init();
    }
    virtual ~MenuLayer() {}
    virtual void onAttach() override {}
    virtual void onDetach() override {}

    virtual void onEvent(Event& event) override {
        EventDispatcher dispatcher(event);
        dispatcher.dispatch<KeyPressedEvent>(
            std::bind(&MenuLayer::onKeyPressed, this, std::placeholders::_1));
        dispatcher.dispatch<GamepadButtonPressedEvent>(std::bind(
            &MenuLayer::onGamepadButtonPressed, this, std::placeholders::_1));
    }

    bool onKeyPressed(KeyPressedEvent& event) {
        if (Menu::get().state != Menu::State::Root) return false;
        return ui_context.get()->process_keyevent(event);
    }

    bool onGamepadButtonPressed(GamepadButtonPressedEvent& event) {
        return ui_context.get()->process_gamepad_button_event(event);
    }

    void draw_ui() {
        using namespace ui;

        // TODO move to input
        bool mouseDown = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
        vec2 mousepos = GetMousePosition();

        ui_context->begin(mouseDown, mousepos);

        const vec2 b_size = {140.f, 50.f};

        if (button(MK_UUID(id, ROOT_ID), WidgetConfig({
                                             .position = vec2{50.f, 150.f},
                                             .size = b_size,
                                             .text = std::string("Play"),
                                         }))) {
            Menu::get().state = Menu::State::Game;
        }

        if (button(MK_UUID(id, ROOT_ID), WidgetConfig({
                                             .position = vec2{50.f, 225.f},
                                             .size = b_size,
                                             .text = std::string("About"),
                                         }))) {
            Menu::get().state = Menu::State::About;
        }

        if (button(MK_UUID(id, ROOT_ID), WidgetConfig({
                                             .position = vec2{50.f, 325.f},
                                             .size = b_size,
                                             .text = std::string("Settings"),
                                         }))) {
            Menu::get().state = Menu::State::Settings;
        }

        ui_context->end();
    }

    virtual void onUpdate(float) override {
        if (Menu::get().state != Menu::State::Root) return;
        SetExitKey(KEY_ESCAPE);
    }

    virtual void onDraw(float) override {
        if (Menu::get().state != Menu::State::Root) return;
        if (minimized) {
            return;
        }
        ClearBackground(Color{30, 30, 30, 255});
        draw_ui();
    }
};
