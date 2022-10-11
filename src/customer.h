

#pragma once

#include "external_include.h"
//
#include "aiperson.h"
//
#include "app.h"
#include "camera.h"
#include "globals.h"
#include "text_util.h"
#include "texture_library.h"

struct SpeechBubble {
    vec3 position;
    std::string icon_tex_name;

    SpeechBubble() { icon_tex_name = "jug"; }

    void update(float, vec3 pos) { this->position = pos; }
    void render() const {
        GameCam cam = GLOBALS.get<GameCam>("game_cam");
        {
            Texture texture = TextureLibrary::get().get("bubble");
            DrawBillboard(cam.camera, texture,
                          vec3{position.x,                      //
                               position.y + (TILESIZE * 1.5f),  //
                               position.z},                     //
                          TILESIZE,                             //
                          WHITE);
        }
    }
};

struct Ailment {
    // Name
    // Treatment
};

struct Customer : public AIPerson {
    std::optional<SpeechBubble> bubble;

    std::string name = "Customer";

    Customer(vec3 p, Color face_color_in, Color base_color_in)
        : AIPerson(p, face_color_in, base_color_in) {}
    Customer(vec2 p, Color face_color_in, Color base_color_in)
        : AIPerson(p, face_color_in, base_color_in) {}
    Customer(vec3 p, Color c) : AIPerson(p, c) {}
    Customer(vec2 p, Color c) : AIPerson(p, c) {}

    virtual void ensure_target() override {
        if (target.has_value()) {
            return;
        }
        this->random_target();
    }
    virtual float base_speed() override { return 5.f; }

    virtual void update(float dt) override {
        AIPerson::update(dt);
        // TODO just for debug purposes
        if (!bubble.has_value()) {
            bubble = SpeechBubble();
        } else {
            bubble.value().update(dt, this->raw_position);
        }
    }

    void render_name() const {
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
            -1.05 * TILESIZE   // this is Y
        );

        DrawText3D(           //
            App::get().font,  //
            name.c_str(),     //
            {0.f},            //
            96,               // font size
            4,                // font spacing
            4,                // line spacing
            true,             // backface
            BLACK);

        rlPopMatrix();
    }

    void render_speech_bubble() const {
        if (!this->bubble.has_value()) return;
        this->bubble.value().render();
    }

    virtual void render() const override {
        AIPerson::render();
        this->render_speech_bubble();
        this->render_name();
    }
};
