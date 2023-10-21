
#pragma once

#include "../camera.h"
#include "../components/collects_user_input.h"
#include "../components/has_timer.h"
#include "../components/transform.h"
#include "../entity.h"
#include "../entity_helper.h"
#include "../map.h"
#include "base_game_renderer.h"
#include "raylib.h"

struct RoundEndReasonLayer : public BaseGameRendererLayer {
    raylib::RenderTexture2D tex;
    RoundEndReasonLayer()
        : BaseGameRendererLayer("RoundEndReason"),
          tex(raylib::LoadRenderTexture(WIN_W(), WIN_H())) {}

    virtual ~RoundEndReasonLayer() { raylib::UnloadRenderTexture(tex); }

    OptEntity get_timer_entity() {
        return EntityHelper::getFirstWithComponent<HasTimer>();
    }

    OptEntity get_player_entity() {
        // TODO why doesnt this work?
        // return EntityHelper::getFirstWithComponent<CollectsUserInput>();
        auto map_ptr = GLOBALS.get_ptr<Map>(strings::globals::MAP);
        if (!map_ptr) return {};
        return map_ptr->get_remote_with_cui();
    }

    virtual bool shouldSkipRender() override { return !shouldRender(); }

    bool shouldRender() {
        if (!GameState::get().should_render_timer()) return false;

        auto entity = get_timer_entity();
        if (!entity) return false;
        const HasTimer& ht = entity->get<HasTimer>();
        if (ht.type != HasTimer::Renderer::Round) return false;

        const auto debug_mode_on =
            GLOBALS.get_or_default<bool>("debug_ui_enabled", false);
        if (debug_mode_on) return true;

        if (ht.currentRoundTime <= 0) return true;

        if (GameState::get().is(game::State::Planning) &&
            ht.block_state_change_reasons.any()) {
            return true;
        }
        return false;
    }

    virtual void onDrawUI(float) override {
        using namespace ui;

        auto window = Rectangle{0, 0, WIN_WF(), WIN_HF()};
        auto content = rect::tpad(window, 5);
        content = rect::lpad(content, 20);
        content = rect::rpad(content, 75);
        content = rect::bpad(content, 80);

        auto entity = get_timer_entity();
        if (!entity) return;
        const HasTimer& ht = entity->get<HasTimer>();
        if (ht.type != HasTimer::Renderer::Round) return;

        auto reason_spots =
            rect::hsplit<HasTimer::WaitingReason::WaitingReasonLast>(content,
                                                                     10);
        // Just make it so that only one reason will show at a time
        // this might be confusing but it avoids the issue where its covering
        // way too much of the screen
        const int max_reason_index =
            0;  // this should never be more than reason_spots.size()

        if (ht.block_state_change_reasons.none()) {
            text(Widget{reason_spots[0]},
                 fmt::format(
                     "{}: {}", text_lookup(strings::i18n::NEXT_ROUND_COUNTDOWN),
                     (int) ceil(util::trunc(ht.roundSwitchCountdown, 1))),
                 theme::Usage::Font, true);
            return;
        }

        std::vector<std::string> active_reasons;
        std::vector<std::optional<vec2>> active_locations;
        // TODO Why does this start a 1?
        for (int i = 1; i < HasTimer::WaitingReason::WaitingReasonLast; i++) {
            if (ht.read_reason(i)) {
                active_reasons.push_back(ht.text_reason(i));
                active_locations.push_back(ht.reason_location(i));
            }
        }

        int i = 0;
        for (auto txt : active_reasons) {
            if (i > max_reason_index) continue;

            text(Widget{reason_spots[i]}, active_reasons[i], theme::Usage::Font,
                 true);

            // TODO eventually i would like this to be a 3d arrow
            // that rotates to the right place
            // but for now lets just draw text
            OptEntity player = get_player_entity();
            std::optional<vec2> possible_location = active_locations[i];

            if (player && possible_location.has_value()) {
                vec2 player_location = player->get<Transform>().as2();
                vec2 location = vec2{possible_location.value().x,
                                     possible_location.value().y};

                vec2 diff{fabs(player_location.x - location.x),
                          fabs(player_location.y - location.y)};

                const auto location_text = fmt::format("{}", vec::snap(diff));

                reason_spots[i].x += reason_spots[i].width;
                reason_spots[i].width = 100.f;
                auto cam = GLOBALS.get_ptr<GameCam>("game_cam");
                if (cam) {
                    inline_3d_model(Widget{reason_spots[i]}, tex, cam->camera,
                                    ModelInfoLibrary::get().get("arrow"));
                }
            }

            i++;
        }
    }
};
