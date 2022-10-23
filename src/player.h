
#pragma once

#include "base_player.h"
#include "globals.h"
#include "keymap.h"
#include "raylib.h"
//
#include "furniture.h"

struct Player : public BasePlayer {

    Player(raylib::vec3 p, raylib::Color face_color_in, raylib::Color base_color_in)
        : BasePlayer(p, face_color_in, base_color_in) {}
    Player(raylib::vec2 p, raylib::Color face_color_in, raylib::Color base_color_in)
        : BasePlayer(p, face_color_in, base_color_in) {}
    Player() : BasePlayer({0, 0, 0}, {0, 255, 0, 255}, {255, 0, 0, 255}) {}
    Player(raylib::vec2 location)
        : BasePlayer({location.x, 0, location.y}, {0, 255, 0, 255},
                     {255, 0, 0, 255}) {}

    virtual raylib::vec3 update_xaxis_position(float dt) override {
        float speed = this->base_speed() * dt;
        auto new_pos_x = this->raw_position;
        float left = KeyMap::is_event(Menu::State::Game, "Player Left");
        float right = KeyMap::is_event(Menu::State::Game, "Player Right");
        new_pos_x.x -= left * speed;
        new_pos_x.x += right * speed;
        return new_pos_x;
    }

    virtual raylib::vec3 update_zaxis_position(float dt) override {
        float speed = this->base_speed() * dt;
        auto new_pos_z = this->raw_position;
        float up = KeyMap::is_event(Menu::State::Game, "Player Forward");
        float down = KeyMap::is_event(Menu::State::Game, "Player Back");
        new_pos_z.z -= up * speed;
        new_pos_z.z += down * speed;
        return new_pos_z;
    }
};
