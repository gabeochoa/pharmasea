#pragma once

#include "../engine.h"
#include "../engine/app.h"
#include "../engine/ui.h"
#include "../engine/util.h"
#include "../external_include.h"

struct UITestLayer : public Layer {
    std::shared_ptr<ui::UIContext> ui_context;

    UITestLayer()
        : Layer("UITest"), ui_context(std::make_shared<ui::UIContext>()) {}

    virtual ~UITestLayer() {}

    bool onGamepadAxisMoved(GamepadAxisMovedEvent& event) override {
        if (MenuState::get().is_not(menu::State::Root)) return false;
        return ui_context.get()->process_gamepad_axis_event(event);
    }

    bool onKeyPressed(KeyPressedEvent& event) override {
        if (MenuState::get().is_not(menu::State::Root)) return false;
        return ui_context.get()->process_keyevent(event);
    }

    bool onGamepadButtonPressed(GamepadButtonPressedEvent& event) override {
        if (MenuState::get().is_not(menu::State::Root)) return false;
        return ui_context.get()->process_gamepad_button_event(event);
    }

    virtual void onUpdate(float) override {
        if (MenuState::get().is_not(menu::State::UI)) return;
        raylib::SetExitKey(raylib::KEY_ESCAPE);
    }

    virtual void onDraw(float dt) override {
        if (MenuState::get().is_not(menu::State::UI)) return;
        ext::clear_background(ui_context->active_theme().background);
        using namespace ui;

        begin(ui_context, dt);

        auto window = Rectangle{0, 0, WIN_WF(), WIN_HF()};
        auto [top, rest] = rect::hsplit(window, 33);
        auto [body, footer] = rect::hsplit(rest, 66);
        auto [drop, _a, _b] = rect::hsplit<3>(rect::hpad(body, 30), 10);

        std::vector<std::string> options = {{
            "option1",
            "option2",
            "option3",
            "option4",
            "option5",
            "option6",
            "option7",
            "option8",
            "option9",
            "option10",
            "option11",
            "option12",
            "option13",
            "option14",
            "option15",
        }};
        if (auto result = dropdown(Widget{drop},
                                   DropdownData{
                                       options,
                                       0,
                                   });
            result) {
            log_info("option chosen {}", result.as<int>());
        }

        end();
    }
};
