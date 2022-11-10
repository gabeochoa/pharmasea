
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

    virtual bool draw_outside_debug_mode() const override {
        return !is_ghost_player;
    }
    virtual bool is_collidable() override { return !is_ghost_player; }

    virtual vec3 update_xaxis_position(float dt) override {
        float left = KeyMap::is_event(Menu::State::Game, "Player Left");
        float right = KeyMap::is_event(Menu::State::Game, "Player Right");
        // TODO do we have to worry about having branches?
        // I feel like sending 4 floats over network probably worse than 4
        // branches on checking float positive but idk
        if (left > 0)
            inputs.push_back({Menu::State::Game, "Player Left", left, dt});
        if (right > 0)
            inputs.push_back({Menu::State::Game, "Player Right", right, dt});
        return this->raw_position;
    }

    virtual vec3 update_zaxis_position(float dt) override {
        float up = KeyMap::is_event(Menu::State::Game, "Player Forward");
        float down = KeyMap::is_event(Menu::State::Game, "Player Back");
        if (up > 0)
            inputs.push_back({Menu::State::Game, "Player Forward", up, dt});
        if (down > 0)
            inputs.push_back({Menu::State::Game, "Player Back", down, dt});
        return this->raw_position;
    }

    virtual vec3 get_position_after_input(
        std::vector<std::shared_ptr<Entity>> entities, UserInputs inpts) {
        for (UserInput& ui : inpts) {
            auto menu_state = std::get<0>(ui);
            if (menu_state != Menu::State::Game) continue;

            std::string input_key_name = std::get<1>(ui);
            float input_amount = std::get<2>(ui);
            float frame_dt = std::get<3>(ui);
            float speed = this->base_speed() * frame_dt;

            auto new_position = this->position;

            if (input_key_name == "Player Left") {
                new_position.x -= input_amount * speed;
            } else if (input_key_name == "Player Right") {
                new_position.x += input_amount * speed;
            } else if (input_key_name == "Player Forward") {
                new_position.z -= input_amount * speed;
            } else if (input_key_name == "Player Back") {
                new_position.z += input_amount * speed;
            }

            auto fd = get_face_direction(new_position, new_position);
            int fd_x = std::get<0>(fd);
            int fd_z = std::get<1>(fd);
            update_facing_direction(fd_x, fd_z);
            handle_collision(entities, fd_x, new_position, fd_z, new_position);
            this->position = this->raw_position;
        }
        return this->position;
    }
};
