
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
#include "components/has_dynamic_model_name.h"
#include "components/has_name.h"
#include "components/has_speech_bubble.h"
#include "components/has_timer.h"
#include "components/has_waiting_queue.h"
#include "components/has_work.h"
#include "components/indexer.h"
#include "components/is_item_container.h"
#include "components/is_rotatable.h"
#include "components/is_snappable.h"
#include "components/is_solid.h"
#include "components/is_spawner.h"
#include "components/is_trigger_area.h"
#include "components/model_renderer.h"
#include "components/responds_to_user_input.h"
#include "components/shows_progress_bar.h"
#include "components/simple_colored_box_renderer.h"
#include "components/transform.h"
#include "components/uses_character_model.h"
#include "engine/assert.h"
#include "external_include.h"
#include "strings.h"
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
#include "engine.h"
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

    // These two functions can be used to validate than an entity has all of the
    // matching components that are needed for this system to run
    template<typename T>
    [[nodiscard]] bool has() const {
        log_trace("checking component {} {} on entity {}",
                  components::get_type_id<T>(), type_name<T>(), id);
        log_trace("your set is now {}", componentSet);
        bool result = componentSet[components::get_type_id<T>()];
        log_trace("and the result was {}", result);
        return result;
    }

    template<typename A, typename B, typename... Rest>
    bool has() const {
        return has<A>() && has<B, Rest...>();
    }

    template<typename T>
    [[nodiscard]] bool is_missing() const {
        return !has<T>();
    }

    template<typename A>
    bool is_missing_any() const {
        return is_missing<A>();
    }

    template<typename A, typename B, typename... Rest>
    bool is_missing_any() const {
        return is_missing<A>() || is_missing_any<B, Rest...>();
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

    template<typename A>
    void addAll() {
        addComponent<A>();
    }

    template<typename A, typename B, typename... Rest>
    void addAll() {
        addComponent<A>();
        addAll<B, Rest...>();
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

static void register_all_components() {
    Entity* entity = new Entity();
    entity->addAll<
        Transform, HasName, CanHoldItem, SimpleColoredBoxRenderer,
        CanBeHighlighted, CanHighlightOthers, CanHoldFurniture,
        CanBeGhostPlayer, CanPerformJob, ModelRenderer, CanBePushed,
        CanHaveAilment, CustomHeldItemPosition, HasWork, HasBaseSpeed, IsSolid,
        CanBeHeld, IsRotatable, CanGrabFromOtherFurniture, ConveysHeldItem,
        HasWaitingQueue, CanBeTakenFrom, IsItemContainer<Bag>,
        IsItemContainer<PillBottle>, IsItemContainer<Pill>, UsesCharacterModel,
        ShowsProgressBar, DebugName, HasDynamicModelName, IsTriggerArea,
        HasSpeechBubble, Indexer, IsSpawner, HasTimer>();
    delete entity;
}

struct DebugOptions {
    std::string name;
};

static bool check_name(std::shared_ptr<Entity> entity, const char* name) {
    return entity->get<DebugName>().name() == name;
}

static void add_entity_components(Entity* entity) {
    entity->addComponent<Transform>();
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
    float size_multiplier = 0.75f;
    person->get<Transform>().update_size(vec3{
        TILESIZE * size_multiplier,
        TILESIZE * size_multiplier,
        TILESIZE * size_multiplier,
    });

    person->addComponent<CanHoldItem>();
    person->addComponent<CanBePushed>();

    person->addComponent<HasBaseSpeed>().update(10.f);

    if (ENABLE_MODELS) {
        // TODO why do we need the update() here?
        person->addComponent<ModelRenderer>().update(ModelInfo{
            .model_name = "character_duck",
            .size_scale = 1.5f,
            .position_offset = vec3{0, 0, 0},
            .rotation_angle = 180,
        });
    } else {
        person->addComponent<SimpleColoredBoxRenderer>().update(RED, PINK);
    }

    person->addComponent<UsesCharacterModel>();
}

static void add_player_components(Entity* player) {
    player->addComponent<CanHighlightOthers>();
    player->addComponent<CanHoldFurniture>();
    player->get<HasBaseSpeed>().update(7.5f);
    player->addComponent<HasName>();
    player->addComponent<HasClientID>();
}

static Entity* make_remote_player(vec3 pos) {
    Entity* remote_player =
        make_entity({.name = strings::entity::REMOTE_PLAYER}, pos);
    add_person_components(remote_player);
    add_player_components(remote_player);
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
                                        int model_index, int last_ping) {
    if (entity
            ->is_missing_any<UsesCharacterModel, ModelRenderer, HasClientID>())
        return;
    UsesCharacterModel& ucm = entity->get<UsesCharacterModel>();
    ModelRenderer& renderer = entity->get<ModelRenderer>();

    ucm.update_index_CLIENT_ONLY(model_index);

    entity->get<HasClientID>().update_ping(last_ping);

    // TODO this should be the same as all other rendere updates for players
    renderer.update(ModelInfo{
        .model_name = ucm.fetch_model_name(),
        .size_scale = 1.5f,
        .position_offset = vec3{0, 0, 0},
        .rotation_angle = 180,
    });
}

static Entity* make_player(vec3 p) {
    Entity* player = make_entity({.name = strings::entity::PLAYER}, p);
    add_person_components(player);
    add_player_components(player);

    // note: these are added to some remote players
    // ie the one the client is controlling
    player->addComponent<CollectsUserInput>();
    player->addComponent<CanBeGhostPlayer>();

    player->addComponent<RespondsToUserInput>();
    return player;
}

static Entity* make_aiperson(const DebugOptions& options, vec3 p) {
    Entity* person = make_entity(options, p);
    add_person_components(person);

    person->addComponent<CanPerformJob>().update(Wandering, Wandering);
    return person;
}

static Entity* make_customer(vec2 p, bool has_ailment = true) {
    Entity* customer = make_aiperson(
        DebugOptions{.name = strings::entity::CUSTOMER}, vec::to3(p));

    customer->addComponent<HasName>().update(get_random_name());

    // TODO for now, eventually move to customer spawner
    if (has_ailment)
        customer->addComponent<CanHaveAilment>().update(
            Ailment::make_insomnia());

    customer->addComponent<HasSpeechBubble>();

    customer->get<HasBaseSpeed>().update(10.f);
    customer->get<CanPerformJob>().update(WaitInQueue, Wandering);

    return customer;
}

typedef Entity Furniture;

// TODO This namespace should probably be "furniture::"
// or add the ones above into it
namespace entities {
static Entity* make_furniture(const DebugOptions& options, vec2 pos, Color face,
                              Color base, bool is_static = false) {
    Entity* furniture = make_entity(options);

    furniture->get<Transform>().init({pos.x, 0, pos.y},
                                     {TILESIZE, TILESIZE, TILESIZE});
    furniture->addComponent<IsSolid>();

    // For renderers we prioritize the ModelRenderer and will fall back if we
    // need
    furniture->addComponent<SimpleColoredBoxRenderer>().update(face, base);
    if (ENABLE_MODELS) {
        furniture->addComponent<ModelRenderer>();
    }

    // we need to add it to set a default, so its here
    furniture->addComponent<CustomHeldItemPosition>().init(
        CustomHeldItemPosition::Positioner::Default);

    // Walls should not have these
    if (!is_static) {
        furniture->addComponent<IsRotatable>();
        furniture->addComponent<CanHoldItem>();
        // These two are the heavy ones
        furniture->addComponent<CanBeHeld>();
        furniture->addComponent<CanBeHighlighted>();
    }

    return furniture;
}

static Entity* make_table(vec2 pos) {
    Entity* table =
        entities::make_furniture(DebugOptions{.name = strings::entity::TABLE},
                                 pos, ui::color::brown, ui::color::brown);

    table->get<CustomHeldItemPosition>().init(
        CustomHeldItemPosition::Positioner::Table);

    table->addComponent<HasWork>().init([](std::shared_ptr<Entity>,
                                           HasWork& hasWork,
                                           std::shared_ptr<Entity>, float dt) {
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
    Entity* character_switcher = entities::make_furniture(
        DebugOptions{.name = strings::entity::CHARACTER_SWITCHER}, pos,
        ui::color::green, ui::color::yellow);

    character_switcher->addComponent<HasWork>().init(
        [](std::shared_ptr<Entity>, HasWork& hasWork,
           std::shared_ptr<Entity> person, float dt) {
            if (person->is_missing<UsesCharacterModel>()) return;
            UsesCharacterModel& usesCharacterModel =
                person->get<UsesCharacterModel>();

            const float amt = 1.5f;
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
    Entity* wall = entities::make_furniture(
        DebugOptions{.name = strings::entity::WALL}, pos, c, c, true);

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
    Entity* conveyer = entities::make_furniture(
        DebugOptions{.name = strings::entity::CONVEYER}, pos, ui::color::blue,
        ui::color::blue);
    conveyer->get<CustomHeldItemPosition>().init(
        CustomHeldItemPosition::Positioner::Conveyer);
    conveyer->addComponent<ConveysHeldItem>();
    conveyer->addComponent<CanBeTakenFrom>();

    if (ENABLE_MODELS) {
        // TODO we probably want this and grabber to be 100% the same
        conveyer->get<ModelRenderer>().update(ModelInfo{
            .model_name = "conveyer",
            .size_scale = 3.f,
            .position_offset = vec3{0, 0, 0},
            .rotation_angle = 90.f,
        });
    }

    return conveyer;
}

[[nodiscard]] static Entity* make_grabber(vec2 pos) {
    Entity* grabber =
        entities::make_furniture(DebugOptions{.name = strings::entity::GRABBER},
                                 pos, ui::color::yellow, ui::color::yellow);

    grabber->get<CustomHeldItemPosition>().init(
        CustomHeldItemPosition::Positioner::Conveyer);
    grabber->addComponent<CanBeTakenFrom>();

    if (ENABLE_MODELS) {
        grabber->get<ModelRenderer>().update(ModelInfo{
            .model_name = "conveyer",
            .size_scale = 3.f,
            .position_offset = vec3{0, 0, 0},
            .rotation_angle = 90.f,
        });
    }
    grabber->addComponent<ConveysHeldItem>();
    grabber->addComponent<CanGrabFromOtherFurniture>();
    return grabber;
}

[[nodiscard]] static Entity* make_register(vec2 pos) {
    Entity* reg = entities::make_furniture(
        DebugOptions{.name = strings::entity::REGISTER}, pos, ui::color::grey,
        ui::color::grey);
    reg->addComponent<HasWaitingQueue>();

    if (ENABLE_MODELS) {
        reg->get<ModelRenderer>().update(ModelInfo{
            .model_name = "register",
            .size_scale = 10.f,
            .position_offset = vec3{0, -TILESIZE / 2.f, 0},
        });
    }
    return reg;
}

template<typename I>
[[nodiscard]] static Entity* make_itemcontainer(const DebugOptions& options,
                                                vec2 pos) {
    Entity* container = entities::make_furniture(options, pos, ui::color::white,
                                                 ui::color::white);
    container->addComponent<IsItemContainer<I>>();
    return container;
}

[[nodiscard]] static Entity* make_bagbox(vec2 pos) {
    Entity* container =
        entities::make_itemcontainer<Bag>({strings::entity::BAG_BOX}, pos);

    if (ENABLE_MODELS) {
        container->get<ModelRenderer>().update(ModelInfo{
            .model_name = "box",
            .size_scale = 4.f,
            .position_offset = vec3{0, -TILESIZE / 2.f, 0},
        });

        container->addComponent<HasDynamicModelName>().init(
            "box", HasDynamicModelName::DynamicType::OpenClosed);
    }

    return container;
}

[[nodiscard]] static Entity* make_medicine_cabinet(vec2 pos) {
    Entity* container = entities::make_itemcontainer<PillBottle>(
        {strings::entity::MEDICINE_CABINET}, pos);
    if (ENABLE_MODELS) {
        container->get<ModelRenderer>().update(ModelInfo{
            .model_name = "medicine_cabinet",
            .size_scale = 2.f,
            .position_offset = vec3{0, -TILESIZE / 2.f, 0},
        });
    }
    return container;
}

[[nodiscard]] static Entity* make_pill_dispenser(vec2 pos) {
    // TODO when making a new itemcontainer, it silently creates a new
    // component and then youll get a polymorphism error, probably need
    // something
    Entity* container = entities::make_itemcontainer<Pill>(
        {strings::entity::PILL_DISPENSER}, pos);
    if (ENABLE_MODELS) {
        container->get<ModelRenderer>().update(ModelInfo{
            .model_name = "crate",
            .size_scale = 2.f,
            .position_offset = vec3{0, -TILESIZE / 2.f, 0},
        });
    }
    container->addComponent<Indexer>(
        (int) magic_enum::enum_count<Pill::PillType>());

    container->addComponent<HasWork>().init(
        [](std::shared_ptr<Entity> owner, HasWork& hasWork,
           std::shared_ptr<Entity>, float dt) {
            const float amt = 2.f;
            hasWork.increase_pct(amt * dt);
            if (hasWork.is_work_complete()) {
                owner->get<Indexer>().increment();
                hasWork.reset_pct();
            }
        });
    container->addComponent<ShowsProgressBar>();
    container->get<CustomHeldItemPosition>().init(
        CustomHeldItemPosition::Positioner::Table);
    return container;
}

[[nodiscard]] static Entity* make_trigger_area(
    vec3 pos, float width, float height,
    std::string title = strings::entity::TRIGGER_AREA) {
    Entity* trigger_area = make_entity({strings::entity::TRIGGER_AREA}, pos);

    trigger_area->get<Transform>().update_size({
        width,
        TILESIZE / 20.f,
        height,
    });

    trigger_area->addComponent<SimpleColoredBoxRenderer>().update(PINK, PINK);
    trigger_area->addComponent<IsTriggerArea>()
        .update_title(title)
        .update_max_entrants(1)
        .update_progress_max(2.f)
        .on_complete([]() {
            // TODO should be lobby only?
            // TODO only for host...
            GameState::s_toggle_to_planning();
        });

    return trigger_area;
}

[[nodiscard]] static Entity* make_customer_spawner(vec3 pos) {
    Entity* customer_spawner =
        make_entity({strings::entity::CUSTOMER_SPAWNER}, pos);

    // TODO maybe one day add some kind of ui that shows when the next
    // person is coming? that migth be good to be part of the round
    // timer ui?
    customer_spawner->addComponent<SimpleColoredBoxRenderer>().update(PINK,
                                                                      PINK);
    const auto sfn = std::bind(&make_customer, std::placeholders::_1, true);
    customer_spawner
        ->addComponent<IsSpawner>()  //
        .set_fn(sfn)
        .set_total(3)
        .set_time_between(2.f);

    return customer_spawner;
}

// This will be a catch all for anything that just needs to get updated
[[nodiscard]] static Entity* make_sophie(vec3 pos) {
    Entity* sophie = make_entity({strings::entity::SOPHIE}, pos);

    // TODO how long is a day?
    sophie->addComponent<HasTimer>(HasTimer::Renderer::Round, 90.f);

    return sophie;
}

}  // namespace entities
