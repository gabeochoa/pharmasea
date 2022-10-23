

#pragma once

#include "base_player.h"
#include "globals.h"
#include "keymap.h"
#include "raylib.h"

struct RemotePlayer : public BasePlayer {
    RemotePlayer(vec3 p, Color face_color_in, Color base_color_in)
        : BasePlayer(p, face_color_in, base_color_in) {}
    RemotePlayer(vec2 p, Color face_color_in, Color base_color_in)
        : BasePlayer(p, face_color_in, base_color_in) {}
    RemotePlayer()
        : BasePlayer({0, 0, 0}, {0, 255, 0, 255}, {255, 0, 0, 255}) {}
    RemotePlayer(vec2 location)
        : BasePlayer({location.x, 0, location.y}, {0, 255, 0, 255},
                     {255, 0, 0, 255}) {}

    virtual vec3 update_xaxis_position(float) override {
        return this->position;
    }

    virtual vec3 update_zaxis_position(float) override {
        return this->position;
    }

    virtual void update_remotely(float* location, int facing_direction) {
        this->position = vec3{location[0], location[1], location[2]};
        this->face_direction =
            static_cast<FrontFaceDirection>(facing_direction);
    }

    virtual bool is_collidable() override { return false; }
};
