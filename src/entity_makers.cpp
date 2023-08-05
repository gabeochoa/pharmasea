
#include "components/has_rope_to_item.h"
#include "components/has_subtype.h"
#include "components/is_pnumatic_pipe.h"
#include "components/is_progression_manager.h"
#include "dataclass/ingredient.h"
#include "engine/ui_color.h"
#include "entity.h"
#include "network/server.h"
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
#include "components/is_pnumatic_pipe.h"
#include "components/is_progression_manager.h"
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
#include "recipe_library.h"
#include "strings.h"

void register_all_components() {
    Entity* entity = new Entity();
    entity->addAll<  //
        DebugName, Transform, HasName,
        // Is
        IsRotatable, IsItem, IsSpawner, IsTriggerArea, IsSolid, IsItemContainer,
        IsDrink, IsPnumaticPipe, IsProgressionManager,
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

void add_entity_components(Entity& entity) { entity.addComponent<Transform>(); }

void add_person_components(Entity& person) {
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
        person.addComponent<ModelRenderer>(strings::model::CHARACTER_DUCK);
    } else {
        person.addComponent<SimpleColoredBoxRenderer>().update(RED, PINK);
    }

    person.addComponent<UsesCharacterModel>();
}

void make_entity(Entity& entity, const DebugOptions& options, vec3 p) {
    entity.addComponent<DebugName>().update(options.name);
    add_entity_components(entity);
    entity.get<Transform>().update(p);
}

void add_player_components(Entity& player) {
    player.addComponent<CanHighlightOthers>();
    player.addComponent<CanHoldFurniture>();
    player.get<HasBaseSpeed>().update(7.5f);
    player.addComponent<HasName>();
    player.addComponent<HasClientID>();
}

void make_remote_player(Entity& remote_player, vec3 pos) {
    make_entity(remote_player, {.name = strings::entity::REMOTE_PLAYER}, pos);
    add_person_components(remote_player);
    add_player_components(remote_player);
}

void move_player_SERVER_ONLY(Entity& entity, vec3 position) {
    if (!is_server()) {
        log_warn(
            "you are calling a server only function from a client context, "
            "this is best case a no-op and worst case a visual desync");
    }

    Transform& transform = entity.get<Transform>();
    transform.update(position);

    // TODO if we have multiple local players then we need to specify which here

    network::Server* server = GLOBALS.get_ptr<network::Server>("server");

    int client_id = server->get_client_id_for_entity(entity);
    if (client_id == -1) {
        log_warn("Tried to find a client id for entity but didnt find one");
        return;
    }

    server->send_player_location_packet(
        client_id, position, static_cast<int>(transform.face_direction()),
        entity.get<HasName>().name());
}

void update_player_remotely(Entity& entity, float* location,
                            const std::string& username, int facing_direction) {
    entity.get<HasName>().update(username);

    Transform& transform = entity.get<Transform>();

    transform.update(vec3{location[0], location[1], location[2]});
    transform.update_face_direction(
        static_cast<Transform::FrontFaceDirection>(facing_direction));
}

void update_player_rare_remotely(Entity& entity, int model_index,
                                 long long last_ping) {
    if (entity.is_missing_any<UsesCharacterModel, ModelRenderer, HasClientID>())
        return;
    UsesCharacterModel& ucm = entity.get<UsesCharacterModel>();
    ModelRenderer& renderer = entity.get<ModelRenderer>();

    ucm.update_index_CLIENT_ONLY(model_index);

    entity.get<HasClientID>().update_ping(last_ping);

    // TODO this should be the same as all other renderer updates for players
    renderer.update_model_name(ucm.fetch_model_name());
}

void make_player(Entity& player, vec3 p) {
    make_entity(player, {.name = strings::entity::PLAYER}, p);
    add_person_components(player);
    add_player_components(player);

    // note: these are added to some remote players
    // ie the one the client is controlling
    player.addComponent<CollectsUserInput>();
    player.addComponent<CanBeGhostPlayer>();

    player.addComponent<RespondsToUserInput>();
}

