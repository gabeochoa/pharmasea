
#pragma once

#include "ailment.h"
#include "camera.h"
#include "components/can_have_ailment.h"
#include "components/can_hold_item.h"
#include "components/can_perform_job.h"
#include "components/has_base_speed.h"
#include "components/is_snappable.h"
#include "drawing_util.h"
#include "engine/astar.h"
#include "engine/globals_register.h"
#include "engine/is_server.h"
#include "engine/log.h"
#include "engine/sound_library.h"
#include "engine/texture_library.h"
#include "entityhelper.h"
#include "external_include.h"
#include "globals.h"
#include "job.h"
#include "names.h"
#include "person.h"
#include "text_util.h"

struct AIPerson : public Person {
   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<Person>{});
        // Only things that need to be rendered, need to be serialized :)
    }

    void add_static_components() {
        addComponent<CanPerformJob>().update(Wandering, Wandering);
        addComponent<HasBaseSpeed>().update(10.f);
    }

   public:
    AIPerson() : AIPerson({0, 0, 0}) {}
    AIPerson(vec3 p) : Person(p) { add_static_components(); }
};

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

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        // Only things that need to be rendered, need to be serialized :)
        s.ext(*this, bitsery::ext::BaseClass<AIPerson>{});
    }

   public:
    Customer() : Customer({0, 0, 0}) { init(); }
    Customer(vec3 p) : AIPerson(p) { init(); }

    void init() {
        addComponent<CanHaveAilment>().update(std::make_shared<Insomnia>());

        get<HasName>().update(get_random_name());
        get<CanPerformJob>().update(WaitInQueue, Wandering);

        // TODO turn back on bubbles
        // bubble = SpeechBubble(ailment->icon_name());
    }

    // TODO support this
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
