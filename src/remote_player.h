

#pragma once

#include "external_include.h"
#include "raylib.h"
//
#include "text_util.h"
#include "util.h"
//
#include "base_player.h"
#include "preload.h"

struct RemotePlayer : public BasePlayer {
    // TODO what is a reasonable default value here?
    // TODO who sets this value?
    int client_id = -1;
    std::string name = "Remote Player";

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

    void update_name(std::string new_name) { name = new_name; }

    virtual void update_remotely(float* location, std::string username,
                                 int facing_direction) {
        this->name = username;
        this->position = vec3{location[0], location[1], location[2]};
        this->face_direction =
            static_cast<FrontFaceDirection>(facing_direction);
    }

    virtual bool is_collidable() override { return false; }

    virtual void render_normal() const override {
        BasePlayer::render_normal();
        // TODO right now cant do wstring //
        DrawFloatingText(this->raw_position, Preload::get().font, name.c_str());
    }
};
