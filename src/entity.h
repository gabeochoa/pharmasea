
#pragma once

#include "components/base_component.h"
#include "components/can_be_ghost_player.h"
#include "components/can_be_held.h"
#include "components/can_be_highlighted.h"
#include "components/can_be_pushed.h"
#include "components/can_be_taken_from.h"
#include "components/can_grab_from_other_furniture.h"
#include "components/can_have_ailment.h"
#include "components/can_highlight_others.h"
#include "components/can_hold_furniture.h"
#include "components/can_hold_item.h"
#include "components/can_perform_job.h"
#include "components/collects_user_input.h"
#include "components/conveys_held_item.h"
#include "components/custom_item_position.h"
#include "components/debug_name.h"
#include "components/has_base_speed.h"
#include "components/has_client_id.h"
#include "components/has_name.h"
#include "components/has_waiting_queue.h"
#include "components/has_work.h"
#include "components/is_item_container.h"
#include "components/is_rotatable.h"
#include "components/is_snappable.h"
#include "components/is_solid.h"
#include "components/model_renderer.h"
#include "components/responds_to_user_input.h"
#include "components/shows_progress_bar.h"
#include "components/simple_colored_box_renderer.h"
#include "components/transform.h"
#include "components/uses_character_model.h"
#include "engine/assert.h"
#include "external_include.h"
//
#include <bitsery/ext/pointer.h>
#include <bitsery/ext/std_map.h>

#include <array>
#include <map>

using bitsery::ext::PointerOwner;
using bitsery::ext::PointerType;
using StdMap = bitsery::ext::StdMap;

#include "dataclass/names.h"
#include "drawing_util.h"
#include "engine/astar.h"
#include "engine/is_server.h"
#include "engine/model_library.h"
#include "engine/util.h"
#include "globals.h"
#include "item.h"
#include "item_helper.h"
#include "preload.h"
#include "raylib.h"
#include "text_util.h"
#include "vec_util.h"

static std::atomic_int ENTITY_ID_GEN = 0;
struct Entity {
    int id;

    ComponentBitSet componentSet;
    ComponentArray componentArray;

    bool cleanup = false;

    template<typename T>
    [[nodiscard]] bool has() const {
        log_trace("checking component {} {} on entity {}",
                  components::get_type_id<T>(), type_name<T>(), id);
        log_trace("your set is now {}", componentSet);
        bool result = componentSet[components::get_type_id<T>()];
        log_trace("and the result was {}", result);
        return result;
    }

    template<typename T>
    [[nodiscard]] bool is_missing() const {
        return !has<T>();
    }

    template<typename T, typename... TArgs>
    T& addComponent(TArgs&&... args) {
        log_trace("adding component_id:{} {} to entity_id: {}",
                  components::get_type_id<T>(), type_name<T>(), id);

        if (this->has<T>()) {
            log_warn(
                "This entity {}, {} already has this component attached id: "
                "{}, "
                "component {}",
                this->get<DebugName>().name(), id, components::get_type_id<T>(),
                type_name<T>());

            VALIDATE(false, "duplicate component");
            // Commented out on purpose because the assert is gonna kill the
            // program anyway at some point we should stop enforcing it to avoid
            // crashing the game when people are playing
            //
            // return this->get<T>();
        }

        T* component(new T(std::forward<TArgs>(args)...));
        componentArray[components::get_type_id<T>()] = component;
        componentSet[components::get_type_id<T>()] = true;

        log_trace("your set is now {}", componentSet);

        component->onAttach();

        return *component;
    }

    template<typename T>
    [[nodiscard]] T& get() const {
        if (this->is_missing<DebugName>()) {
            log_error(
                "This entity is missing debugname which will cause issues for "
                "if the get<> is missing");
        }
        if (this->is_missing<T>()) {
            log_warn(
                "This entity {} {} is missing id: {}, "
                "component {}",
                this->get<DebugName>().name(), id, components::get_type_id<T>(),
                type_name<T>());
        }
        BaseComponent* comp = componentArray.at(components::get_type_id<T>());
        return *static_cast<T*>(comp);
    }

    virtual ~Entity() {}

    Entity() : id(ENTITY_ID_GEN++) {}

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.value4b(id);
        s.ext(componentSet, bitsery::ext::StdBitset{});
        s.value1b(cleanup);

        s.ext(componentArray, StdMap{max_num_components},
              [](S& sv, int& key, BaseComponent*(&value)) {
                  sv.value4b(key);
                  sv.ext(value, PointerOwner{PointerType::Nullable});
              });
    }
};