void make_aiperson(Entity& person, const DebugOptions& options, vec3 p) {
    make_entity(person, options, p);
    add_person_components(person);

    person.addComponent<CanPerformJob>().update(Wandering, Wandering);
}

// TODO This namespace should probably be "furniture::"
// or add the ones above into it
namespace furniture {
void make_furniture(Entity& furniture, const DebugOptions& options, vec2 pos,
                    Color face, Color base, bool is_static) {
    make_entity(furniture, options);

    furniture.get<Transform>().init({pos.x, 0, pos.y},
                                    {TILESIZE, TILESIZE, TILESIZE});
    furniture.addComponent<IsSolid>();

    // For renderers we prioritize the ModelRenderer and will fall back if we
    // need
    furniture.addComponent<SimpleColoredBoxRenderer>().update(face, base);
    if (ENABLE_MODELS) {
        furniture.addComponent<ModelRenderer>(options.name);
    }

    // we need to add it to set a default, so its here
    furniture.addComponent<CustomHeldItemPosition>().init(
        CustomHeldItemPosition::Positioner::Table);

    // Walls should not have these
    if (!is_static) {
        furniture.addComponent<IsRotatable>();
        furniture.addComponent<CanHoldItem>(IsItem::HeldBy::UNKNOWN_FURNITURE);
        // These two are the heavy ones
        furniture.addComponent<CanBeHeld>();
        furniture.addComponent<CanBeHighlighted>();
    }
}

void process_table_working(Entity& table, HasWork& hasWork, Entity& player,
                           float dt) {
    CanHoldItem& tableCHI = table.get<CanHoldItem>();
    if (tableCHI.empty()) return;

    if (!tableCHI.item()->has<HasWork>()) return;

    // TODO add comment on why we have to run the "itemHasWork" and not
    // hasWork.call()
    HasWork& itemHasWork = tableCHI.item()->get<HasWork>();
    itemHasWork.call(hasWork, *tableCHI.item(), player, dt);

    return;
}

void make_table(Entity& table, vec2 pos) {
    furniture::make_furniture(table,
                              DebugOptions{.name = strings::entity::TABLE}, pos,
                              ui::color::brown, ui::color::brown);

    table.addComponent<HasWork>().init(std::bind(
        process_table_working, std::placeholders::_1, std::placeholders::_2,
        std::placeholders::_3, std::placeholders::_4));
    table.addComponent<ShowsProgressBar>();
}

void make_character_switcher(Entity& character_switcher, vec2 pos) {
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

void make_map_randomizer(Entity& map_randomizer, vec2 pos) {
    furniture::make_furniture(
        map_randomizer, DebugOptions{.name = strings::entity::MAP_RANDOMIZER},
        pos, ui::color::baby_blue, ui::color::baby_pink);

    map_randomizer.addComponent<HasWork>().init(
        [](Entity&, HasWork& hasWork, Entity&, float dt) {
            const float amt = 1.5f;
            hasWork.increase_pct(amt * dt);
            if (hasWork.is_work_complete()) {
                hasWork.reset_pct();
                if (!is_server()) {
                    log_warn(
                        "you are calling a server only function from a client "
                        "context, this is probably gonna crash");
                }

                network::Server* server =
                    GLOBALS.get_ptr<network::Server>("server");
                server->get_map_SERVER_ONLY()->update_seed(randString(10));
            }
        });

    map_randomizer.addComponent<ShowsProgressBar>();
}

void make_wall(Entity& wall, vec2 pos, Color c) {
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
    // (this.raw_position,                        //
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

void make_conveyer(Entity& conveyer, vec2 pos, const DebugOptions& options) {
    furniture::make_furniture(conveyer, options, pos, ui::color::blue,
                              ui::color::blue);
    conveyer.get<CustomHeldItemPosition>().init(
        CustomHeldItemPosition::Positioner::Conveyer);
    conveyer.addComponent<ConveysHeldItem>();
    conveyer.addComponent<CanBeTakenFrom>();
}

void make_grabber(Entity& grabber, vec2 pos, const DebugOptions& options) {
    furniture::make_conveyer(grabber, pos, options);
    grabber.addComponent<CanGrabFromOtherFurniture>();
}

void make_filtered_grabber(Entity& grabber, vec2 pos,
                           const DebugOptions& options) {
    furniture::make_grabber(grabber, pos, options);
    // Initially set empty filter
    grabber.get<CanHoldItem>().set_filter(
        EntityFilter()
            .set_enabled_flags(EntityFilter::FilterDatumType::Name |
                               EntityFilter::FilterDatumType::Subtype)
            .set_filter_strength(EntityFilter::FilterStrength::Suggestion));
}

void make_register(Entity& reg, vec2 pos) {
    furniture::make_furniture(reg,
                              DebugOptions{.name = strings::entity::REGISTER},
                              pos, ui::color::grey, ui::color::grey);
    reg.addComponent<HasWaitingQueue>();
}

void make_itemcontainer(Entity& container, const DebugOptions& options,
                        vec2 pos, const std::string& item_type) {
    furniture::make_furniture(container, options, pos, ui::color::white,
                              ui::color::white);
    container.addComponent<IsItemContainer>(item_type);
}

void make_squirter(Entity& squ, vec2 pos) {
    furniture::make_furniture(squ, {strings::entity::SQUIRTER}, pos);
    // TODO change how progress bar works to support this
    // squ.addComponent<ShowsProgressBar>();
}

void make_trash(Entity& trash, vec2 pos) {
    furniture::make_furniture(trash, {strings::entity::TRASH}, pos);
}

void make_pnumatic_pipe(Entity& pnumatic, vec2 pos) {
    furniture::make_conveyer(pnumatic, pos, {strings::entity::PNUMATIC_PIPE});

    pnumatic.addComponent<IsPnumaticPipe>();
    pnumatic.get<CustomHeldItemPosition>().init(
        CustomHeldItemPosition::Positioner::PnumaticPipe);
}

void make_medicine_cabinet(Entity& container, vec2 pos) {
    furniture::make_itemcontainer(container,
                                  {strings::entity::MEDICINE_CABINET}, pos,
                                  strings::item::ALCOHOL);
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
}

void make_fruit_basket(Entity& container, vec2 pos) {
    furniture::make_itemcontainer(container, {strings::entity::PILL_DISPENSER},
                                  pos, strings::item::LEMON);

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

void make_cupboard(Entity& cupboard, vec2 pos) {
    furniture::make_itemcontainer(cupboard, {strings::entity::CUPBOARD}, pos,
                                  strings::item::DRINK);
    cupboard.addComponent<HasDynamicModelName>().init(
        strings::entity::CUPBOARD,
        HasDynamicModelName::DynamicType::OpenClosed);
}

void make_soda_machine(Entity& soda_machine, vec2 pos) {
    furniture::make_itemcontainer(
        soda_machine, DebugOptions{.name = strings::entity::SODA_MACHINE}, pos,
        strings::item::SODA_SPOUT);
    soda_machine.addComponent<HasRopeToItem>();
    soda_machine.get<IsItemContainer>().set_max_generations(1);
    soda_machine.get<CanHoldItem>()
        .update_held_by(IsItem::HeldBy::SODA_MACHINE)
        .set_filter(
            EntityFilter()
                .set_enabled_flags(EntityFilter::FilterDatumType::Name)
                .set_filter_value_for_type(EntityFilter::FilterDatumType::Name,
                                           strings::item::SODA_SPOUT)
                .set_filter_strength(
                    EntityFilter::FilterStrength::Requirement));
}

void make_mop_holder(Entity& mop_holder, vec2 pos) {
    furniture::make_itemcontainer(
        mop_holder, DebugOptions{.name = strings::entity::MOP_HOLDER}, pos,
        strings::item::MOP);
    mop_holder.get<IsItemContainer>().set_max_generations(1);
    mop_holder.get<CanHoldItem>()
        .update_held_by(IsItem::HeldBy::MOP_HOLDER)
        .set_filter(
            EntityFilter()
                .set_enabled_flags(EntityFilter::FilterDatumType::Name)
                .set_filter_value_for_type(EntityFilter::FilterDatumType::Name,
                                           strings::item::MOP)
                .set_filter_strength(
                    EntityFilter::FilterStrength::Requirement));
}

void make_trigger_area(Entity& trigger_area, vec3 pos, float width,
                       float height, const std::string& title) {
    make_entity(trigger_area, {strings::entity::TRIGGER_AREA}, pos);

    trigger_area.get<Transform>().update_size({
        width,
        TILESIZE / 20.f,
        height,
    });

    trigger_area.addComponent<SimpleColoredBoxRenderer>().update(PINK, PINK);
    trigger_area.addComponent<IsTriggerArea>()
        .update_title(title)
        // TODO we dont need to hard code these, why not just default to these
        .update_max_entrants(1)
        .update_progress_max(2.f)
        .on_complete([](const Entities& all) {
            // TODO should be lobby only?
            // TODO only for host...
            GameState::s_toggle_to_planning();

            for (std::shared_ptr<Entity> e : all) {
                if (!e) continue;
                if (!check_name(*e, strings::entity::PLAYER)) continue;
                move_player_SERVER_ONLY(*e, {0, 0, 0});
            }
        });
}

void make_blender(Entity& blender, vec2 pos) {
    furniture::make_furniture(blender,
                              DebugOptions{.name = strings::entity::BLENDER},
                              pos, ui::color::red, ui::color::yellow);
    blender.get<CanHoldItem>().update_held_by(IsItem::HeldBy::BLENDER);
}

// This will be a catch all for anything that just needs to get updated
void make_sophie(Entity& sophie, vec3 pos) {
    make_entity(sophie, {strings::entity::SOPHIE}, pos);

    sophie.addComponent<HasTimer>(HasTimer::Renderer::Round,
                                  round_settings::ROUND_LENGTH_S);
    sophie.addComponent<IsProgressionManager>();
}

void make_vomit(Entity& vomit, vec2 pos) {
    make_entity(vomit, {.name = strings::entity::VOMIT});

    vomit.get<Transform>().init({pos.x, 0, pos.y},
                                {TILESIZE, TILESIZE, TILESIZE});
    if (ENABLE_MODELS) {
        vomit.addComponent<ModelRenderer>(strings::entity::VOMIT);
    }

    vomit.addComponent<CanBeHighlighted>();

    // TODO please just add this to has work or something cmon
    vomit.addComponent<ShowsProgressBar>();

    vomit.addComponent<HasWork>().init(
        [](Entity& vom, HasWork& hasWork, const Entity& player, float dt) {
            const CanHoldItem& playerCHI = player.get<CanHoldItem>();
            // not holding anything
            if (playerCHI.empty()) return;
            std::shared_ptr<Item> item = playerCHI.const_item();
            // Has to be holding mop
            if (!check_name(*item, strings::item::MOP)) return;

            const float amt = 1.f;
            hasWork.increase_pct(amt * dt);
            if (hasWork.is_work_complete()) {
                hasWork.reset_pct();
                // Clean it up
                vom.cleanup = true;
            }
        });
}

}  // namespace furniture

namespace items {

void make_item(Item& item, const DebugOptions& options, vec2 p) {
    make_entity(item, options, {p.x, 0, p.y});
    item.addComponent<IsItem>();
    // TODO Not everyone needs this but easier for now
    item.addComponent<CustomHeldItemPosition>().init(
        CustomHeldItemPosition::Positioner::ItemHoldingItem);
}

void make_soda_spout(Item& soda_spout, vec2 pos) {
    make_item(soda_spout, {.name = strings::item::SODA_SPOUT}, pos);

    soda_spout.addComponent<ModelRenderer>(strings::item::SODA_SPOUT);

    soda_spout.get<IsItem>().set_hb_filter(IsItem::HeldBy::SODA_MACHINE |
                                           IsItem::HeldBy::PLAYER);

    // TODO :SODAWAND: right now theres no good way to change what is selected
    // in the soda wand, id like to have it automatically figure it out but it
    // doesnt really work because we dont know what the player is trying to make
    // and so its easier if everything is soda
    soda_spout.addComponent<AddsIngredient>(
        [](Entity&) { return Ingredient::Soda; });
}

void make_mop(Item& mop, vec2 pos) {
    make_item(mop, {.name = strings::item::MOP}, pos);

    mop.addComponent<ModelRenderer>(strings::item::MOP);

    mop.get<IsItem>().set_hb_filter(IsItem::HeldBy::MOP_HOLDER |
                                    IsItem::HeldBy::PLAYER);
}

// Returns true if item was cleaned up
bool _add_ingredient_to_drink_NO_VALIDATION(Entity& drink, Item& toadd) {
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

void process_drink_working(Entity& drink, HasWork& hasWork, Entity& player,
                           float dt) {
    auto _process_add_ingredient = [&]() {
        CanHoldItem& playerCHI = player.get<CanHoldItem>();
        // not holding anything
        if (playerCHI.empty()) return;
        std::shared_ptr<Item> item = playerCHI.const_item();
        // not holding item that adds ingredients
        if (item->is_missing<AddsIngredient>()) return;
        const AddsIngredient& addsIG = item->get<AddsIngredient>();
        Ingredient ing = addsIG.get(*item);

        const IsDrink& isdrink = drink.get<IsDrink>();
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

void make_alcohol(Item& alc, vec2 pos, int index) {
    make_item(alc, {.name = strings::item::ALCOHOL}, pos);

    alc.addComponent<ModelRenderer>(strings::item::ALCOHOL);

    alc.addComponent<HasSubtype>(ingredient::ALC_START, ingredient::ALC_END,
                                 index);
    alc.addComponent<AddsIngredient>([](const Entity& alcohol) {
           const HasSubtype& hst = alcohol.get<HasSubtype>();
           return get_ingredient_from_index(ingredient::ALC_START +
                                            hst.get_type_index());
       })
        // TODO need a place to put the bottles when they are half used and
        // track them
        .set_num_uses(1);

    alc.addComponent<HasDynamicModelName>().init(
        strings::item::ALCOHOL, HasDynamicModelName::DynamicType::Subtype,
        [](const Item& owner, const std::string&) -> std::string {
            const HasSubtype& hst = owner.get<HasSubtype>();
            Ingredient bottle = get_ingredient_from_index(
                ingredient::ALC_START + hst.get_type_index());
            return util::toLowerCase(magic_enum::enum_name<Ingredient>(bottle));
        });
}

void make_simple_syrup(Item& simple_syrup, vec2 pos) {
    make_item(simple_syrup, {.name = strings::item::SIMPLE_SYRUP}, pos);

    simple_syrup.addComponent<ModelRenderer>(strings::item::SIMPLE_SYRUP);
    simple_syrup
        .addComponent<AddsIngredient>(
            [](Entity&) { return Ingredient::SimpleSyrup; })
        .set_num_uses(-1);
}

void make_lemon(Item& lemon, vec2 pos, int index) {
    make_item(lemon, {.name = strings::item::LEMON}, pos);

    // TODO i dont like that you have to remember to do +1 here
    lemon.addComponent<HasSubtype>(ingredient::LEMON_START,
                                   ingredient::LEMON_END + 1, index);

    // Scale needs to be a vec3 {3.f, 3.f, 12.f}
    lemon.addComponent<ModelRenderer>(strings::item::LEMON);

    lemon
        .addComponent<AddsIngredient>([](const Entity& lemmy) {
            const HasSubtype& hst = lemmy.get<HasSubtype>();
            return get_ingredient_from_index(ingredient::LEMON_START +
                                             hst.get_type_index());
        })
        .set_num_uses(1);

    lemon.addComponent<HasDynamicModelName>().init(
        strings::item::LEMON,  //
        HasDynamicModelName::DynamicType::Subtype,
        [](const Item& owner, const std::string& base_name) -> std::string {
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
                    return strings::item::LEMON;
                case Ingredient::LemonJuice:
                    return strings::model::LEMON_HALF;
                default:
                    log_warn("Failed to get matching model for lemon type: {}",
                             magic_enum::enum_name(lemon_type));
                    break;
            }
            return base_name;
        });

    lemon.addComponent<HasWork>().init([](Entity& owner, HasWork& hasWork,
                                          Entity& /*person*/, float dt) {
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

            const HasDynamicModelName& hDMN = owner.get<HasDynamicModelName>();
            ModelRenderer& renderer = owner.get<ModelRenderer>();

            hasSubtype.increment_type();
            renderer.update_model_name(hDMN.fetch(owner));
        }
    });
    lemon.addComponent<ShowsProgressBar>();
}

void make_drink(Item& drink, vec2 pos) {
    make_item(drink, {.name = strings::item::DRINK}, pos);

    drink.addComponent<ModelRenderer>(strings::item::DRINK);

    drink.addComponent<IsDrink>();
    drink.addComponent<HasWork>().init(std::bind(
        process_drink_working, std::placeholders::_1, std::placeholders::_2,
        std::placeholders::_3, std::placeholders::_4));
    // TODO should this just be part of has work?
    drink.addComponent<ShowsProgressBar>();

    drink.addComponent<HasDynamicModelName>().init(
        strings::item::DRINK, HasDynamicModelName::DynamicType::Ingredients,
        [](const Item& owner, const std::string&) -> std::string {
            const IsDrink& isdrink = owner.get<IsDrink>();
            constexpr auto drinks = magic_enum::enum_values<Drink>();
            for (Drink d : drinks) {
                if (isdrink.matches_recipe(d))
                    return get_model_name_for_drink(d);
            }
            return strings::item::DRINK;
        });
}

void make_item_type(Item& item, const std::string& type_name,  //
                    vec2 pos,                                  //
                    int index                                  //
) {
    // log_info("generating new item {} of type {} at {} subtype{}", item.id,
    // type_name, pos, index);
    switch (hashString(type_name)) {
        case hashString(strings::item::SODA_SPOUT):
            return make_soda_spout(item, pos);
        case hashString(strings::item::ALCOHOL):
            return make_alcohol(item, pos, index);
        case hashString(strings::item::LEMON):
            return make_lemon(item, pos, index);
        case hashString(strings::item::DRINK):
            return make_drink(item, pos);
        case hashString(strings::item::MOP):
            return make_mop(item, pos);
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

void make_customer(Entity& customer, vec2 p, bool has_order) {
    make_aiperson(customer, DebugOptions{.name = strings::entity::CUSTOMER},
                  vec::to3(p));

    customer.addComponent<HasName>().update(get_random_name());

    // TODO for now, eventually move to customer spawner
    if (has_order) customer.addComponent<CanOrderDrink>();

    customer.addComponent<HasSpeechBubble>();

    customer.get<HasBaseSpeed>().update(5.f);
    customer.get<CanPerformJob>().update(WaitInQueue, Wandering);

    customer
        .addComponent<IsSpawner>()  //
        .set_fn(&furniture::make_vomit)
        .set_validation_fn([](const Entity& entity, vec2) {
            const CanOrderDrink& cod = entity.get<CanOrderDrink>();
            // not vomiting since didnt have anything to drink yet
            if (cod.num_orders_had <= 0) return false;
            return true;
        })
        // check if there is already vomit in that spot
        .enable_prevent_duplicates()
        // TODO dynamically set these based on num drinks
        .set_total(2)
        .set_time_between(5.f);
}

namespace furniture {
void make_customer_spawner(Entity& customer_spawner, vec3 pos) {
    make_entity(customer_spawner, {strings::entity::CUSTOMER_SPAWNER}, pos);

    // TODO maybe one day add some kind of ui that shows when the next
    // person is coming? that migth be good to be part of the round
    // timer ui?
    customer_spawner.addComponent<SimpleColoredBoxRenderer>().update(PINK,
                                                                     PINK);
    const auto sfn = std::bind(&make_customer, std::placeholders::_1,
                               std::placeholders::_2, true);
    customer_spawner
        .addComponent<IsSpawner>()  //
        .set_fn(sfn)
        .set_total(round_settings::NUM_CUSTOMERS)
        .set_time_between(round_settings::TIME_BETWEEN_CUSTOMERS_S);
}

}  // namespace furniture