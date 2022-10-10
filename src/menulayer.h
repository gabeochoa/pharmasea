#pragma once

#include "camera.h"
#include "external_include.h"
#include "input.h"
#include "layer.h"
#include "menu.h"
#include "raylib.h"

struct MenuLayer : public Layer {
    MenuCam cam;

    MenuLayer() : Layer("Menu") { minimized = false; }
    virtual ~MenuLayer() {}
    virtual void onAttach() override {}
    virtual void onDetach() override {}

    virtual void onEvent(Event& event) override {
        EventDispatcher dispatcher(event);
        dispatcher.dispatch<KeyPressedEvent>(
            std::bind(&MenuLayer::onKeyPressed, this, std::placeholders::_1));
    }

    bool onKeyPressed(KeyPressedEvent& event) {
        if (event.keycode == KEY_ENTER) {
            Menu::get().state = Menu::State::Game;
            return true;
        }
        return false;
    }

    virtual void onUpdate(float) override {
        if (Menu::get().state != Menu::State::Root) return;
        SetExitKey(KEY_ESCAPE);

        // TODO with gamelayer, support events
        if (minimized) {
            return;
        }

        BeginDrawing();
        {
            ClearBackground(RAYWHITE);
            DrawText(Menu::get().tostring(), 19, 20, 20, LIGHTGRAY);
            DrawText("Main Menu", 190, 200, 20, BLACK);
        }
        EndDrawing();
    }
};
