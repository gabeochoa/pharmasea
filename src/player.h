
#pragma once

#include "base_player.h"
#include "globals.h"
#include "keymap.h"
#include "raylib.h"
//
#include "furniture.h"

struct Player : public BasePlayer {
    std::string username;
    std::vector<UserInput> inputs;
    bool is_ghost_player = false;

    Player(vec3 p, Color face_color_in, Color base_color_in)
        : BasePlayer(p, face_color_in, base_color_in) {}
    Player(vec2 p, Color face_color_in, Color base_color_in)
        : BasePlayer(p, face_color_in, base_color_in) {}
    Player() : BasePlayer({0, 0, 0}, {0, 255, 0, 255}, {255, 0, 0, 255}) {}
    Player(vec2 location)
        : BasePlayer({location.x, 0, location.y}, {0, 255, 0, 255},
                     {255, 0, 0, 255}) {}

    virtual bool draw_outside_debug_mode() const { return !is_ghost_player; }
    virtual bool is_collidable() { return !is_ghost_player; }

    virtual vec3 update_xaxis_position(float dt) override {
        float left = KeyMap::is_event(Menu::State::Game, "Player Left");
        float right = KeyMap::is_event(Menu::State::Game, "Player Right");
        inputs.push_back({Menu::State::Game, "Player Left", left, dt});
        inputs.push_back({Menu::State::Game, "Player Right", right, dt});
        return this->raw_position;
    }

    virtual vec3 update_zaxis_position(float dt) override {
        float up = KeyMap::is_event(Menu::State::Game, "Player Forward");
        float down = KeyMap::is_event(Menu::State::Game, "Player Back");
        inputs.push_back({Menu::State::Game, "Player Forward", up, dt});
        inputs.push_back({Menu::State::Game, "Player Back", down, dt});
        return this->raw_position;
    }

    virtual vec3 get_position_after_input(UserInputs inpts) {
        for (UserInput& ui : inpts) {
            auto menu_state = std::get<0>(ui);
            if (menu_state != Menu::State::Game) continue;

            std::string input_key_name = std::get<1>(ui);
            float input_amount = std::get<2>(ui);
            float frame_dt = std::get<3>(ui);
            float speed = this->base_speed() * frame_dt;

            if (input_key_name == "Player Left") {
                this->position.x -= input_amount * speed;
            } else if (input_key_name == "Player Right") {
                this->position.x += input_amount * speed;
            } else if (input_key_name == "Player Forward") {
                this->position.z -= input_amount * speed;
            } else if (input_key_name == "Player Back") {
                this->position.z += input_amount * speed;
            }
        }
        return this->position;
    }
};
