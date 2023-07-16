

#pragma once

#include "entity.h"
//
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

static void register_all_components() {
    Entity* entity = new Entity();
    entity->addAll<
        Transform, HasName, CanHoldItem, SimpleColoredBoxRenderer,
        CanBeHighlighted, CanHighlightOthers, CanHoldFurniture,
        CanBeGhostPlayer, CanPerformJob, ModelRenderer, CanBePushed,
        CanHaveAilment, CustomHeldItemPosition, HasWork, HasBaseSpeed, IsSolid,
        CanBeHeld, IsRotatable, CanGrabFromOtherFurniture, ConveysHeldItem,
        HasWaitingQueue, CanBeTakenFrom, IsItemContainer, UsesCharacterModel,
        ShowsProgressBar, DebugName, HasDynamicModelName, IsTriggerArea,
        HasSpeechBubble, Indexer, IsSpawner, HasTimer, HasSubtype, IsItem
        //
        >();

    // Now that they are all registered we can delete them
    //
    // since we dont have a destructor today TODO because we are copying
    // components we have to delete these manually before the ent delete because
    // otherwise it will leak the memory
    //

    for (auto it = entity->componentArray.cbegin(), next_it = it;
         it != entity->componentArray.cend(); it = next_it) {
        ++next_it;
        BaseComponent* comp = it->second;
        if (comp) delete comp;
        entity->componentArray.erase(it);
    }

    delete entity;
}

static void add_entity_components(Entity& entity) {
    entity.addComponent<Transform>();
}

static void add_person_components(Entity& person) {
    // TODO idk why but you spawn under the ground without this
    person.get<Transform>().update_y(0);
    float size_multiplier = 0.75f;
    person.get<Transform>().update_size(vec3{
        TILESIZE * size_multiplier,
        TILESIZE * size_multiplier,
        TILESIZE * size_multiplier,
    });

    person.addComponent<CanHoldItem>(IsItem::HeldBy::PLAYER);
    person.addComponent<CanBePushed>();

    person.addComponent<HasBaseSpeed>().update(10.f);

    if (ENABLE_MODELS) {
        // TODO why do we need the update() here?
        person.addComponent<ModelRenderer>().update(ModelInfo{
            .model_name = "character_duck",
            .size_scale = 1.5f,
            .position_offset = vec3{0, 0, 0},
            .rotation_angle = 180,
        });
    } else {
        person.addComponent<SimpleColoredBoxRenderer>().update(RED, PINK);
    }

    person.addComponent<UsesCharacterModel>();
}

static void make_entity(Entity& entity, const DebugOptions& options,
                        vec3 p = {-2, -2, -2}) {
    entity.addComponent<DebugName>().update(options.name);
    add_entity_components(entity);
    entity.get<Transform>().update(p);
}

static void add_player_components(Entity& player) {
    player.addComponent<CanHighlightOthers>();
    player.addComponent<CanHoldFurniture>();
    player.get<HasBaseSpeed>().update(7.5f);
    player.addComponent<HasName>();
    player.addComponent<HasClientID>();
}

static void make_remote_player(Entity& remote_player, vec3 pos) {
    make_entity(remote_player, {.name = strings::entity::REMOTE_PLAYER}, pos);
    add_person_components(remote_player);
    add_player_components(remote_player);
}

static void update_player_remotely(Entity& entity, float* location,
                                   std::string username, int facing_direction) {
    entity.get<HasName>().update(username);

    Transform& transform = entity.get<Transform>();

    transform.update(vec3{location[0], location[1], location[2]});
    transform.update_face_direction(
        static_cast<Transform::FrontFaceDirection>(facing_direction));
}

static void update_player_rare_remotely(Entity& entity, int model_index,
                                        int last_ping) {
    if (entity.is_missing_any<UsesCharacterModel, ModelRenderer, HasClientID>())
        return;
    UsesCharacterModel& ucm = entity.get<UsesCharacterModel>();
    ModelRenderer& renderer = entity.get<ModelRenderer>();

    ucm.update_index_CLIENT_ONLY(model_index);

    entity.get<HasClientID>().update_ping(last_ping);

    // TODO this should be the same as all other rendere updates for players
    renderer.update(ModelInfo{
        .model_name = ucm.fetch_model_name(),
        .size_scale = 1.5f,
        .position_offset = vec3{0, 0, 0},
        .rotation_angle = 180,
    });
}

