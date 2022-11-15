
#pragma once

#include "external_include.h"
#include "globals_register.h"
#include "keymap.h"
#include "layer.h"
#include "menu.h"
#include "ui.h"

using namespace ui;

struct BasePauseLayer : public Layer {
    std::shared_ptr<ui::UIContext> ui_context;
    Menu::State back_state;
    Menu::State enabled_state;

    BasePauseLayer(const char* name, Menu::State b_state, Menu::State e_state)
        : Layer(name), back_state(b_state), enabled_state(e_state) {
        ui_context.reset(new ui::UIContext());
    }
    virtual ~BasePauseLayer() {}

    virtual void onEvent(Event& event) override {
        if (Menu::get().state != enabled_state) return;
        EventDispatcher dispatcher(event);
        dispatcher.dispatch<KeyPressedEvent>(std::bind(
            &BasePauseLayer::onKeyPressed, this, std::placeholders::_1));
        dispatcher.dispatch<GamepadButtonPressedEvent>(
            std::bind(&BasePauseLayer::onGamepadButtonPressed, this,
                      std::placeholders::_1));
    }

    bool onGamepadButtonPressed(GamepadButtonPressedEvent& event) {
        if (KeyMap::get_button(Menu::State::Game, InputName::Pause) ==
            event.button) {
            Menu::get().state = back_state;
            return true;
        }
        return ui_context.get()->process_gamepad_button_event(event);
    }

    bool onKeyPressed(KeyPressedEvent& event) {
        if (KeyMap::get_key_code(Menu::State::Game, InputName::Pause) ==
            event.keycode) {
            Menu::get().state = back_state;
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

    virtual void onDraw(float dt) override {
        if (Menu::get().state != enabled_state) return;

        // NOTE: We specifically dont clear background
        // because people are used to pause menu being an overlay

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
                        Menu::get().state = back_state;
                    }
                    if (button(*mk_button(MK_UUID(id, ROOT_ID)), "Quit")) {
                        Menu::get().state = Menu::State::Root;
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

struct PauseLayer : public BasePauseLayer {
    PauseLayer()
        : BasePauseLayer("Pause", Menu::State::Game, Menu::State::Paused) {}
    virtual ~PauseLayer() {}
};

struct PausePlanningLayer : public BasePauseLayer {
    PausePlanningLayer()
        : BasePauseLayer("Pause", Menu::State::Planning,
                         Menu::State::PausedPlanning) {}
    virtual ~PausePlanningLayer() {}
};
