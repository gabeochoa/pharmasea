
#pragma once

#include "external_include.h"
//
#include "engine/globals_register.h"
#include "engine/keymap.h"
#include "entity.h"
#include "entityhelper.h"
//
#include "components/can_be_ghost_player.h"
#include "components/can_highlight_others.h"
#include "components/can_hold_furniture.h"
#include "components/collects_user_input.h"
#include "components/has_base_speed.h"
#include "components/responds_to_user_input.h"
#include "components/uses_character_model.h"
#include "furniture.h"
#include "globals.h"
#include "person.h"
#include "raylib.h"
#include "statemanager.h"

struct Person : public Entity {
   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<Entity>{});
    }

   protected:
    Person() : Person({0, 0, 0}) {}

    Person(vec3 p) : Entity(p, WHITE, WHITE) { update_component(); }

   public:
    void update_component() {
        get<Transform>().size =
            vec3{TILESIZE * 0.75f, TILESIZE * 0.75f, TILESIZE * 0.75f};

        addComponent<HasBaseSpeed>().update(10.f);
        // TODO why do we need the udpate() here?
        addComponent<ModelRenderer>().update(ModelInfo{
            .model_name = "character_duck",
            .size_scale = 1.5f,
            .position_offset = vec3{0, 0, 0},
            .rotation_angle = 180,
        });

        addComponent<UsesCharacterModel>();
    }
};

#include "components/has_client_id.h"

// TODO add more information on what the difference is between Person and
// BasePlayer and Player
struct BasePlayer : public Person {
    void add_static_components() {
        addComponent<CanHighlightOthers>();
        addComponent<CanHoldFurniture>();

        // addComponent<HasBaseSpeed>().update(10.f);
        get<HasBaseSpeed>().update(7.5f);

        // TODO what is a reasonable default value here?
        // TODO who sets this value?
        // this comes from remote player and probably should only be there
        addComponent<HasClientID>();
    }

    BasePlayer(vec3 p) : Person(p) { add_static_components(); }
    BasePlayer() : BasePlayer({0, 0, 0}) {}
    BasePlayer(vec2 location) : BasePlayer({location.x, 0, location.y}) {}
};

struct RemotePlayer : public BasePlayer {
    RemotePlayer(vec3 p) : BasePlayer(p) {}
    RemotePlayer() : BasePlayer({0, 0, 0}) {}
    RemotePlayer(vec2 location) : BasePlayer({location.x, 0, location.y}) {}

    static void update_remotely(std::shared_ptr<Entity> entity, float* location,
                                std::string username, int facing_direction) {
        HasName& hasname = entity->get<HasName>();
        hasname.name = username;

        entity->get<HasName>().name = username;
        Transform& transform = entity->get<Transform>();
        // TODO add setters
        transform.position = vec3{location[0], location[1], location[2]};
        transform.face_direction =
            static_cast<Transform::FrontFaceDirection>(facing_direction);
    }
};

struct Player : public BasePlayer {
    void add_static_components() {
        addComponent<CanBeGhostPlayer>();
        addComponent<CollectsUserInput>();
        addComponent<RespondsToUserInput>();
    }

    // NOTE: this is kept public because we use it in the network when prepping
    // server players
    Player() : BasePlayer({0, 0, 0}) { add_static_components(); }

    Player(vec3 p) : BasePlayer(p) { add_static_components(); }
    Player(vec2 location) : Player({location.x, 0, location.y}) {}
};

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

static Entity* make_customer(vec3 p) {
    Entity* customer = new AIPerson(p);

    customer->addComponent<CanHaveAilment>().update(
        std::make_shared<Insomnia>());
    customer->get<HasName>().update(get_random_name());
    customer->get<CanPerformJob>().update(WaitInQueue, Wandering);

    return customer;

    // TODO turn back on bubbles
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
}
