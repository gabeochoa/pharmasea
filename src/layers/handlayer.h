
#pragma once

#include "../engine/layer.h"
#include "../engine/statemanager.h"
#include "../engine/texture_library.h"

struct HandLayer : public Layer {
    raylib::Texture texture;
    HandLayer() : Layer("hand_layer") {
        texture = TextureLibrary::get().get("hand");
        raylib::DisableCursor();
    }
    virtual ~HandLayer() {}

    virtual void onUpdate(float) override {}

    virtual void onDraw(float) override {
        if (MenuState::s_in_game() && !GameState::s_is_paused()) return;

        vec2 mouse_pos = ext::get_mouse_position();
        vec2 pos = {mouse_pos.x - 120.f, mouse_pos.y - 20.f};
        float scale = 0.5f;
        raylib::DrawTextureEx(texture, pos, 0.f, scale, WHITE);
    }
};