typedef Transform::Transform::FrontFaceDirection EntityDir;

namespace bitsery {
template<typename S>
void serialize(S& s, std::shared_ptr<Entity>& entity) {
    s.ext(entity, bitsery::ext::StdSmartPtr{});
}
}  // namespace bitsery

// TODO from customer
// std::optional<SpeechBubble> bubble;
// struct SpeechBubble {
// vec3 position;
// // TODO we arent using any string functionality can we swap to const
// char*?
// // perhaps in a typedef?
// std::string icon_tex_name;
//
// explicit SpeechBubble(std::string icon) : icon_tex_name(icon) {}
//
// void update(float, vec3 pos) { this->position = pos; }
// void render() const {
// GameCam cam = GLOBALS.get<GameCam>("game_cam");
// raylib::Texture texture = TextureLibrary::get().get(icon_tex_name);
// raylib::DrawBillboard(cam.camera, texture,
// vec3{position.x,                      //
// position.y + (TILESIZE * 1.5f),  //
// position.z},                     //
// TILESIZE,                             //
// raylib::WHITE);
// }
// };
//

struct DebugOptions {
    std::string name;
};

static void add_entity_components(Entity* entity) {
    entity->addComponent<Transform>();
    // TODO figure out which entities need these
    entity->addComponent<CanBeHeld>();
}

static Entity* make_entity(const DebugOptions& options, vec3 p = {-2, -2, -2}) {
    Entity* entity = new Entity();

    entity->addComponent<DebugName>().update(options.name);

    add_entity_components(entity);

    entity->get<Transform>().update(p);
    return entity;
}

static void add_person_components(Entity* person) {
    // TODO idk why but you spawn under the ground without this
    person->get<Transform>().update_y(0);

    person->addComponent<CanHoldItem>();
    person->addComponent<CanBePushed>();

    person->get<Transform>().update_size(
        vec3{TILESIZE * 0.75f, TILESIZE * 0.75f, TILESIZE * 0.75f});

    person->addComponent<HasBaseSpeed>().update(10.f);
    // TODO why do we need the udpate() here?
    person->addComponent<ModelRenderer>().update(ModelInfo{
        .model_name = "character_duck",
        .size_scale = 1.5f,
        .position_offset = vec3{0, 0, 0},
        .rotation_angle = 180,
    });

    person->addComponent<UsesCharacterModel>();
}

static Entity* make_remote_player(vec3 pos) {
    Entity* remote_player = make_entity({.name = "remote player"}, pos);
    add_person_components(remote_player);

    remote_player->addComponent<CanHighlightOthers>();
    remote_player->addComponent<CanHoldFurniture>();

    remote_player->get<HasBaseSpeed>().update(7.5f);

    remote_player->addComponent<HasName>();
    remote_player->addComponent<HasClientID>();

    return remote_player;
}

static void update_player_remotely(std::shared_ptr<Entity> entity,
                                   float* location, std::string username,
                                   int facing_direction) {
    entity->get<HasName>().update(username);

    Transform& transform = entity->get<Transform>();

    transform.update(vec3{location[0], location[1], location[2]});
    transform.update_face_direction(
        static_cast<Transform::FrontFaceDirection>(facing_direction));
}

static void update_player_rare_remotely(std::shared_ptr<Entity> entity,
                                        int model_index) {
    entity->get<UsesCharacterModel>().update_index_CLIENT_ONLY(model_index);
}

static Entity* make_player(vec3 p) {
    Entity* player = make_entity({.name = "player"}, p);
    add_person_components(player);

    player->get<HasBaseSpeed>().update(7.5f);

    player->addComponent<HasName>();
    player->addComponent<CanHighlightOthers>();
    player->addComponent<CanHoldFurniture>();
    player->addComponent<CanBeGhostPlayer>();
    player->addComponent<CollectsUserInput>();
    player->addComponent<RespondsToUserInput>();

    return player;
}

static Entity* make_aiperson(vec3 p) {
    Entity* person = make_entity(DebugOptions{.name = "aiperson"}, p);
    add_person_components(person);

    person->addComponent<CanPerformJob>().update(Wandering, Wandering);
    return person;
}

