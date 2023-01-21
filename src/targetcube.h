#pragma once

#include "external_include.h"
//
#include "entityhelper.h"
#include "globals.h"
#include "person.h"
#include "statemanager.h"

struct TargetCube : public Person {
    TargetCube(vec3 p, Color face_color_in, Color base_color_in)
        : Person(p, face_color_in, base_color_in) {}
    TargetCube(vec2 p, Color face_color_in, Color base_color_in)
        : Person(p, face_color_in, base_color_in) {}
    TargetCube(vec3 p, Color c) : Person(p, c) {}
    TargetCube(vec2 p, Color c) : Person(p, c) {}

    virtual vec3 update_xaxis_position(float dt) override {
        float speed = 10.0f * dt;
        auto new_pos_x = this->get<Transform>().raw_position;
        bool left =
            (bool) KeyMap::is_event(menu::State::Game, InputName::TargetLeft);
        bool right =
            (bool) KeyMap::is_event(menu::State::Game, InputName::TargetRight);
        if (left) new_pos_x.x -= speed;
        if (right) new_pos_x.x += speed;
        return new_pos_x;
    }

    virtual vec3 update_zaxis_position(float dt) override {
        float speed = 10.0f * dt;
        auto new_pos_z = this->get<Transform>().raw_position;
        bool up = (bool) KeyMap::is_event(menu::State::Game,
                                          InputName::TargetForward);
        bool down =
            (bool) KeyMap::is_event(menu::State::Game, InputName::TargetBack);
        if (up) new_pos_z.z -= speed;
        if (down) new_pos_z.z += speed;
        return new_pos_z;
    }

    virtual bool is_collidable() override { return false; }
};