static void make_player(Entity& player, vec3 p) {
    make_entity(player, {.name = strings::entity::PLAYER}, p);
    add_person_components(player);
    add_player_components(player);

    // note: these are added to some remote players
    // ie the one the client is controlling
    player.addComponent<CollectsUserInput>();
    player.addComponent<CanBeGhostPlayer>();

    player.addComponent<RespondsToUserInput>();
}

static void make_aiperson(Entity& person, const DebugOptions& options, vec3 p) {
    make_entity(person, options, p);
    add_person_components(person);

    person.addComponent<CanPerformJob>().update(Wandering, Wandering);
}

static void make_customer(Entity& customer, vec2 p, bool has_ailment = true) {
    make_aiperson(customer, DebugOptions{.name = strings::entity::CUSTOMER},
                  vec::to3(p));

    customer.addComponent<HasName>().update(get_random_name());

    // TODO for now, eventually move to customer spawner
    if (has_ailment)
        customer.addComponent<CanHaveAilment>().update(
            Ailment::make_insomnia());

    customer.addComponent<HasSpeechBubble>();

    customer.get<HasBaseSpeed>().update(10.f);
    customer.get<CanPerformJob>().update(WaitInQueue, Wandering);
}

typedef Entity Furniture;

// TODO This namespace should probably be "furniture::"
// or add the ones above into it
namespace entities {
static void make_furniture(Entity& furniture, const DebugOptions& options,
                           vec2 pos, Color face, Color base,
                           bool is_static = false) {
    make_entity(furniture, options);

    furniture.get<Transform>().init({pos.x, 0, pos.y},
                                    {TILESIZE, TILESIZE, TILESIZE});
    furniture.addComponent<IsSolid>();

    // For renderers we prioritize the ModelRenderer and will fall back if we
    // need
    furniture.addComponent<SimpleColoredBoxRenderer>().update(face, base);
    if (ENABLE_MODELS) {
        furniture.addComponent<ModelRenderer>();
    }

    // we need to add it to set a default, so its here
    furniture.addComponent<CustomHeldItemPosition>().init(
        CustomHeldItemPosition::Positioner::Default);

    // Walls should not have these
    if (!is_static) {
        furniture.addComponent<IsRotatable>();
        furniture.addComponent<CanHoldItem>(IsItem::HeldBy::UNKNOWN_FURNITURE);
        // These two are the heavy ones
        furniture.addComponent<CanBeHeld>();
        furniture.addComponent<CanBeHighlighted>();
    }
}

static void make_table(Entity& table, vec2 pos) {
    entities::make_furniture(table,
                             DebugOptions{.name = strings::entity::TABLE}, pos,
                             ui::color::brown, ui::color::brown);

    table.get<CustomHeldItemPosition>().init(
        CustomHeldItemPosition::Positioner::Table);

    table.addComponent<HasWork>().init(
        [](Entity&, HasWork& hasWork, Entity&, float dt) {
            // TODO eventually we need it to decide whether it has work
            // based on the current held item
            const float amt = 0.5f;
            hasWork.increase_pct(amt * dt);
            if (hasWork.is_work_complete()) hasWork.reset_pct();
        });
    table.addComponent<ShowsProgressBar>();
}

static void make_character_switcher(Entity& character_switcher, vec2 pos) {
    entities::make_furniture(
        character_switcher,
        DebugOptions{.name = strings::entity::CHARACTER_SWITCHER}, pos,
        ui::color::green, ui::color::yellow);

    character_switcher.addComponent<HasWork>().init(
        [](Entity&, HasWork& hasWork, Entity& person, float dt) {
            if (person.is_missing<UsesCharacterModel>()) return;
            UsesCharacterModel& usesCharacterModel =
                person.get<UsesCharacterModel>();

            const float amt = 1.5f;
            hasWork.increase_pct(amt * dt);
            if (hasWork.is_work_complete()) {
                hasWork.reset_pct();
                usesCharacterModel.increment();
            }
        });
    character_switcher.addComponent<ShowsProgressBar>();
}

static void make_wall(Entity& wall, vec2 pos, Color c = ui::color::brown) {
    entities::make_furniture(wall, DebugOptions{.name = strings::entity::WALL},
                             pos, c, c, true);

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
    // switch (this.type) {
    // case Type::DOUBLE_TEE: {
    // DrawCubeCustom(this.raw_position,                        //
    // this.size().x / 2,                        //
    // this.size().y,                            //
    // this.size().z,                            //
    // FrontFaceDirectionMap.at(face_direction),  //
    // this.face_color, this.base_color);
    // DrawCubeCustom(this.raw_position,                        //
    // this.size().x,                            //
    // this.size().y,                            //
    // this.size().z / 2,                        //
    // FrontFaceDirectionMap.at(face_direction),  //
    // this.face_color, this.base_color);
    // } break;
    // case Type::FULL: {
    // DrawCubeCustom(this.raw_position,                        //
    // this.size().x,                            //
    // this.size().y,                            //
    // this.size().z,                            //
    // FrontFaceDirectionMap.at(face_direction),  //
    // this.face_color, this.base_color);
    // } break;
    // case Type::HALF: {
    // DrawCubeCustom(this.raw_position,                        //
    // this.size().x,                            //
    // this.size().y,                            //
    // this.size().z / 2,                        //
    // FrontFaceDirectionMap.at(face_direction),  //
    // this.face_color, this.base_color);
    // } break;
    // case Type::CORNER: {
    // DrawCubeCustom(this.raw_position,                        //
    // this.size().x / 2,                        //
    // this.size().y,                            //
    // this.size().z,                            //
    // FrontFaceDirectionMap.at(face_direction),  //
    // this.face_color, this.base_color);
    // DrawCubeCustom(this.raw_position,                        //
    // this.size().x,                            //
    // this.size().y,                            //
    // this.size().z / 2,                        //
    // FrontFaceDirectionMap.at(face_direction),  //
    // this.face_color, this.base_color);
    // } break;
    // case Type::TEE: {
    // DrawCubeCustom(this.raw_position,                        //
    // this.size().x / 2,                        //
    // this.size().y,                            //
    // this.size().z,                            //
    // FrontFaceDirectionMap.at(face_direction),  //
    // this.face_color, this.base_color);
    // DrawCubeCustom(this.raw_position,                        //
    // this.size().x,                            //
    // this.size().y,                            //
    // this.size().z / 2,                        //
    // FrontFaceDirectionMap.at(face_direction),  //
    // this.face_color, this.base_color);
    // } break;
    // case Type::QUARTER:
    // break;
    // }
    // }
}

static void make_conveyer(Entity& conveyer, vec2 pos) {
    entities::make_furniture(conveyer,
                             DebugOptions{.name = strings::entity::CONVEYER},
                             pos, ui::color::blue, ui::color::blue);
    conveyer.get<CustomHeldItemPosition>().init(
        CustomHeldItemPosition::Positioner::Conveyer);
    conveyer.addComponent<ConveysHeldItem>();
    conveyer.addComponent<CanBeTakenFrom>();

    if (ENABLE_MODELS) {
        // TODO we probably want this and grabber to be 100% the same
        conveyer.get<ModelRenderer>().update(ModelInfo{
            .model_name = "conveyer",
            .size_scale = 3.f,
            .position_offset = vec3{0, 0, 0},
            .rotation_angle = 90.f,
        });
    }
}

static void make_grabber(Entity& grabber, vec2 pos) {
    entities::make_furniture(grabber,
                             DebugOptions{.name = strings::entity::GRABBER},
                             pos, ui::color::yellow, ui::color::yellow);

    grabber.get<CustomHeldItemPosition>().init(
        CustomHeldItemPosition::Positioner::Conveyer);
    grabber.addComponent<CanBeTakenFrom>();

    if (ENABLE_MODELS) {
        grabber.get<ModelRenderer>().update(ModelInfo{
            .model_name = "conveyer",
            .size_scale = 3.f,
            .position_offset = vec3{0, 0, 0},
            .rotation_angle = 90.f,
        });
    }
    grabber.addComponent<ConveysHeldItem>();
    grabber.addComponent<CanGrabFromOtherFurniture>();
}

static void make_register(Entity& reg, vec2 pos) {
    entities::make_furniture(reg,
                             DebugOptions{.name = strings::entity::REGISTER},
                             pos, ui::color::grey, ui::color::grey);
    reg.addComponent<HasWaitingQueue>();

    if (ENABLE_MODELS) {
        reg.get<ModelRenderer>().update(ModelInfo{
            .model_name = "register",
            .size_scale = 10.f,
            .position_offset = vec3{0, -TILESIZE / 2.f, 0},
        });
    }
}

static void make_itemcontainer(Entity& container, const DebugOptions& options,
                               vec2 pos, const std::string& item_type) {
    entities::make_furniture(container, options, pos, ui::color::white,
                             ui::color::white);
    container.addComponent<IsItemContainer>(item_type);
}

static void make_bagbox(Entity& container, vec2 pos) {
    entities::make_itemcontainer(container, {strings::entity::BAG_BOX}, pos,
                                 strings::item::BAG);

    if (ENABLE_MODELS) {
        container.get<ModelRenderer>().update(ModelInfo{
            .model_name = "box",
            .size_scale = 4.f,
            .position_offset = vec3{0, -TILESIZE / 2.f, 0},
        });

        container.addComponent<HasDynamicModelName>().init(
            "box", HasDynamicModelName::DynamicType::OpenClosed);
    }
}

static void make_medicine_cabinet(Entity& container, vec2 pos) {
    entities::make_itemcontainer(container, {strings::entity::MEDICINE_CABINET},
                                 pos, strings::item::PILL_BOTTLE);
    if (ENABLE_MODELS) {
        container.get<ModelRenderer>().update(ModelInfo{
            .model_name = "medicine_cabinet",
            .size_scale = 2.f,
            .position_offset = vec3{0, -TILESIZE / 2.f, 0},
        });
    }
}

static void make_pill_dispenser(Entity& container, vec2 pos) {
    entities::make_itemcontainer(container, {strings::entity::PILL_DISPENSER},
                                 pos, strings::item::PILL);
    if (ENABLE_MODELS) {
        container.get<ModelRenderer>().update(ModelInfo{
            .model_name = "crate",
            .size_scale = 2.f,
            .position_offset = vec3{0, -TILESIZE / 2.f, 0},
        });
    }
    container.addComponent<Indexer>((Subtype::PILL_END - Subtype::PILL_START));

    container.addComponent<HasWork>().init(
        [](Entity& owner, HasWork& hasWork, Entity&, float dt) {
            const float amt = 2.f;
            hasWork.increase_pct(amt * dt);
            if (hasWork.is_work_complete()) {
                owner.get<Indexer>().increment();
                hasWork.reset_pct();
            }
        });
    container.addComponent<ShowsProgressBar>();
    container.get<CustomHeldItemPosition>().init(
        CustomHeldItemPosition::Positioner::Table);
}

static void make_trigger_area(
    Entity& trigger_area, vec3 pos, float width, float height,
    std::string title = strings::entity::TRIGGER_AREA) {
    make_entity(trigger_area, {strings::entity::TRIGGER_AREA}, pos);

    trigger_area.get<Transform>().update_size({
        width,
        TILESIZE / 20.f,
        height,
    });

    trigger_area.addComponent<SimpleColoredBoxRenderer>().update(PINK, PINK);
    trigger_area.addComponent<IsTriggerArea>()
        .update_title(title)
        .update_max_entrants(1)
        .update_progress_max(2.f)
        .on_complete([]() {
            // TODO should be lobby only?
            // TODO only for host...
            GameState::s_toggle_to_planning();
        });
}

// This lives in entityhelper for now
static void spawn_customer(vec2 pos);

static void make_customer_spawner(Entity& customer_spawner, vec3 pos) {
    make_entity(customer_spawner, {strings::entity::CUSTOMER_SPAWNER}, pos);

    // TODO maybe one day add some kind of ui that shows when the next
    // person is coming? that migth be good to be part of the round
    // timer ui?
    customer_spawner.addComponent<SimpleColoredBoxRenderer>().update(PINK,
                                                                     PINK);
    const auto sfn = std::bind(&spawn_customer, std::placeholders::_1);
    customer_spawner
        .addComponent<IsSpawner>()  //
        .set_fn(sfn)
        .set_total(3)
        .set_time_between(2.f);
}

static void make_blender(Entity& blender, vec2 pos) {
    entities::make_furniture(blender,
                             DebugOptions{.name = strings::entity::BLENDER},
                             pos, ui::color::red, ui::color::yellow);
    blender.get<CanHoldItem>().update_held_by(IsItem::HeldBy::BLENDER);
}

// This will be a catch all for anything that just needs to get updated
static void make_sophie(Entity& sophie, vec3 pos) {
    make_entity(sophie, {strings::entity::SOPHIE}, pos);

    // TODO how long is a day?
    sophie.addComponent<HasTimer>(HasTimer::Renderer::Round, 90.f);
}

}  // namespace entities

