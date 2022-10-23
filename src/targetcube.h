#pragma once

#include "external_include.h"
//
#include "entityhelper.h"
#include "globals.h"
#include "person.h"

struct TargetCube : public Person {
    TargetCube(raylib::vec3 p, raylib::Color face_color_in, raylib::Color base_color_in)
        : Person(p, face_color_in, base_color_in) {}
    TargetCube(raylib::vec2 p, raylib::Color face_color_in, raylib::Color base_color_in)
        : Person(p, face_color_in, base_color_in) {}
    TargetCube(raylib::vec3 p, raylib::Color c) : Person(p, c) {}
    TargetCube(raylib::vec2 p, raylib::Color c) : Person(p, c) {}

    virtual raylib::vec3 update_xaxis_position(float dt) override {
        float speed = 10.0f * dt;
        auto new_pos_x = this->raw_position;
        bool left = (bool) KeyMap::is_event(Menu::State::Game, "Target Left");
        bool right = (bool) KeyMap::is_event(Menu::State::Game, "Target Right");
        if (left) new_pos_x.x -= speed;
        if (right) new_pos_x.x += speed;
        return new_pos_x;
    }

    virtual raylib::vec3 update_zaxis_position(float dt) override {
        float speed = 10.0f * dt;
        auto new_pos_z = this->raw_position;
        bool up = (bool) KeyMap::is_event(Menu::State::Game, "Target Forward");
        bool down = (bool) KeyMap::is_event(Menu::State::Game, "Target Back");
        if (up) new_pos_z.z -= speed;
        if (down) new_pos_z.z += speed;
        return new_pos_z;
    }

    virtual bool is_collidable() override { return false; }
};
