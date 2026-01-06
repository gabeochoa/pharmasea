
#include "seedmanagerlayer.h"

#include "../dataclass/names.h"
#include "../engine/runtime_globals.h"
#include "../network/network.h"

bool SeedManagerLayer::is_user_host() {
    if (network_info) {
        return network_info->is_host();
    }
    return false;
}

bool SeedManagerLayer::onCharPressedEvent(CharPressedEvent& event) {
    if (GameState::get().is(game::State::Paused)) return false;
    if (MenuState::get().is_not(menu::State::Game)) return false;
    if (!is_user_host()) return false;
    if (!map_ptr) return false;

    return ui_context.get()->process_char_press_event(event);
}

bool SeedManagerLayer::onKeyPressed(KeyPressedEvent& event) {
    if (GameState::get().is(game::State::Paused)) return false;
    if (GameState::get().is_not(game::State::Lobby)) return false;
    if (MenuState::get().is_not(menu::State::Game)) return false;
    if (!is_user_host()) return false;
    if (!map_ptr) return false;
    // We need this check to guarantee that you are near the box
    // otherwise pressing space will freeze you until you hit esc
    if (!map_ptr->showMinimap) return false;

    if (map_ptr->showSeedInputBox &&
        KeyMap::get_key_code(menu::State::Game, InputName::Pause) ==
            event.keycode) {
        map_ptr->showSeedInputBox = false;
        return true;
    }

    if (!map_ptr->showSeedInputBox &&
        KeyMap::get_key_code(menu::State::Game, InputName::PlayerPickup) ==
            event.keycode) {
        map_ptr->showSeedInputBox = true;
        return true;
    }

    return ui_context.get()->process_keyevent(event);
}

void SeedManagerLayer::onUpdate(float) {
    map_ptr = globals::world_map();
    if (!map_ptr) return;

    if (!map_ptr->showSeedInputBox) {
        tempSeed = map_ptr->seed;
    }
}

void SeedManagerLayer::draw_seed_input(float dt) {
    using namespace ui;
    begin(ui_context, dt);

    auto screen = Rectangle{0, 0, WIN_WF(), WIN_HF()};

    auto [_t, middle, _b] = rect::hsplit<3>(screen);
    auto [_l, popup, _r] = rect::vsplit<3>(middle);

    if (auto windowresult = window(Widget{popup, -10}); windowresult) {
        int z_index = windowresult.as<int>();
        auto [label, tf, buttons] = rect::hsplit<3>(popup, 20);

        text(Widget{label, windowresult.as<int>()},
             TranslatableString("Enter Seed:"));

        if (auto result = textfield(
                Widget{tf, z_index},
                TextfieldData{tempSeed,
                              [](const std::string& content) {
                                  // TODO add max seed length
                                  if (content.size() >=
                                      network::MAX_NAME_LENGTH)
                                      return TextfieldValidationDecisionFlag::
                                          StopNewInput;
                                  return TextfieldValidationDecisionFlag::Valid;
                              }});
            result) {
            tempSeed = (result.as<std::string>());
        }

        auto [_b1, randomize, _b2, select, _b3] = rect::vsplit<5>(buttons, 20);

        if (button(Widget{randomize, z_index}, TranslatableString("Randomize"),
                   true)) {
            const auto name = get_random_name_rot13();
            network_info->send_updated_seed(name);
            map_ptr->showSeedInputBox = false;
        }

        if (button(Widget{select, z_index}, TranslatableString("Save Seed"),
                   true)) {
            network_info->send_updated_seed(tempSeed);
            map_ptr->showSeedInputBox = false;
        }
    }

    end();
}

void SeedManagerLayer::draw_minimap(float dt) {
    raylib::BeginMode3D((*cam).get());
    {
        raylib::rlTranslatef(-5, 0, 7.f);
        float scale = 0.10f;
        raylib::rlScalef(scale, scale, scale);
        raylib::DrawPlane((vec3){0.0f, -TILESIZE, 0.0f}, (vec2){40.0f, 40.0f},
                          DARKGRAY);
        map_ptr->onDraw(dt);
    }
    raylib::EndMode3D();
}

void SeedManagerLayer::onDraw(float dt) {
    TRACY_ZONE_SCOPED;
    if (!MenuState::s_in_game()) return;
    if (GameState::get().is(game::State::Paused)) return;
    if (!map_ptr) return;

    // Only show minimap during lobby
    if (!map_ptr->showMinimap) return;

    draw_minimap(dt);

    if (is_user_host() && map_ptr->showSeedInputBox) draw_seed_input(dt);
}
