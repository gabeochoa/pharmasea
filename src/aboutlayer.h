
#pragma once

#include "app.h"
#include "external_include.h"
#include "layer.h"
#include "menu.h"
#include "raylib.h"
#include "ui.h"
#include "ui_autolayout.h"
#include "ui_theme.h"
#include "uuid.h"

struct AboutLayer : public Layer {
    std::shared_ptr<ui::UIContext> ui_context;

    AboutLayer() : Layer("About") {
        minimized = false;

        ui_context.reset(new ui::UIContext());

        ui_context.get()->init();
        ui_context.get()->set_font(Preload::get().font);
        // TODO we should probably enforce that you cant do this
        // and we should have ->set_base_theme()
        // and push_theme separately, if you end() with any stack not empty...
        // thats a flag
        ui_context->push_theme(ui::DEFAULT_THEME);
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
        if (Menu::get().is_not(Menu::State::About)) return false;
        if (event.keycode == KEY_ESCAPE) {
            Menu::get().go_back();
            return true;
        }
        return ui_context.get()->process_keyevent(event);
    }

    void draw_ui(float dt) {
        SetExitKey(KEY_ESCAPE);
        using namespace ui;

        // TODO move to input
        bool mouseDown = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
        vec2 mousepos = GetMousePosition();

        ui_context->begin(mouseDown, mousepos, dt);
        ui_context->push_theme(DEFAULT_THEME);

        ui::Widget root;
        root.set_expectation({.mode = ui::SizeMode::Pixels,
                              .value = WIN_WF(),
                              .strictness = 1.f},
                             {.mode = ui::SizeMode::Pixels,
                              .value = WIN_HF(),
                              .strictness = 1.f});
        root.growflags = ui::GrowFlags::Row;

        Widget left_padding(
            {.mode = Pixels, .value = 100.f, .strictness = 1.f},
            {.mode = Pixels, .value = WIN_HF(), .strictness = 1.f});

        Widget content({.mode = Children},
                       {.mode = Percent, .value = 1.f, .strictness = 1.0f});
        content.growflags = ui::GrowFlags::Column;

        Widget top_padding({.mode = Pixels, .value = 100.f, .strictness = 1.f},
                           {.mode = Percent, .value = 0.5f, .strictness = 0.f});

        Widget bottom_padding(
            {.mode = Pixels, .value = 50.f, .strictness = 1.f},
            {.mode = Percent, .value = 0.5f, .strictness = 0.f});

        Widget about_text({.mode = Pixels, .value = 200.f},
                          {.mode = Pixels, .value = 400.f});

        Widget back_button(MK_UUID(id, ROOT_ID),
                           {.mode = Pixels, .value = 120.f},
                           {.mode = Pixels, .value = 50.f});

        // NOTE: this is not aligned on purpose
        std::string about_info = R"(
A game by: 
    Gabe 
    Brett
    Alice)";

        ui_context.get()->push_parent(&root);
        {
            padding(left_padding);
            div(content);

            ui_context.get()->push_parent(&content);
            {
                padding(top_padding);
                text(about_text, about_info);
                if (button(back_button, "Back")) {
                    Menu::get().go_back();
                }
                padding(bottom_padding);
            }
            ui_context.get()->pop_parent();
        }
        ui_context.get()->pop_parent();
        ui_context->end(&root);
        // std::cout << "********************** END FRAME **************** " <<
        // std::endl;
    }

    virtual void onUpdate(float) override {
        if (Menu::get().is_not(Menu::State::About)) return;
        SetExitKey(KEY_NULL);

        // TODO with gamelayer, support events
        if (minimized) {
            return;
        }
    }

    virtual void onDraw(float dt) override {
        if (Menu::get().is_not(Menu::State::About)) return;
        // TODO with gamelayer, support events
        if (minimized) {
            return;
        }

        ClearBackground(ui_context->active_theme().background);
        draw_ui(dt);
    }
};
