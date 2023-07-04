
#pragma once

#include "../engine.h"
#include "../entity.h"
#include "../external_include.h"
#include "../preload.h"

struct PlayerInfoLayer : public Layer {
    PlayerInfoLayer() : Layer("PlayerInfo") {}

    int y_pos = 0;

    virtual ~PlayerInfoLayer() {}
    virtual void onUpdate(float) override {}

    void draw_text(const std::string& str) {
        float y = 200.f + y_pos;
        DrawTextEx(Preload::get().font, str.c_str(), vec2{5, y}, 20, 0, WHITE);
        y_pos += 15;
    }

    virtual void onDraw(float) override {
        if (!MenuState::s_in_game()) return;
        if (!global_player) return;

        DrawRectangle(5, 200, 175, 75, (Color){50, 50, 50, 200});

        y_pos = 0;
        draw_text("PlayerInfo:");
        draw_text(fmt::format("id: {} position: {}", global_player->id,
                              global_player->get<Transform>().pos()));
        draw_text(fmt::format(
            "holding furniture?: {}",
            global_player->get<CanHoldFurniture>().is_holding_furniture()));
        draw_text(
            fmt::format("holding item?: {}",
                        global_player->get<CanHoldItem>().is_holding_item()));
    }
};
