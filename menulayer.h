#pragma once

#include "camera.h"
#include "external_include.h"
#include "layer.h"
#include "menu.h"
#include "raylib.h"
#include "input.h"

struct MenuLayer : public Layer {
    MenuCam cam;

    MenuLayer() : Layer("Menu") { minimized = false; }
    virtual ~MenuLayer() {}
    virtual void onAttach() override {}
    virtual void onDetach() override {}

    virtual void onUpdate(float) override {
        if (Menu::get().state != Menu::State::Root) return;
        SetExitKey(KEY_ESCAPE);
        if (input.eat(KEY_ENTER)) {
            Menu::get().state = Menu::State::Game;
            return;
        }

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
