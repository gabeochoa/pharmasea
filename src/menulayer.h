#pragma once

#include "external_include.h"
#include "input.h"
#include "layer.h"
#include "menu.h"
#include "raylib.h"
#include "settings.h"
#include "ui.h"
#include "ui_theme.h"
#include "ui_widget.h"
#include "uuid.h"

struct MenuLayer : public Layer {
    std::shared_ptr<ui::UIContext> ui_context;

    MenuLayer() : Layer("Menu") {
        minimized = false;

        ui_context.reset(new ui::UIContext());
        ui_context->init();
        ui_context->set_font(App::get().font);
        // TODO we should probably enforce that you cant do this
        // and we should have ->set_base_theme()
        // and push_theme separately, if you end() with any stack not empty...
        // thats a flag
        ui_context->push_theme(ui::DEFAULT_THEME);
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

        // TODO replace with expectation that these use the text size
        const SizeExpectation button_x = {.mode = Pixels, .value = 120.f};
        const SizeExpectation button_y = {.mode = Pixels, .value = 50.f};

        const SizeExpectation padd_x = {
            .mode = Pixels, .value = 120.f, .strictness = 0.9f};
        const SizeExpectation padd_y = {
            .mode = Pixels, .value = 25.f, .strictness = 0.5f};

        ui_context->begin(mouseDown, mousepos);

        ui::Widget root(
            {.mode = ui::SizeMode::Pixels, .value = WIN_W, .strictness = 1.f},
            {.mode = ui::SizeMode::Pixels, .value = WIN_H, .strictness = 1.f});
        root.growflags = ui::GrowFlags::Row;

        Widget left_padding(
            {.mode = Pixels, .value = 100.f, .strictness = 1.f},
            {.mode = Pixels, .value = WIN_H, .strictness = 1.f});

        Widget content({.mode = Children},
                       {.mode = Percent, .value = 1.f, .strictness = 1.0f});
        content.growflags = ui::GrowFlags::Column;

        Widget play_button(MK_UUID(id, ROOT_ID), button_x, button_y);
        Widget button_padding(padd_x, padd_y);
        Widget about_button(MK_UUID(id, ROOT_ID), button_x, button_y);
        Widget settings_button(MK_UUID(id, ROOT_ID), button_x, button_y);

        ui_context->push_parent(&root);
        {
            padding(left_padding);
            div(content);

            ui_context->push_parent(&content);
            {
                padding(button_padding);
                if (button_with_label(play_button, "Play")) {
                    Menu::get().state = Menu::State::Game;
                }
                padding(button_padding);
                if (button_with_label(about_button, "About")) {
                    Menu::get().state = Menu::State::About;
                }
                padding(button_padding);
                if (button_with_label(settings_button, "Settings")) {
                    Menu::get().state = Menu::State::Settings;
                }
            }
            ui_context->pop_parent();
        }
        ui_context->pop_parent();
        ui_context->end(&root);
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
        ClearBackground(ui_context->active_theme().background);
        draw_ui();
    }
};
