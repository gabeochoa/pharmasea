
#pragma once

#include "external_include.h"
#include "input.h"
#include "layer.h"
#include "menu.h"
#include "raylib.h"
#include "ui.h"
#include "uuid.h"

struct AboutLayer : public Layer {

    std::shared_ptr<ui::UIContext> ui_context;

    AboutLayer() : Layer("About") {
        minimized = false;

        ui_context.reset(new ui::UIContext());
        ui_context.get()->init();
    }
    virtual ~AboutLayer() {}
    virtual void onAttach() override {}
    virtual void onDetach() override {}

    virtual void onEvent(Event& event) override {
        EventDispatcher dispatcher(event);
        dispatcher.dispatch<KeyPressedEvent>(
            std::bind(&AboutLayer::onKeyPressed, this, std::placeholders::_1));
    }

    bool onKeyPressed(KeyPressedEvent& event) {
        if (Menu::get().state != Menu::State::About) return false;
        if (event.keycode == KEY_ESCAPE) {
            Menu::get().state = Menu::State::Root;
            return true;
        }
        return ui_context.get()->process_keyevent(event);
    }

    void draw_ui() {
        using namespace ui;

        // TODO move to input
        bool mouseDown = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
        vec2 mousepos = GetMousePosition();

        ui_context->begin(mouseDown, mousepos);

        // NOTE: this is not aligned on purpose
        std::string about_info = R"(
PharmaSea

A game by: 
    Gabe 
    Brett
    Alice

        )";
        text(MK_UUID(id, ROOT_ID), WidgetConfig({
                                       .position = vec2{50.f, 50.f},
                                       .size = vec2{30.f, 5.f},
                                       .text = about_info,
                                   }));

        if (button(MK_UUID(id, ROOT_ID), WidgetConfig({
                                             .position = vec2{50.f, 400.f},
                                             .size = vec2{100.f, 50.f},
                                             .text = std::string("Back"),
                                         }))) {
            Menu::get().state = Menu::State::Root;
        }

        ui_context->end();
    }

    virtual void onUpdate(float) override {
        if (Menu::get().state != Menu::State::About) return;
        SetExitKey(KEY_NULL);

        // TODO with gamelayer, support events
        if (minimized) {
            return;
        }
    }

    virtual void onDraw(float) override {
        if (Menu::get().state != Menu::State::About) return;
        // TODO with gamelayer, support events
        if (minimized) {
            return;
        }

        ClearBackground(BLACK);
        draw_ui();
    }
};
