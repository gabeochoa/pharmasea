#pragma once

#include "external_include.h"
#include "input.h"
#include "layer.h"
#include "menu.h"
#include "raylib.h"
#include "ui.h"
#include "uuid.h"
#include "settings.h"

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
    }

    bool onKeyPressed(KeyPressedEvent& event) {
        if (Menu::get().state != Menu::State::Root) return false;
        return ui_context.get()->process_keyevent(event);
    }

    void draw_ui() {
        using namespace ui;

        // TODO move to input
        bool mouseDown = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
        vec2 mousepos = GetMousePosition();

        ui_context->begin(mouseDown, mousepos);

        if (button(MK_UUID(id, ROOT_ID), WidgetConfig({
                                             .position = vec2{50.f, 150.f},
                                             .size = vec2{100.f, 50.f},
                                             .text = std::string("Play"),
                                         }))) {
            Menu::get().state = Menu::State::Game;
        }

        if (button(MK_UUID(id, ROOT_ID), WidgetConfig({
                                             .position = vec2{50.f, 225.f},
                                             .size = vec2{120.f, 50.f},
                                             .text = std::string("About"),
                                         }))) {
            Menu::get().state = Menu::State::About;
        }

        if (button(MK_UUID(id, ROOT_ID),
                   WidgetConfig({
                       .position = vec2{50.f, 325.f},
                       .size = vec2{175.f, 50.f},
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
