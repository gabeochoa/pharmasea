

#pragma once

#include "components/can_hold_item.h"
#include "components/can_perform_job.h"
#include "drawing_util.h"
#include "entityhelper.h"
#include "external_include.h"
//
#include "aiperson.h"
//
#include "ailment.h"
#include "camera.h"
#include "engine/texture_library.h"
#include "furniture/register.h"
#include "globals.h"
#include "job.h"
#include "names.h"
#include "text_util.h"

struct SpeechBubble {
    vec3 position;
    // TODO we arent using any string functionality can we swap to const char*?
    // perhaps in a typedef?
    std::string icon_tex_name;

    explicit SpeechBubble(std::string icon) : icon_tex_name(icon) {}

    void update(float, vec3 pos) { this->position = pos; }
    void render() const {
        GameCam cam = GLOBALS.get<GameCam>("game_cam");
        raylib::Texture texture = TextureLibrary::get().get(icon_tex_name);
        raylib::DrawBillboard(cam.camera, texture,
                              vec3{position.x,                      //
                                   position.y + (TILESIZE * 1.5f),  //
                                   position.z},                     //
                              TILESIZE,                             //
                              raylib::WHITE);
    }
};

struct Customer : public AIPerson {
    std::optional<SpeechBubble> bubble;
    std::shared_ptr<Ailment> ailment;

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        // Only things that need to be rendered, need to be serialized :)
        s.ext(*this, bitsery::ext::BaseClass<AIPerson>{});
    }

   public:
    Customer() : AIPerson() { init(); }
    Customer(vec3 p, Color face_color_in, Color base_color_in)
        : AIPerson(p, face_color_in, base_color_in) {
        init();
    }
    Customer(vec2 p, Color face_color_in, Color base_color_in)
        : AIPerson(p, face_color_in, base_color_in) {
        init();
    }
    Customer(vec3 p, Color c) : AIPerson(p, c) { init(); }
    Customer(vec2 p, Color c) : AIPerson(p, c) { init(); }

    void init() {
        get<HasName>().update(get_random_name());
        get<CanPerformJob>().update(WaitInQueue, Wandering);

        // TODO turn back on ailments
        // ailment.reset(new Insomnia());
        // bubble = SpeechBubble(ailment->icon_name());
    }

    virtual float base_speed() override {
        float base_speed = 4.f;
        if (ailment) base_speed *= ailment->speed_multiplier();
        return base_speed;
    }

    virtual float stagger_mult() override {
        return ailment ? ailment->stagger() : 0.f;
    }

    // virtual void in_round_update(float dt) override {
    // AIPerson::in_round_update(dt);

    // Register* reg = get_target_register();
    // if (reg) {
    // this->turn_to_face_entity(reg);
    // }
    //
    // if (bubble.has_value())
    // bubble.value().update(dt, this->get<Transform>().raw_position);
    // }

    // virtual void render_normal() const override {
    // auto render_speech_bubble = [&]() {
    // if (!this->bubble.has_value()) return;
    // this->bubble.value().render();
    // };
    // AIPerson::render_normal();
    // render_speech_bubble();
    // }
};
