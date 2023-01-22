

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

    RemotePlayer(vec3 p, Color face_color_in, Color base_color_in)
        : BasePlayer(p, face_color_in, base_color_in) {}
    RemotePlayer(vec2 p, Color face_color_in, Color base_color_in)
        : BasePlayer(p, face_color_in, base_color_in) {}
    RemotePlayer() : BasePlayer({0, 0, 0}, WHITE, WHITE) {}
    RemotePlayer(vec2 location)
        : BasePlayer({location.x, 0, location.y}, {0, 255, 0, 255},
                     {255, 0, 0, 255}) {}

    virtual void update_remotely(float* location, std::string username,
                                 int facing_direction) {
        HasName& hasname = this->get<HasName>();
        hasname.name = username;

        this->get<HasName>().name = username;
        Transform& transform = this->get<Transform>();
        // TODO add setters
        transform.position = vec3{location[0], location[1], location[2]};
        transform.face_direction =
            static_cast<Transform::FrontFaceDirection>(facing_direction);
    }

    virtual bool is_collidable() override { return false; }
};
