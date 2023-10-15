#pragma once

#include "../drawing_util.h"
#include "../engine/event.h"
#include "../external_include.h"
//
#include "../engine.h"
#include "../globals.h"
#include "../map.h"
#include "../preload.h"
//
#include "../camera.h"
#include "../components/can_hold_furniture.h"
#include "../components/can_hold_item.h"
#include "../components/collects_user_input.h"
#include "../components/has_timer.h"
#include "../components/is_progression_manager.h"
#include "../components/is_spawner.h"
#include "../engine/layer.h"
#include "../engine/statemanager.h"
#include "../entity_helper.h"
#include "raylib.h"

struct GameDebugLayer : public Layer {
    std::shared_ptr<ui::UIContext> ui_context;

    GameDebugLayer() : Layer(strings::menu::GAME) {
        ui_context = std::make_shared<ui::UIContext>();
    }
    virtual ~GameDebugLayer() {}

    virtual void onUpdate(float) override {}
    virtual void onDraw(float dt) override {
        if (!MenuState::s_in_game()) return;
        if (GameState::get().is(game::State::Paused)) return;

        bool debug_ui = GLOBALS.get<bool>("debug_ui_enabled");
        if (!debug_ui) return;
        draw_debug_ui(dt);
    }

    void draw_debug_ui(float dt) {
        auto map_ptr = GLOBALS.get_ptr<Map>(strings::globals::MAP);
        using namespace ui;
        begin(ui_context, dt);

        auto window = Rectangle{0, 0, WIN_WF(), WIN_HF()};
        auto [l_col, _] = rect::vsplit(window, 20);

        auto [_ping_game_version, menu_state, player_info, round_info,
              _bottom_padding] = rect::hsplit<5>(l_col, 20);

        {
            auto [menu_div, game_div] = rect::hsplit<2>(menu_state, 10);
            text(Widget{menu_div}, std::string(MenuState::get().tostring()));
            text(Widget{game_div}, std::string(GameState::get().tostring()));
        }

        // Player Info
        {
            if (map_ptr) {
                OptEntity player = map_ptr->get_remote_with_cui();
                if (player) {
                    EntityID furniture_id =
                        player->get<CanHoldFurniture>().is_holding_furniture()
                            ? player->get<CanHoldFurniture>().furniture_id()
                            : -1;
                    OptEntity furniture =
                        EntityHelper::getEntityForID(furniture_id);

                    auto [id_div, position_div, holding_div, item_div] =
                        rect::hsplit<4>(player_info, 10);

                    text(Widget{id_div}, fmt::format("{}", (player->id)));
                    text(Widget{position_div},
                         fmt::format("{}", (player->get<Transform>().pos())));
                    text(Widget{holding_div},
                         fmt::format("holding furniture?: {}",
                                     furniture
                                         ? furniture->get<DebugName>().name()
                                         : "Empty"));
                    text(
                        Widget{item_div},
                        fmt::format("holding item?: {}",
                                    player->get<CanHoldItem>().is_holding_item()
                                        ? player->get<CanHoldItem>()
                                              .item()
                                              ->get<DebugName>()
                                              .name()
                                        : "Empty"));
                } else {
                    text(Widget{player_info}, "No matching player found");
                }
            } else {
                text(Widget{player_info}, "Map not valid");
            }
        }

        // Round Info
        if (GameState::get().is(game::State::InRound)) {
            if (map_ptr) {
                OptEntity sophie =
                    EntityHelper::getFirstOfType(EntityType::Sophie);
                const HasTimer& hasTimer = sophie->get<HasTimer>();

                auto [round_time_div, round_spawn_div, drinks_div,
                      ingredients_div] = rect::hsplit<4>(round_info, 10);

                text(Widget{round_time_div},
                     fmt::format("Round Length: {:.0f}/{}",
                                 hasTimer.currentRoundTime,
                                 hasTimer.totalRoundTime));

                {
                    auto [num_spawned, countdown] =
                        rect::hsplit<2>(round_spawn_div);

                    auto spawners = EntityHelper::getAllWithType(
                        EntityType::CustomerSpawner);

                    Entity& spawner = spawners[0];
                    const IsSpawner& iss = spawner.get<IsSpawner>();
                    text(Widget{num_spawned},
                         fmt::format("Num Spawned: {} / {}",
                                     iss.get_num_spawned(),
                                     iss.get_max_spawned()));
                }
            } else {
                text(Widget{round_info}, "Map not valid");
            }
        }

        end();
    }
};
