

#pragma once

#include "entityhelper.h"
#include "external_include.h"
//
#include "aiperson.h"
//
#include "app.h"
#include "camera.h"
#include "furniture/register.h"
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

struct Customer : public AIPerson {
    std::optional<SpeechBubble> bubble;

    std::string name = "Customer";

    Customer(vec3 p, Color face_color_in, Color base_color_in)
        : AIPerson(p, face_color_in, base_color_in) {}
    Customer(vec2 p, Color face_color_in, Color base_color_in)
        : AIPerson(p, face_color_in, base_color_in) {}
    Customer(vec3 p, Color c) : AIPerson(p, c) {}
    Customer(vec2 p, Color c) : AIPerson(p, c) {}

    virtual void reset_to_find_new_target() override {
        if (this->path_length() == 0) {
            Register* reg = get_target_register();
            if (reg != nullptr) {
                bool in_line = reg->is_in_line(std::shared_ptr<AIPerson>(this));
                if (in_line) {
                    return;
                }
            }
            // this->target_entity = nullptr;
            // this->target.reset();
            // this->path.reset();
            // this->local_target.reset();
        }
    }

    virtual void ensure_target() override {
        if (target.has_value()) {
            return;
        }
        std::shared_ptr<Register> closest_target =
            EntityHelper::getClosestMatchingEntity<Register>(
                vec::to2(this->position), TILESIZE * 100.f,
                [](auto&&) { return true; });
        if (!closest_target) return;

        this->target = closest_target->get_next_queue_position(this);
    }

    Register* get_target_register() {
        std::shared_ptr<Register> reg;
        if (this->target.has_value()) {
            reg = EntityHelper::getClosestMatchingEntity<Register>(
                this->target.value(), 1.f, [](auto&&) { return true; });
        }
        return nullptr;
    }

    virtual float base_speed() override { return 2.5f; }

    virtual void update(float dt) override {
        AIPerson::update(dt);

        Register* reg = get_target_register();
        if (reg) {
            this->turn_to_face_entity(reg);
        }

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

        if (this->target.has_value()) {
            const float box_size = TILESIZE / 10.f;
            DrawCube(vec::to3(this->target.value()), box_size, box_size,
                     box_size, BLUE);
        }
    }
};
