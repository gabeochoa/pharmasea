
#pragma once

#include "external_include.h"
//
#include "engine/globals_register.h"
#include "engine/keymap.h"
#include "entity.h"
#include "entityhelper.h"
//
#include "ailment.h"
#include "camera.h"
#include "components/can_be_ghost_player.h"
#include "components/can_have_ailment.h"
#include "components/can_highlight_others.h"
#include "components/can_hold_furniture.h"
#include "components/can_hold_item.h"
#include "components/can_perform_job.h"
#include "components/collects_user_input.h"
#include "components/has_base_speed.h"
#include "components/has_client_id.h"
#include "components/is_snappable.h"
#include "components/responds_to_user_input.h"
#include "components/uses_character_model.h"
#include "drawing_util.h"
#include "engine/astar.h"
#include "engine/globals_register.h"
#include "engine/is_server.h"
#include "engine/log.h"
#include "engine/sound_library.h"
#include "engine/statemanager.h"
#include "engine/texture_library.h"
#include "entityhelper.h"
#include "external_include.h"
#include "furniture.h"
#include "globals.h"
#include "job.h"
#include "names.h"
#include "person.h"
#include "raylib.h"
#include "text_util.h"

static void add_person_components(Entity* person) {
    person->get<Transform>().size =
        vec3{TILESIZE * 0.75f, TILESIZE * 0.75f, TILESIZE * 0.75f};

    person->addComponent<HasBaseSpeed>().update(10.f);
    // TODO why do we need the udpate() here?
    person->addComponent<ModelRenderer>().update(ModelInfo{
        .model_name = "character_duck",
        .size_scale = 1.5f,
        .position_offset = vec3{0, 0, 0},
        .rotation_angle = 180,
    });

    person->addComponent<UsesCharacterModel>();

    // TODO this came from AIPersons and even though all AI People are
    // adding it themselves we have to thave this here for the time being
    // ... otherwise the game just crashes
    person->addComponent<CanPerformJob>().update(Wandering, Wandering);
}

struct Person : public Entity {
   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<Entity>{});
    }

   public:
    Person() : Person({0, 0, 0}) {}

    Person(vec3 p) : Entity(p, WHITE, WHITE) { add_person_components(this); }
};

static Entity* make_remote_player(vec3 pos) {
    Entity* remote_player = new Entity(pos, WHITE, WHITE);

    add_person_components(remote_player);

    remote_player->addComponent<CanHighlightOthers>();
    remote_player->addComponent<CanHoldFurniture>();

    remote_player->get<HasBaseSpeed>().update(7.5f);

    remote_player->addComponent<HasClientID>();
    return remote_player;
}

static void update_player_remotely(std::shared_ptr<Entity> entity,
                                   float* location, std::string username,
                                   int facing_direction) {
    HasName& hasname = entity->get<HasName>();
    hasname.name = username;

    entity->get<HasName>().name = username;
    Transform& transform = entity->get<Transform>();
    // TODO add setters
    transform.position = vec3{location[0], location[1], location[2]};
    transform.face_direction =
        static_cast<Transform::FrontFaceDirection>(facing_direction);
}

static Entity* make_player(vec3 p) {
    Entity* player = new Entity(p, WHITE, WHITE);
    add_person_components(player);

    player->addComponent<CanHighlightOthers>();
    player->addComponent<CanHoldFurniture>();

    // addComponent<HasBaseSpeed>().update(10.f);
    player->get<HasBaseSpeed>().update(7.5f);

    // TODO what is a reasonable default value here?
    // TODO who sets this value?
    // this comes from remote player and probably should only be there
    player->addComponent<HasClientID>();

    player->addComponent<CanBeGhostPlayer>();
    player->addComponent<CollectsUserInput>();
    player->addComponent<RespondsToUserInput>();

    return player;
}

static Entity* make_aiperson(vec3 p) {
    // TODO This cant use make_person due to segfault in render model
    Entity* person = new Person(p);
    // Entity* person = new Entity(p, WHITE, WHITE);
    // add_person_components(person);

    person->addComponent<CanPerformJob>().update(Wandering, Wandering);
    person->addComponent<HasBaseSpeed>().update(10.f);

    return person;
}

static Entity* make_customer(vec3 p) {
    Entity* customer = make_aiperson(p);

    customer->addComponent<CanPerformJob>().update(Wandering, Wandering);
    customer->addComponent<HasBaseSpeed>().update(10.f);

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