namespace items {

static void make_item(Item& item, const DebugOptions& options,
                      vec2 p = {0, 0}) {
    make_entity(item, options, {p.x, 0, p.y});
    item.addComponent<IsItem>();
    // TODO Not everyone needs this but easier for now
    item.addComponent<CustomHeldItemPosition>().init(
        CustomHeldItemPosition::Positioner::ItemHoldingItem);
}

static void make_pill(Item& pill, vec2 pos, int index) {
    make_item(pill, {.name = strings::item::PILL}, pos);
    pill.addComponent<HasSubtype>(
        Subtype::PILL_START, Subtype::PILL_END,
        // TODO add a check to see if its past the end
        static_cast<Subtype>(Subtype::PILL_START + index));

    pill.addComponent<ModelRenderer>().update(ModelInfo{
        .model_name = "pill_red",
        // Scale needs to be a vec3 {3.f, 3.f, 12.f}
        .size_scale = 3.f,
        .position_offset = vec3{0, 0, 0},
        .rotation_angle = 0,
    });

    pill.addComponent<HasDynamicModelName>().init(
        "pill_red",  //
        HasDynamicModelName::DynamicType::Subtype,
        [](const Item& owner, const std::string base_name) -> std::string {
            if (owner.is_missing<HasSubtype>()) {
                log_warn(
                    "Generating a dynamic model name with a subtype, but your "
                    "entity doesnt have a subtype {}",
                    owner.get<DebugName>());
                return base_name;
            }
            const Subtype type = owner.get<HasSubtype>().get_type();
            switch (type) {
                case Subtype::PillRed:
                    return "pill_red";
                case Subtype::PillRedLong:
                    return "pill_redlong";
                case Subtype::PillBlue:
                    return "pill_blue";
                case Subtype::PillBlueLong:
                    return "pill_bluelong";
                default:
                    log_warn("Failed to get matching model for pill type: {}",
                             magic_enum::enum_name(type));
                    break;
            }
            return base_name;
        });

    pill.addComponent<HasWork>().init(
        [](Entity& owner, HasWork& hasWork, Entity& /* person */, float dt) {
            const IsItem& ii = owner.get<IsItem>();
            HasSubtype& hasSubtype = owner.get<HasSubtype>();

            if (ii.is_not_held_by(IsItem::HeldBy::BLENDER)) {
                hasWork.reset_pct();
                return;
            }

            const float amt = 1.5f;
            hasWork.increase_pct(amt * dt);
            if (hasWork.is_work_complete()) {
                hasWork.reset_pct();

                HasDynamicModelName& hDMN = owner.get<HasDynamicModelName>();
                ModelRenderer& renderer = owner.get<ModelRenderer>();

                hasSubtype.increment_type();
                renderer.update_model_name(hDMN.fetch(owner));
            }
        });
    pill.addComponent<ShowsProgressBar>();
}

static void make_pill_bottle(Item& pill_bottle, vec2 pos) {
    make_item(pill_bottle, {.name = strings::item::PILL_BOTTLE}, pos);

    pill_bottle.addComponent<ModelRenderer>().update(ModelInfo{
        .model_name = "pill_bottle",
        .size_scale = 1.5f,
        .position_offset = vec3{0, 0, 0},
        .rotation_angle = 0,
    });

    pill_bottle.addComponent<CanHoldItem>(IsItem::HeldBy::ITEM)
        .set_filter_fn([](const Item& item) {
            return check_name(item, strings::item::PILL);
        });
}

static void make_bag(Item& bag, vec2 pos) {
    make_item(bag, {.name = strings::item::BAG}, pos);

    bag.addComponent<ModelRenderer>().update(ModelInfo{
        .model_name = "bag",
        .size_scale = 0.25f,
        .position_offset = vec3{0, 0, 0},
        .rotation_angle = 0,
    });

    bag.addComponent<HasDynamicModelName>().init(
        "bag", HasDynamicModelName::DynamicType::EmptyFull,
        [](const Item& owner, const std::string base_name) -> std::string {
            if (owner.is_missing<CanHoldItem>()) {
                log_warn(
                    "Generating a dynamic model name with empty/full but your "
                    "entity cant hold items {}",
                    owner.get<DebugName>());
                return base_name;
            }
            const CanHoldItem& chi = owner.get<CanHoldItem>();
            return chi.empty() ? fmt::format("empty_{}", base_name) : base_name;
        });

    bag.addComponent<CanHoldItem>(IsItem::HeldBy::ITEM)
        .set_filter_fn([](const Item& item) {
            return check_name(item, strings::item::PILL_BOTTLE);
        });
}

static void make_item_type(Item& item, const std::string& type_name,  //
                           vec2 pos,                                  //
                           int index = -1                             //
) {
    log_info("generating new item {} of type {} at {}", item.id, type_name,
             pos);
    switch (hashString(type_name)) {
        case hashString(strings::item::PILL):
            return make_pill(item, pos, index);
        case hashString(strings::item::PILL_BOTTLE):
            return make_pill_bottle(item, pos);
        case hashString(strings::item::BAG):
            return make_bag(item, pos);
    }
    log_warn(
        "Trying to make item with item type {} but not handled in "
        "make_item_type()",
        type_name);
}

}  // namespace items
