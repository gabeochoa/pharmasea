

#pragma once

#include "components/has_rope_to_item.h"
#include "components/has_subtype.h"
#include "dataclass/ingredient.h"
#include "entity.h"
//
#include "components/adds_ingredient.h"
#include "components/can_be_ghost_player.h"
#include "components/can_be_held.h"
#include "components/can_be_highlighted.h"
#include "components/can_be_pushed.h"
#include "components/can_be_taken_from.h"
#include "components/can_grab_from_other_furniture.h"
#include "components/can_highlight_others.h"
#include "components/can_hold_furniture.h"
#include "components/can_hold_item.h"
#include "components/can_order_drink.h"
#include "components/can_perform_job.h"
#include "components/collects_user_input.h"
#include "components/conveys_held_item.h"
#include "components/custom_item_position.h"
#include "components/has_base_speed.h"
#include "components/has_client_id.h"
#include "components/has_dynamic_model_name.h"
#include "components/has_name.h"
#include "components/has_rope_to_item.h"
#include "components/has_speech_bubble.h"
#include "components/has_timer.h"
#include "components/has_waiting_queue.h"
#include "components/has_work.h"
#include "components/indexer.h"
#include "components/is_drink.h"
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
#include "strings.h"

static void register_all_components() {
    Entity* entity = new Entity();
    entity->addAll<  //
        DebugName, Transform, HasName,
        // Is
        IsRotatable, IsItem, IsSpawner, IsTriggerArea, IsSolid, IsItemContainer,
        IsDrink,
        //
        AddsIngredient, CanHoldItem, CanBeHighlighted, CanHighlightOthers,
        CanHoldFurniture, CanBeGhostPlayer, CanPerformJob, CanBePushed,
        CustomHeldItemPosition, CanBeHeld, CanGrabFromOtherFurniture,
        ConveysHeldItem, CanBeTakenFrom, UsesCharacterModel, Indexer,
        CanOrderDrink,
        //
        HasWaitingQueue, HasTimer, HasSubtype, HasSpeechBubble, HasWork,
        HasBaseSpeed, HasRopeToItem,
        // render
        ShowsProgressBar, ModelRenderer, HasDynamicModelName,
        SimpleColoredBoxRenderer
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

static void make_customer(Entity& customer, vec2 p, bool has_order = true) {
    make_aiperson(customer, DebugOptions{.name = strings::entity::CUSTOMER},
                  vec::to3(p));

    customer.addComponent<HasName>().update(get_random_name());

    // TODO for now, eventually move to customer spawner
    if (has_order) customer.addComponent<CanOrderDrink>();

    customer.addComponent<HasSpeechBubble>();

    customer.get<HasBaseSpeed>().update(10.f);
    customer.get<CanPerformJob>().update(WaitInQueue, Wandering);
}

typedef Entity Furniture;

// TODO This namespace should probably be "furniture::"
// or add the ones above into it
namespace furniture {
static void make_furniture(Entity& furniture, const DebugOptions& options,
                           vec2 pos, Color face = PINK, Color base = PINK,
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

static void process_table_working(Entity& table, HasWork& hasWork,
                                  Entity& player, float dt) {
    CanHoldItem& tableCHI = table.get<CanHoldItem>();
    if (tableCHI.empty()) return;

    if (!tableCHI.item()->has<HasWork>()) return;
    HasWork& itemHasWork = tableCHI.item()->get<HasWork>();
    if (itemHasWork.do_work)
        itemHasWork.do_work(*tableCHI.item(), hasWork, player, dt);

    return;
}

static void make_table(Entity& table, vec2 pos) {
    furniture::make_furniture(table,
                              DebugOptions{.name = strings::entity::TABLE}, pos,
                              ui::color::brown, ui::color::brown);

    table.get<CustomHeldItemPosition>().init(
        CustomHeldItemPosition::Positioner::Table);

    table.addComponent<HasWork>().init(std::bind(
        process_table_working, std::placeholders::_1, std::placeholders::_2,
        std::placeholders::_3, std::placeholders::_4));
    table.addComponent<ShowsProgressBar>();
}

static void make_character_switcher(Entity& character_switcher, vec2 pos) {
    furniture::make_furniture(
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
    furniture::make_furniture(wall, DebugOptions{.name = strings::entity::WALL},
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
    furniture::make_furniture(conveyer,
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
    furniture::make_furniture(grabber,
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

static void make_filtered_grabber(Entity& grabber, vec2 pos) {
    furniture::make_furniture(
        grabber, DebugOptions{.name = strings::entity::FILTERED_GRABBER}, pos,
        ui::color::yellow, ui::color::yellow);

    grabber.get<CustomHeldItemPosition>().init(
        CustomHeldItemPosition::Positioner::Conveyer);
    grabber.addComponent<CanBeTakenFrom>();

    if (ENABLE_MODELS) {
        grabber.get<ModelRenderer>().update(ModelInfo{
            .model_name = "filtered_grabber",
            .size_scale = 3.f,
            .position_offset = vec3{0, 0, 0},
            .rotation_angle = 90.f,
        });
    }
    grabber.addComponent<ConveysHeldItem>();
    grabber.addComponent<CanGrabFromOtherFurniture>();

    // Initially set empty filter
    grabber.get<CanHoldItem>().set_filter(EntityFilter().set_filter_strength(
        EntityFilter::FilterStrength::Suggestion));
}

static void make_register(Entity& reg, vec2 pos) {
    furniture::make_furniture(reg,
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
    furniture::make_furniture(container, options, pos, ui::color::white,
                              ui::color::white);
    container.addComponent<IsItemContainer>(item_type);
}

static void make_squirter(Entity& squ, vec2 pos) {
    furniture::make_furniture(squ, {strings::entity::SQUIRTER}, pos);
    if (ENABLE_MODELS) {
        squ.get<ModelRenderer>().update(ModelInfo{
            .model_name = "coffee_machine",
            .size_scale = 4.f,
            .position_offset = vec3{-TILESIZE / 2.f, 0, 0},
        });
    }

    squ.get<CustomHeldItemPosition>().init(
        CustomHeldItemPosition::Positioner::Table);

    // TODO change how progress bar works to support this
    // squ.addComponent<ShowsProgressBar>();
}

static void make_trash(Entity& trash, vec2 pos) {
    furniture::make_furniture(trash, {strings::entity::TRASH}, pos);
    if (ENABLE_MODELS) {
        trash.get<ModelRenderer>().update(ModelInfo{
            .model_name = "toilet",
            .size_scale = 2.f,
            .position_offset = vec3{0, 0, 0},
        });
    }
}

static void make_medicine_cabinet(Entity& container, vec2 pos) {
    furniture::make_itemcontainer(container,
                                  {strings::entity::MEDICINE_CABINET}, pos,
                                  strings::item::ALCOHOL);
    if (ENABLE_MODELS) {
        container.get<ModelRenderer>().update(ModelInfo{
            .model_name = "medicine_cabinet",
            .size_scale = 2.f,
            .position_offset = vec3{0, -TILESIZE / 2.f, 0},
        });
    }

    container.addComponent<Indexer>(ingredient::NUM_ALC);
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

static void make_fruit_basket(Entity& container, vec2 pos) {
    furniture::make_itemcontainer(container, {strings::entity::PILL_DISPENSER},
                                  pos, strings::item::LEMON);
    if (ENABLE_MODELS) {
        container.get<ModelRenderer>().update(ModelInfo{
            .model_name = "crate",
            .size_scale = 2.f,
            .position_offset = vec3{0, -TILESIZE / 2.f, 0},
        });
    }
    container.get<CustomHeldItemPosition>().init(
        CustomHeldItemPosition::Positioner::Table);

    // TODO right now lets just worry about lemon first we can come back and
    // handle other fruits later
    container.addComponent<Indexer>(1);
    //
    // container.addComponent<HasWork>().init(
    // [](Entity& owner, HasWork& hasWork, Entity&, float dt) {
    // const float amt = 2.f;
    // hasWork.increase_pct(amt * dt);
    // if (hasWork.is_work_complete()) {
    // owner.get<Indexer>().increment();
    // hasWork.reset_pct();
    // }
    // });
    // container.addComponent<ShowsProgressBar>();
}

static void make_cupboard(Entity& cupboard, vec2 pos) {
    furniture::make_itemcontainer(cupboard, {strings::entity::CUPBOARD}, pos,
                                  strings::item::DRINK);
    if (ENABLE_MODELS) {
        cupboard.get<ModelRenderer>().update(ModelInfo{
            .model_name = "box",
            .size_scale = 4.f,
            .position_offset = vec3{0, -TILESIZE / 2.f, 0},
        });
    }

    cupboard.addComponent<HasDynamicModelName>().init(
        "box", HasDynamicModelName::DynamicType::OpenClosed);

    cupboard.get<CustomHeldItemPosition>().init(
        CustomHeldItemPosition::Positioner::Table);
}

static void make_soda_machine(Entity& soda_machine, vec2 pos) {
    furniture::make_itemcontainer(
        soda_machine, DebugOptions{.name = strings::entity::BLENDER}, pos,
        strings::item::SODA_SPOUT);
    if (ENABLE_MODELS) {
        soda_machine.get<ModelRenderer>().update(ModelInfo{
            .model_name = "crate",
            .size_scale = 2.f,
            .position_offset = vec3{0, -TILESIZE / 2.f, 0},
        });
    }
    soda_machine.addComponent<HasRopeToItem>();
    soda_machine.get<IsItemContainer>().set_max_generations(1);
    soda_machine.get<CanHoldItem>()
        .update_held_by(IsItem::HeldBy::SODA_MACHINE)
        .set_filter(EntityFilter()
                        .set_name_filter(strings::item::SODA_SPOUT)
                        .set_filter_strength(
                            EntityFilter::FilterStrength::Requirement));
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
    furniture::make_furniture(blender,
                              DebugOptions{.name = strings::entity::BLENDER},
                              pos, ui::color::red, ui::color::yellow);
    blender.get<CanHoldItem>().update_held_by(IsItem::HeldBy::BLENDER);
    if (ENABLE_MODELS) {
        blender.get<ModelRenderer>().update(ModelInfo{
            .model_name = "blender",
            // Scale needs to be a vec3 {3.f, 3.f, 12.f}
            .size_scale = 7.f,
            .position_offset = vec3{-0.5f, 0, 0.5f},
            .rotation_angle = 0,
        });
    }
}

// This will be a catch all for anything that just needs to get updated
static void make_sophie(Entity& sophie, vec3 pos) {
    make_entity(sophie, {strings::entity::SOPHIE}, pos);

    // TODO how long is a day?
    sophie.addComponent<HasTimer>(HasTimer::Renderer::Round, 90.f);
}

}  // namespace furniture

namespace items {

static void make_item(Item& item, const DebugOptions& options,
                      vec2 p = {0, 0}) {
    make_entity(item, options, {p.x, 0, p.y});
    item.addComponent<IsItem>();
    // TODO Not everyone needs this but easier for now
    item.addComponent<CustomHeldItemPosition>().init(
        CustomHeldItemPosition::Positioner::ItemHoldingItem);
}

static void make_soda_spout(Item& soda_spout, vec2 pos) {
    make_item(soda_spout, {.name = strings::item::SODA_SPOUT}, pos);

    soda_spout.addComponent<ModelRenderer>().update(ModelInfo{
        .model_name = "banana",
        .size_scale = 2.0f,
        .position_offset = vec3{0, 0, 0},
        .rotation_angle = 0,
    });

    soda_spout.get<IsItem>().set_hb_filter(IsItem::HeldBy::SODA_MACHINE |
                                           IsItem::HeldBy::PLAYER);

    // TODO :SODAWAND: right now theres no good way to change what is selected
    // in the soda wand, id like to have it automatically figure it out but it
    // doesnt really work because we dont know what the player is trying to make
    // and so its easier if everything is soda
    soda_spout.addComponent<AddsIngredient>(
        [](Entity&) { return Ingredient::Soda; });
}

// Returns true if item was cleaned up
static bool _add_ingredient_to_drink_NO_VALIDATION(Entity& drink, Item& toadd) {
    IsDrink& isdrink = drink.get<IsDrink>();

    AddsIngredient& addsIG = toadd.get<AddsIngredient>();
    Ingredient ing = addsIG.get(toadd);

    isdrink.add_ingredient(ing);
    addsIG.decrement_uses();

    // We do == 0 because infinite is -1
    if (addsIG.uses_left() == 0) {
        toadd.cleanup = true;
        return true;
    }
    return false;
}

static void process_drink_working(Entity& drink, HasWork& hasWork,
                                  Entity& player, float dt) {
    auto _process_add_ingredient = [&]() {
        CanHoldItem& playerCHI = player.get<CanHoldItem>();
        // not holding anything
        if (playerCHI.empty()) return;
        std::shared_ptr<Item> item = playerCHI.const_item();
        // not holding item that adds ingredients
        if (item->is_missing<AddsIngredient>()) return;
        AddsIngredient& addsIG = item->get<AddsIngredient>();
        Ingredient ing = addsIG.get(*item);

        IsDrink& isdrink = drink.get<IsDrink>();
        // Already has the ingredient
        if (isdrink.has_ingredient(ing)) return;

        const float amt = 1.f;
        hasWork.increase_pct(amt * dt);
        if (hasWork.is_work_complete()) {
            hasWork.reset_pct();
            bool cleaned_up =
                _add_ingredient_to_drink_NO_VALIDATION(drink, *item);
            if (cleaned_up) playerCHI.update(nullptr);
        }
    };

    _process_add_ingredient();
}

static void make_alcohol(Item& alc, vec2 pos, int index) {
    make_item(alc, {.name = strings::item::ALCOHOL}, pos);

    alc.addComponent<ModelRenderer>().update(ModelInfo{
        .model_name = "bottle_a_brown",
        .size_scale = 1.0f,
        .position_offset = vec3{0, 0, 0},
        .rotation_angle = 0,
    });

    alc.addComponent<HasSubtype>(ingredient::ALC_START, ingredient::ALC_END,
                                 index);
    alc.addComponent<AddsIngredient>([](Entity& alcohol) {
           const HasSubtype& hst = alcohol.get<HasSubtype>();
           return get_ingredient_from_index(ingredient::ALC_START +
                                            hst.get_type_index());
       })
        // TODO need a place to put the bottles when they are half used and
        // track them
        .set_num_uses(1);

    alc.addComponent<HasDynamicModelName>().init(
        "bottle_a_brown", HasDynamicModelName::DynamicType::Subtype,
        [](const Item& owner, const std::string base_name) -> std::string {
            const HasSubtype& hst = owner.get<HasSubtype>();
            Ingredient bottle = get_ingredient_from_index(
                ingredient::ALC_START + hst.get_type_index());
            switch (bottle) {
                case Ingredient::Rum:
                    return "bottle_c_brown";
                    break;
                case Ingredient::Tequila:
                    return "bottle_a_green";
                    break;
                case Ingredient::Vodka:
                    return "bottle_b_brown";
                    break;
                case Ingredient::Whiskey:
                    return "bottle_b_green";
                    break;
                case Ingredient::Gin:
                    return "bottle_a_brown";
                    break;
                case Ingredient::Bitters:
                    return "bottle_a_brown";
                    break;
                default:
                    if (util::in_range(ingredient::ALC_START,
                                       ingredient::ALC_END,
                                       hst.get_type_index())) {
                        log_warn(
                            "You are trying to set an alcohol dynamic model "
                            "but forgot to setup the model name for {} {}",
                            hst.get_type_index(),
                            magic_enum::enum_name(bottle));
                    }
                    break;
            }
            return base_name;
        });
}

static void make_simple_syrup(Item& simple_syrup, vec2 pos) {
    make_item(simple_syrup, {.name = strings::item::SIMPLE_SYRUP}, pos);

    simple_syrup.addComponent<ModelRenderer>().update(ModelInfo{
        .model_name = "simple_syrup",
        .size_scale = 3.0f,
        .position_offset = vec3{0, 0, 0},
        .rotation_angle = 0,
    });

    simple_syrup
        .addComponent<AddsIngredient>(
            [](Entity&) { return Ingredient::SimpleSyrup; })
        .set_num_uses(-1);
}

static void make_lemon(Item& lemon, vec2 pos, int index) {
    make_item(lemon, {.name = strings::item::LEMON}, pos);

    // TODO i dont like that you have to remember to do +1 here
    lemon.addComponent<HasSubtype>(ingredient::LEMON_START,
                                   ingredient::LEMON_END + 1, index);

    lemon.addComponent<ModelRenderer>().update(ModelInfo{
        .model_name = "lemon",
        // Scale needs to be a vec3 {3.f, 3.f, 12.f}
        .size_scale = 2.f,
        .position_offset = vec3{0, 0, 0},
        .rotation_angle = 0,
    });

    lemon
        .addComponent<AddsIngredient>([](Entity& lemmy) {
            const HasSubtype& hst = lemmy.get<HasSubtype>();
            return get_ingredient_from_index(ingredient::LEMON_START +
                                             hst.get_type_index());
        })
        .set_num_uses(1);

    lemon.addComponent<HasDynamicModelName>().init(
        "lemon",  //
        HasDynamicModelName::DynamicType::Subtype,
        [](const Item& owner, const std::string base_name) -> std::string {
            if (owner.is_missing<HasSubtype>()) {
                log_warn(
                    "Generating a dynamic model name with a subtype, but your "
                    "entity doesnt have a subtype {}",
                    owner.get<DebugName>().name());
                return base_name;
            }
            const HasSubtype& hst = owner.get<HasSubtype>();
            Ingredient lemon_type = get_ingredient_from_index(
                ingredient::LEMON_START + hst.get_type_index());
            switch (lemon_type) {
                case Ingredient::Lemon:
                    return "lemon";
                case Ingredient::LemonJuice:
                    return "lemon_half";
                default:
                    log_warn("Failed to get matching model for lemon type: {}",
                             magic_enum::enum_name(lemon_type));
                    break;
            }
            return base_name;
        });

    lemon.addComponent<HasWork>().init(
        [](Entity& owner, HasWork& hasWork, Entity& /*person*/, float dt) {
            const IsItem& ii = owner.get<IsItem>();
            HasSubtype& hasSubtype = owner.get<HasSubtype>();
            Ingredient lemon_type = get_ingredient_from_index(
                ingredient::LEMON_START + hasSubtype.get_type_index());

            // Can only handle lemon -> juice right now
            if (lemon_type != Ingredient::Lemon) return;
            // TODO we shouldnt blindly increment type but in this case its
            // okay i guess

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
    lemon.addComponent<ShowsProgressBar>();
}

static void make_drink(Item& drink, vec2 pos) {
    make_item(drink, {.name = strings::item::DRINK}, pos);

    drink.addComponent<ModelRenderer>().update(ModelInfo{
        .model_name = "eggcup",
        .size_scale = 2.0f,
        .position_offset = vec3{0, 0, 0},
        .rotation_angle = 0,
    });

    drink.addComponent<IsDrink>();
    drink.addComponent<HasWork>().init(std::bind(
        process_drink_working, std::placeholders::_1, std::placeholders::_2,
        std::placeholders::_3, std::placeholders::_4));
    // TODO should this just be part of has work?
    drink.addComponent<ShowsProgressBar>();

    drink.addComponent<HasDynamicModelName>().init(
        "eggcup", HasDynamicModelName::DynamicType::Ingredients,
        [](const Item& owner, const std::string base_name) -> std::string {
            const IsDrink& isdrink = owner.get<IsDrink>();
            if (isdrink.matches_recipe(recipe::COKE)) return "soda_bottle";
            if (isdrink.matches_recipe(recipe::RUM_AND_COKE))
                return "soda_glass";
            if (isdrink.matches_recipe(recipe::G_AND_T)) return "soda_can";
            if (isdrink.matches_recipe(recipe::DAIQUIRI)) return "cocktail";
            return base_name;
        });
}

static void make_item_type(Item& item, const std::string& type_name,  //
                           vec2 pos,                                  //
                           int index = -1                             //
) {
    log_info("generating new item {} of type {} at {} subtype{}", item.id,
             type_name, pos, index);
    switch (hashString(type_name)) {
        case hashString(strings::item::SODA_SPOUT):
            return make_soda_spout(item, pos);
        case hashString(strings::item::ALCOHOL):
            return make_alcohol(item, pos, index);
        case hashString(strings::item::LEMON):
            return make_lemon(item, pos, index);
        case hashString(strings::item::DRINK):
            return make_drink(item, pos);
            // TODO add rope item
            // case hashString(strings::item::ROPE):
            // return make_rope(item, pos);
    }
    log_warn(
        "Trying to make item with item type {} but not handled in "
        "make_item_type()",
        type_name);
}

}  // namespace items
