
#pragma once

#include "app.h"
#include "external_include.h"
#include "globals_register.h"
#include "keymap.h"
#include "layer.h"
#include "menu.h"
#include "ui.h"

using namespace ui;

struct PauseLayer : public Layer {
    std::shared_ptr<ui::UIContext> ui_context;
    bool paused = true;

    PauseLayer() : Layer("Pause") {
        ui_context.reset(new ui::UIContext());
        GLOBALS.set("paused", &paused);
    }
    virtual ~PauseLayer() {}

    virtual void onAttach() override { paused = true; }
    virtual void onDetach() override { paused = false; }

    virtual void onEvent(Event& event) override {
        EventDispatcher dispatcher(event);
        dispatcher.dispatch<KeyPressedEvent>(
            std::bind(&PauseLayer::onKeyPressed, this, std::placeholders::_1));
        dispatcher.dispatch<GamepadButtonPressedEvent>(std::bind(
            &PauseLayer::onGamepadButtonPressed, this, std::placeholders::_1));
    }

    bool onGamepadButtonPressed(GamepadButtonPressedEvent& event) {
        if (KeyMap::get_button(Menu::State::Game, "Pause") == event.button) {
            App::get().popLayer(this);
            return true;
        }
        return ui_context.get()->process_gamepad_button_event(event);
    }

    bool onKeyPressed(KeyPressedEvent& event) {
        if (KeyMap::get_key_code(Menu::State::Game, "Pause") == event.keycode) {
            App::get().popLayer(this);
            return true;
        }
        return ui_context.get()->process_keyevent(event);
    }

    virtual void onUpdate(float) override {}

    const SizeExpectation button_x = {.mode = Pixels, .value = 120.f};
    const SizeExpectation button_y = {.mode = Pixels, .value = 50.f};
    const SizeExpectation padd_x = {.mode = Pixels, .value = 120.f};
    const SizeExpectation padd_y = {.mode = Pixels, .value = 25.f};

    std::shared_ptr<Widget> mk_button(uuid id) {
        return ui_context->own(Widget(id, button_x, button_y));
    }

    // NOTE: We specifically dont clear background
    // because people are used to pause menu being an overlay
    virtual bool shouldDrawLayersBehind() const override { return true; }

    virtual void onDraw(float dt) override {
        // TODO move to input
        bool mouseDown = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
        vec2 mousepos = GetMousePosition();

        ui_context->begin(mouseDown, mousepos, dt);

        auto root = ui_context->own(
            Widget({.mode = Pixels, .value = WIN_W, .strictness = 1.f},
                   {.mode = Pixels, .value = WIN_H, .strictness = 1.f},
                   GrowFlags::Row));

        ui_context->push_parent(root);
        {
            auto left_padding = ui_context->own(
                Widget({.mode = Pixels, .value = 100.f, .strictness = 1.f},
                       {.mode = Pixels, .value = WIN_H, .strictness = 1.f}));

            auto content = ui_context->own(Widget(
                {.mode = Children, .strictness = 1.f},
                {.mode = Percent, .value = 1.f, .strictness = 1.0f}, Column));

            padding(*left_padding);
            div(*content);
            ui_context->push_parent(content);
            {
                auto top_padding = ui_context->own(
                    Widget({.mode = Pixels, .value = 100.f, .strictness = 1.f},
                           {.mode = Percent, .value = 1.f, .strictness = 0.f}));
                padding(*top_padding);
                {
                    if (button(*mk_button(MK_UUID(id, ROOT_ID)), "Continue")) {
                        App::get().popLayer(this);
                    }
                    if (button(*mk_button(MK_UUID(id, ROOT_ID)), "Quit")) {
                        App::get().popLayer(this);
                        App::get().popLast();
                    }
                }
                padding(*ui_context->own(Widget(
                    {.mode = Pixels, .value = 100.f, .strictness = 1.f},
                    {.mode = Percent, .value = 1.f, .strictness = 0.f})));
            }
            ui_context->pop_parent();
        }
        ui_context->pop_parent();
        ui_context->end(root.get());
    }
};
