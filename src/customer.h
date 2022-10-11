

#pragma once

#include "external_include.h"
//
#include "aiperson.h"
//
#include "app.h"
#include "globals.h"
#include "text_util.h"

struct SpeechBubble {
    // vec<Icon>
    // iconIndex

    void render() const {
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
        // this->random_target();
    }

    virtual void update(float dt) override { AIPerson::update(dt); }

    void render_name() const {
        rlPushMatrix();
        rlTranslatef(          //
            this->position.x,  //
            0.f,               //
            this->position.z   //
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
        if(this->bubble.has_value()) return;
        rlPushMatrix();
        rlTranslatef(          //
            this->position.x,  //
            0.f,               //
            this->position.z   //
        );
        rlRotatef(90.0f, 1.0f, 0.0f, 0.0f);

        rlTranslatef(          //
            -0.5f * TILESIZE,  //
            0.f,               //
            -1.05 * TILESIZE   // this is Y
        );

        this->bubble.value().render();

        rlPopMatrix();
    }


    virtual void render() const override {
        AIPerson::render();
        this->render_name();
    }
};
