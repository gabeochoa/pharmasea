

#pragma once

#include "external_include.h"
#include "raylib.h"
#include "util.h"
//
#include "base_player.h"
#include "preload.h"

struct RemotePlayer : public BasePlayer {
    int client_id;
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

    virtual vec3 get_position_after_input(UserInputs inputs) {
        for (UserInput& ui : inputs) {
            auto menu_state = std::get<0>(ui);
            if (menu_state != Menu::State::Game) continue;

            std::cout << "userintpu" << std::endl;

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

    virtual void update_remotely(std::string my_name, float* location,
                                 int facing_direction) {
        this->name = my_name;
        this->position = vec3{location[0], location[1], location[2]};
        this->face_direction =
            static_cast<FrontFaceDirection>(facing_direction);
    }

    virtual bool is_collidable() override { return false; }

    virtual void render() const override {
        auto render_name = [&]() {
            rlPushMatrix();
            rlTranslatef(              //
                this->raw_position.x,  //
                0.f,                   //
                this->raw_position.z   //
            );
            rlRotatef(90.0f, 1.0f, 0.0f, 0.0f);

            rlTranslatef(          //
                -0.5f * TILESIZE,  //
                0.f,               //
                -1.05f * TILESIZE  // this is Y
            );

            DrawText3D(               //
                Preload::get().font,  //
                // TODO right now cant do wstring //
                name.c_str(),  //
                {0.f},         //
                96,            // font size
                4,             // font spacing
                4,             // line spacing
                true,          // backface
                BLACK);

            rlPopMatrix();
        };

        BasePlayer::render();
        render_name();
    }
};
