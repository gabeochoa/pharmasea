
#pragma once

#include "base_player.h"
#include "globals.h"
#include "keymap.h"
#include "raylib.h"
//
#include "furniture.h"

struct Player : public BasePlayer {
    Player(vec3 p, Color face_color_in, Color base_color_in)
        : BasePlayer(p, face_color_in, base_color_in) {}
    Player(vec2 p, Color face_color_in, Color base_color_in)
        : BasePlayer(p, face_color_in, base_color_in) {}
    Player() : BasePlayer({0, 0, 0}, {0, 255, 0, 255}, {255, 0, 0, 255}) {}
    explicit Player(vec2 location)
        : BasePlayer({location.x, 0, location.y}, {0, 255, 0, 255},
                     {255, 0, 0, 255}) {}

    virtual vec3 update_xaxis_position(float dt) override {
        float speed = this->base_speed() * dt;
        auto new_pos_x = this->raw_position;
        float left = KeyMap::is_event(Menu::State::Game, "Player Left");
        float right = KeyMap::is_event(Menu::State::Game, "Player Right");
        new_pos_x.x -= left * speed;
        new_pos_x.x += right * speed;
        return new_pos_x;
    }

    virtual vec3 update_zaxis_position(float dt) override {
        float speed = this->base_speed() * dt;
        auto new_pos_z = this->raw_position;
        float up = KeyMap::is_event(Menu::State::Game, "Player Forward");
        float down = KeyMap::is_event(Menu::State::Game, "Player Back");
        new_pos_z.z -= up * speed;
        new_pos_z.z += down * speed;
        return new_pos_z;
    }
};