static Entity* make_customer(vec3 p) {
    Entity* customer = make_aiperson(p);

    customer->addComponent<HasName>().update(get_random_name());

    customer->addComponent<CanHaveAilment>().update(
        std::make_shared<Insomnia>());

    customer->get<HasBaseSpeed>().update(10.f);
    // TODO turn back on normal customer ai
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

typedef Entity Furniture;

namespace entities {
static Entity* make_furniture(const DebugOptions& options, vec2 pos, Color face,
                              Color base) {
    Entity* furniture = make_entity(options);

    furniture->get<Transform>().init({pos.x, 0, pos.y},
                                     {TILESIZE, TILESIZE, TILESIZE});
    furniture->addComponent<IsSolid>();
    furniture->addComponent<IsRotatable>();

    // For renderers we prioritize the ModelRenderer and will fall back if we
    // need
    furniture->addComponent<SimpleColoredBoxRenderer>().update(face, base);
    furniture->addComponent<ModelRenderer>();

    // we need to add it to set a default, so its here
    furniture->addComponent<CustomHeldItemPosition>().init(
        CustomHeldItemPosition::Positioner::Default);

    // TODO does all furniture have this?
    furniture->addComponent<CanHoldItem>();

    return furniture;
}

static Entity* make_table(vec2 pos) {
    Entity* table = entities::make_furniture(
        DebugOptions{.name = "table"}, pos, ui::color::brown, ui::color::brown);

    table->get<CustomHeldItemPosition>().init(
        CustomHeldItemPosition::Positioner::Table);

    table->addComponent<HasWork>().init(
        [](HasWork& hasWork, std::shared_ptr<Entity>, float dt) {
            // TODO eventually we need it to decide whether it has work
            // based on the current held item
            const float amt = 0.5f;
            hasWork.increase_pct(amt * dt);
            if (hasWork.is_work_complete()) hasWork.reset_pct();
        });
    table->addComponent<ShowsProgressBar>();
    return table;
}

static Entity* make_character_switcher(vec2 pos) {
    Entity* character_switcher =
        entities::make_furniture(DebugOptions{.name = "character switcher"},
                                 pos, ui::color::green, ui::color::yellow);

    character_switcher->addComponent<HasWork>().init(
        [](HasWork& hasWork, std::shared_ptr<Entity> person, float dt) {
            if (person->is_missing<UsesCharacterModel>()) return;
            UsesCharacterModel& usesCharacterModel =
                person->get<UsesCharacterModel>();

            const float amt = 2.f;
            hasWork.increase_pct(amt * dt);
            if (hasWork.is_work_complete()) {
                hasWork.reset_pct();
                usesCharacterModel.increment();
            }
        });
    character_switcher->addComponent<ShowsProgressBar>();
    return character_switcher;
}

static Entity* make_wall(vec2 pos, Color c = ui::color::brown) {
    Entity* wall =
        entities::make_furniture(DebugOptions{.name = "wall"}, pos, c, c);

    return wall;
    // enum Type {
    // FULL,
    // HALF,
    // QUARTER,
    // CORNER,
    // TEE,
    // DOUBLE_TEE,
    // };
    //
    // Type type = FULL;

    // TODO need to make sure we dont have this
    // its inherited from entity...
    // addComponent<CanHoldItem>();
    // virtual bool can_place_item_into(std::shared_ptr<Item>) override {
    // return false;
    // }

    // virtual void render_normal() const override {
    // TODO fix
    // switch (this->type) {
    // case Type::DOUBLE_TEE: {
    // DrawCubeCustom(this->raw_position,                        //
    // this->size().x / 2,                        //
    // this->size().y,                            //
    // this->size().z,                            //
    // FrontFaceDirectionMap.at(face_direction),  //
    // this->face_color, this->base_color);
    // DrawCubeCustom(this->raw_position,                        //
    // this->size().x,                            //
    // this->size().y,                            //
    // this->size().z / 2,                        //
    // FrontFaceDirectionMap.at(face_direction),  //
    // this->face_color, this->base_color);
    // } break;
    // case Type::FULL: {
    // DrawCubeCustom(this->raw_position,                        //
    // this->size().x,                            //
    // this->size().y,                            //
    // this->size().z,                            //
    // FrontFaceDirectionMap.at(face_direction),  //
    // this->face_color, this->base_color);
    // } break;
    // case Type::HALF: {
    // DrawCubeCustom(this->raw_position,                        //
    // this->size().x,                            //
    // this->size().y,                            //
    // this->size().z / 2,                        //
    // FrontFaceDirectionMap.at(face_direction),  //
    // this->face_color, this->base_color);
    // } break;
    // case Type::CORNER: {
    // DrawCubeCustom(this->raw_position,                        //
    // this->size().x / 2,                        //
    // this->size().y,                            //
    // this->size().z,                            //
    // FrontFaceDirectionMap.at(face_direction),  //
    // this->face_color, this->base_color);
    // DrawCubeCustom(this->raw_position,                        //
    // this->size().x,                            //
    // this->size().y,                            //
    // this->size().z / 2,                        //
    // FrontFaceDirectionMap.at(face_direction),  //
    // this->face_color, this->base_color);
    // } break;
    // case Type::TEE: {
    // DrawCubeCustom(this->raw_position,                        //
    // this->size().x / 2,                        //
    // this->size().y,                            //
    // this->size().z,                            //
    // FrontFaceDirectionMap.at(face_direction),  //
    // this->face_color, this->base_color);
    // DrawCubeCustom(this->raw_position,                        //
    // this->size().x,                            //
    // this->size().y,                            //
    // this->size().z / 2,                        //
    // FrontFaceDirectionMap.at(face_direction),  //
    // this->face_color, this->base_color);
    // } break;
    // case Type::QUARTER:
    // break;
    // }
    // }
}

[[nodiscard]] static Entity* make_conveyer(vec2 pos) {
    Entity* conveyer =
        entities::make_furniture(DebugOptions{.name = "conveyer"}, pos,
                                 ui::color::blue, ui::color::blue);
    // TODO fix
    // bool can_take_item_from() const {
    // return (get<CanHoldItem>().is_holding_item() && can_take_from);
    // }

    conveyer->get<CustomHeldItemPosition>().init(
        CustomHeldItemPosition::Positioner::Conveyer);
    conveyer->addComponent<ConveysHeldItem>();
    conveyer->addComponent<CanBeTakenFrom>();

    // TODO add a component for this
    conveyer->get<ModelRenderer>().update(ModelInfo{
        .model_name = "conveyer",
        .size_scale = 0.5f,
        .position_offset = vec3{0, 0, 0},
    });

    return conveyer;
}

[[nodiscard]] static Entity* make_grabber(vec2 pos) {
    Entity* grabber =
        entities::make_furniture(DebugOptions{.name = "grabber"}, pos,
                                 ui::color::yellow, ui::color::yellow);

    grabber->get<CustomHeldItemPosition>().init(
        CustomHeldItemPosition::Positioner::Conveyer);
    grabber->addComponent<CanBeTakenFrom>();

    grabber->get<ModelRenderer>().update(ModelInfo{
        .model_name = "conveyer",
        .size_scale = 0.5f,
        .position_offset = vec3{0, 0, 0},
    });
    grabber->addComponent<ConveysHeldItem>();
    grabber->addComponent<CanGrabFromOtherFurniture>();
    return grabber;
}

[[nodiscard]] static Entity* make_register(vec2 pos) {
    Entity* reg =
        entities::make_furniture(DebugOptions{.name = "register"}, pos,
                                 ui::color::grey, ui::color::grey);
    reg->addComponent<HasWaitingQueue>();

    reg->get<ModelRenderer>().update(ModelInfo{
        .model_name = "register",
        .size_scale = 10.f,
        .position_offset = vec3{0, -TILESIZE / 2.f, 0},
    });
    return reg;
}

template<typename I>
[[nodiscard]] static Entity* make_itemcontainer(vec2 pos) {
    Entity* container =
        entities::make_furniture(DebugOptions{.name = "item container"}, pos,
                                 ui::color::white, ui::color::white);
    container->addComponent<IsItemContainer<I>>();
    return container;
    // virtual bool can_place_item_into(
    // std::shared_ptr<Item> item = nullptr) override {
    // return this->get<IsItemContainer<I>>().is_matching_item(item);
    // }
}

[[nodiscard]] static Entity* make_bagbox(vec2 pos) {
    Entity* container = entities::make_itemcontainer<Bag>(pos);

    const bool in_planning = GameState::get().is(game::State::Planning);
    container->get<ModelRenderer>().update(ModelInfo{
        .model_name = in_planning ? "box" : "open_box",
        .size_scale = 4.f,
        .position_offset = vec3{0, -TILESIZE / 2.f, 0},
    });
    return container;
}

[[nodiscard]] static Entity* make_medicine_cabinet(vec2 pos) {
    Entity* container = entities::make_itemcontainer<PillBottle>(pos);
    container->get<ModelRenderer>().update(ModelInfo{
        .model_name = "medicine_cabinet",
        .size_scale = 2.f,
        .position_offset = vec3{0, -TILESIZE / 2.f, 0},
    });
    return container;
}
}  // namespace entities
