
#pragma once

#include "external_include.h"
//
#include "entity.h"
#include "entityhelper.h"
//
#include "components/has_base_speed.h"
#include "engine/keymap.h"
#include "external_include.h"
//
#include "engine/astar.h"
#include "engine/globals_register.h"
#include "engine/is_server.h"
#include "engine/log.h"
#include "engine/sound_library.h"
//
#include "ailment.h"
#include "camera.h"
#include "components/can_have_ailment.h"
#include "components/can_hold_item.h"
#include "components/can_perform_job.h"
#include "components/has_base_speed.h"
#include "components/is_snappable.h"
#include "entityhelper.h"
#include "job.h"
#include "names.h"

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

struct Person : public Entity {
   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<Entity>{});
    }

   protected:
    Person() : Entity() {}
    Person(vec3 p, Color face_color_in, Color base_color_in)
        : Entity(p, face_color_in, base_color_in) {}
    Person(vec2 p, Color face_color_in, Color base_color_in)
        : Entity(p, face_color_in, base_color_in) {}
    Person(vec3 p, Color c) : Entity(p, c) {}
    Person(vec2 p, Color c) : Entity(p, c) {}

    virtual vec3 size() const override {
        const float sz = TILESIZE * 0.75f;
        return (vec3){sz, sz, sz};
    }

   public:
    static Person* create_person(vec3 pos, Color face_color, Color base_color) {
        // TODO move into component
        // s.value4b(model_index);
        std::array<std::string, 3> character_models = {
            "character_duck",
            "character_dog",
            "character_bear",
        };
        int model_index = 0;
        // void select_next_character_model() {
        // model_index = (model_index + 1) % character_models.size();
        // }

        Person* person = new Person(pos, face_color, base_color);

        person->addComponent<HasBaseSpeed>().update(10.f);
        // log_info("model index: {}", model_index);
        // TODO add a component for this
        person->get<ModelRenderer>().update(ModelInfo{
            // TODO fix this
            .model_name = character_models[model_index],
            .size_scale = 1.5f,
            .position_offset = vec3{0, 0, 0},
            .rotation_angle = 180,
        });

        return person;
    }

    static Person* create_aiperson(vec3 pos, Color face_color,
                                   Color base_color) {
        Person* aiperson = new Person(pos, face_color, base_color);

        aiperson->addComponent<CanPerformJob>().update(Wandering, Wandering);
        aiperson->addComponent<HasBaseSpeed>().update(10.f);

        return aiperson;
    }

    static Person* create_customer(vec2 pos, Color base_color) {
        Person* customer = new Person(pos, base_color);

        customer->addComponent<CanPerformJob>().update(Wandering, Wandering);
        customer->addComponent<HasBaseSpeed>().update(10.f);

        customer->addComponent<CanHaveAilment>().update(
            std::make_shared<Insomnia>());

        customer->get<HasName>().update(get_random_name());
        customer->get<CanPerformJob>().update(WaitInQueue, Wandering);

        // TODO turn back on bubbles
        // std::optional<SpeechBubble> bubble;
        // bubble = SpeechBubble(ailment->icon_name());

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

        return customer;
    }
};

typedef Person AIPerson;
typedef AIPerson Customer;
