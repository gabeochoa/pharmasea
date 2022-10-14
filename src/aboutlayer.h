
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
        ui_context.get()->set_font(App::get().font);
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
        SetExitKey(KEY_ESCAPE);
        using namespace ui;

        // TODO move to input
        bool mouseDown = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
        vec2 mousepos = GetMousePosition();

        ui_context->begin(mouseDown, mousepos);
        ui_context->push_theme(DEFAULT_THEME);

        ui::Widget root;
        root.set_expectation(
            {.mode = ui::SizeMode::Pixels, .value = WIN_W, .strictness = 1.f},
            {.mode = ui::SizeMode::Pixels, .value = WIN_H, .strictness = 1.f});
        root.growflags = ui::GrowFlags::Row;

        Widget left_padding(
            {.mode = Pixels, .value = 100.f, .strictness = 0.9},
            {.mode = Pixels, .value = WIN_H, .strictness = 1.f});

        Widget content({.mode = Children},
                       {.mode = Percent, .value = 1.f, .strictness = 1.0f});
        content.growflags = ui::GrowFlags::Column;

        Widget about_text({.mode = Pixels, .value = 120.f},
                          {.mode = Pixels, .value = 400.f});

        Widget back_button({.mode = Pixels, .value = 120.f},
                           {.mode = Pixels, .value = 50.f});

        Widget button_padding(
            {.mode = Pixels, .value = 120.f, .strictness = 0.9},
            {.mode = Pixels, .value = 25.f, .strictness = 1.f});

        Widget back_button2({.mode = Pixels, .value = 120.f},
                            {.mode = Pixels, .value = 50.f});

        // NOTE: this is not aligned on purpose
        std::string about_info = R"(
PharmaSea

A game by: 
    Gabe 
    Brett
    Alice
        )";

        ui_context.get()->push_parent(&root);
        {
            padding(left_padding);
            div(content);

            ui_context.get()->push_parent(&content);
            {
                text(about_text, about_info);
                button_with_label(back_button, "Back");
                // padding(button_padding);
                button_with_label(back_button2, "Back");
            }
            ui_context.get()->pop_parent();
        }
        ui_context.get()->pop_parent();

        // after all children are done, process
        std::cout << "process widget" << std::endl;
        autolayout::process_widget(&root);


        std::cout << "render widget" << std::endl;
        ui_context.get()->render_all();

        root.print_tree();

        std::cout << "********************** END FRAME **************** " << std::endl;

        /*
        text_old(MK_UUID(id, ROOT_ID), WidgetConfig({
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

        */

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

        ClearBackground(Color{30, 30, 30, 255});
        draw_ui();
    }
};
