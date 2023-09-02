
#include "entity_makers.h"

#include <ranges>

#include "components/has_rope_to_item.h"
#include "components/has_subtype.h"
#include "components/is_pnumatic_pipe.h"
#include "components/is_progression_manager.h"
#include "dataclass/ingredient.h"
#include "engine/ui/color.h"
#include "engine/util.h"
#include "entity.h"
#include "entity_type.h"
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
#include "components/has_speech_bubble.h"
#include "components/has_timer.h"
#include "components/has_waiting_queue.h"
#include "components/has_work.h"
#include "components/indexer.h"
#include "components/is_drink.h"
#include "components/is_item_container.h"
#include "components/is_rotatable.h"
#include "components/is_solid.h"
#include "components/is_spawner.h"
#include "components/is_trigger_area.h"
#include "components/model_renderer.h"
#include "components/responds_to_user_input.h"
#include "components/shows_progress_bar.h"
#include "components/simple_colored_box_renderer.h"
#include "components/transform.h"
#include "components/uses_character_model.h"
#include "dataclass/names.h"
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

void add_person_components(Entity& person, DebugOptions options = {}) {
    // TODO idk why but you spawn under the ground without this
    person.get<Transform>().update_y(0);
    float size_multiplier = 0.75f;
    person.get<Transform>().update_size(vec3{
        TILESIZE * size_multiplier,
        TILESIZE * size_multiplier,
        TILESIZE * size_multiplier,
    });

    person.addComponent<CanBeTakenFrom>().update(false);
    person.addComponent<CanHoldItem>(EntityType::Player);
    person.addComponent<CanBePushed>();

    person.addComponent<HasBaseSpeed>().update(10.f);

    if (ENABLE_MODELS) {
        person.addComponent<ModelRenderer>(strings::model::CHARACTER_DUCK);
    } else {
        person.addComponent<SimpleColoredBoxRenderer>().update(RED, PINK);
    }

    if (options.enableCharacterModel) {
        person.addComponent<UsesCharacterModel>();
    }
}

void make_entity(Entity& entity, const DebugOptions& options,
                 vec3 p = {-2, 0, -2}) {
    entity.addComponent<DebugName>().update(options.type);
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
    make_entity(remote_player, {.type = EntityType::RemotePlayer}, pos);
    add_person_components(remote_player);
    add_player_components(remote_player);
}

void update_player_remotely(Entity& entity, float* location,
                            const std::string& username, float facing) {
    entity.get<HasName>().update(username);

    Transform& transform = entity.get<Transform>();
    vec3 new_pos = vec3{location[0], location[1], location[2]};

    transform.update(new_pos);
    transform.update_face_direction(facing);
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
    make_entity(player, {.type = EntityType::Player}, p);
    add_person_components(player);
    add_player_components(player);

    // note: these are added to some remote players
    // ie the one the client is controlling
    player.addComponent<CollectsUserInput>();
    player.addComponent<IsSolid>();

    // This one is only added to local players
    // player.addComponent<CanBeGhostPlayer>();

    player.addComponent<RespondsToUserInput>();
}

void make_aiperson(Entity& person, const DebugOptions& options, vec3 p) {
    make_entity(person, options, p);
    add_person_components(person, options);

    person.addComponent<CanPerformJob>().update(Wandering, Wandering);
}

void make_mop_buddy(Entity& mop_buddy, vec2 pos) {
    make_aiperson(mop_buddy,
                  DebugOptions{.type = EntityType::MopBuddy,
                               .enableCharacterModel = false},
                  vec::to3(pos));

    mop_buddy.get<ModelRenderer>().update_model_name(
        util::convertToSnakeCase(EntityType::MopBuddy));

    mop_buddy.get<HasBaseSpeed>().update(1.5f);
    mop_buddy.get<CanPerformJob>().update(Mopping, Mopping);
    mop_buddy.addComponent<IsItem>().set_hb_filter(EntityType::Player);
}

// TODO This namespace should probably be "furniture::"
// or add the ones above into it
namespace furniture {
void make_furniture(Entity& furniture, const DebugOptions& options, vec2 pos,
                    Color face = PINK, Color base = PINK,
                    bool is_static = false) {
    make_entity(furniture, options);

    furniture.get<Transform>().init({pos.x, 0, pos.y},
                                    {TILESIZE, TILESIZE, TILESIZE});
    furniture.addComponent<IsSolid>();

    // For renderers we prioritize the ModelRenderer and will fall back if we
    // need
    furniture.addComponent<SimpleColoredBoxRenderer>().update(face, base);
    if (ENABLE_MODELS) {
        furniture.addComponent<ModelRenderer>(options.type);
    }

    // we need to add it to set a default, so its here
    furniture.addComponent<CustomHeldItemPosition>().init(
        CustomHeldItemPosition::Positioner::Table);

    // Walls should not have these
    if (!is_static) {
        furniture.addComponent<IsRotatable>();
        furniture.addComponent<CanHoldItem>(options.type);
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
    furniture::make_furniture(table, DebugOptions{.type = EntityType::Table},
                              pos, ui::color::brown, ui::color::brown);

    table.addComponent<HasWork>().init(std::bind(
        process_table_working, std::placeholders::_1, std::placeholders::_2,
        std::placeholders::_3, std::placeholders::_4));
    table.addComponent<ShowsProgressBar>();
}

void make_character_switcher(Entity& character_switcher, vec2 pos) {
    furniture::make_furniture(
        character_switcher, DebugOptions{.type = EntityType::CharacterSwitcher},
        pos, ui::color::green, ui::color::yellow);

    character_switcher.addComponent<HasWork>().init(
        [](Entity&, HasWork& hasWork, Entity& person, float dt) {
            if (!GameState::get().is_lobby_like()) return;
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
    furniture::make_furniture(map_randomizer,
                              DebugOptions{.type = EntityType::MapRandomizer},
                              pos, ui::color::baby_blue, ui::color::baby_pink);

    map_randomizer.addComponent<HasName>().update("");

    map_randomizer.get<CanBeHighlighted>().set_on_change(
        [](Entity&, bool is_highlighted) {
            if (!is_server()) {
                log_warn(
                    "you are calling a server only function from a client "
                    "context, this is probably gonna crash");
            }
            network::Server* server =
                GLOBALS.get_ptr<network::Server>("server");
            server->get_map_SERVER_ONLY()->showMinimap = is_highlighted;
        });

    map_randomizer.addComponent<HasWork>().init([](Entity& randomizer,
                                                   HasWork& hasWork, Entity&,
                                                   float dt) {
        if (GameState::get().is_not(game::State::Lobby)) return;
        if (!is_server()) {
            log_warn(
                "you are calling a server only function from a client "
                "context, this is probably gonna crash");
        }
        network::Server* server = GLOBALS.get_ptr<network::Server>("server");

        const float amt = 1.5f;
        hasWork.increase_pct(amt * dt);
        if (hasWork.is_work_complete()) {
            hasWork.reset_pct();

            const auto name = get_random_name_rot13();
            server->get_map_SERVER_ONLY()->update_seed(name);
        }
        randomizer.get<HasName>().update(server->get_map_SERVER_ONLY()->seed);
    });

    map_randomizer.addComponent<ShowsProgressBar>();
}

void make_fast_forward(Entity& fast_forward, vec2 pos) {
    furniture::make_furniture(fast_forward,
                              DebugOptions{.type = EntityType::FastForward},
                              pos, ui::color::apricot, ui::color::apricot);

    fast_forward.addComponent<HasName>().update("Fast-Forward Day");

    fast_forward.addComponent<HasWork>().init(
        [](Entity&, HasWork& hasWork, Entity&, float dt) {
            // TODO why does this not work
            // std::shared_ptr<Entity> sophie =
            // (EntityHelper::getAllWithName(EntityType::SOPHIE))[0];

            const float amt = 5.f;

            // TODO i dont think the spawner is working correctly

            for (auto e : SystemManager::get().oldAll) {
                if (check_type(*e, EntityType::Sophie)) {
                    e->get<HasTimer>().pass_time(amt * dt);
                }
                if (check_type(*e, EntityType::CustomerSpawner)) {
                    e->get<IsSpawner>().pass_time(amt * dt);
                }
            }

            hasWork.increase_pct(dt * 0.05f);
            if (hasWork.is_work_complete()) {
                hasWork.reset_pct();
            }
        });

    fast_forward.addComponent<ShowsProgressBar>();
}

void make_wall(Entity& wall, vec2 pos, Color c) {
    furniture::make_furniture(wall, DebugOptions{.type = EntityType::Wall}, pos,
                              c, c, true);

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
}

void make_conveyer(Entity& conveyer, vec2 pos,
                   const DebugOptions& options = {
                       .type = EntityType::Conveyer}) {
    furniture::make_furniture(conveyer, options, pos, ui::color::blue,
                              ui::color::blue);
    conveyer.get<CustomHeldItemPosition>().init(
        CustomHeldItemPosition::Positioner::Conveyer);
    conveyer.addComponent<ConveysHeldItem>();
    conveyer.addComponent<CanBeTakenFrom>();
}

void make_grabber(Entity& grabber, vec2 pos,
                  const DebugOptions& options = {.type = EntityType::Grabber}) {
    furniture::make_conveyer(grabber, pos, options);
    grabber.addComponent<CanGrabFromOtherFurniture>();
}

void make_filtered_grabber(Entity& grabber, vec2 pos,
                           const DebugOptions& options = DebugOptions{
                               .type = EntityType::FilteredGrabber}) {
    furniture::make_grabber(grabber, pos, options);
    // Initially set empty filter
    grabber.get<CanHoldItem>().set_filter(
        EntityFilter()
            .set_enabled_flags(EntityFilter::FilterDatumType::Name |
                               EntityFilter::FilterDatumType::Subtype)
            .set_filter_strength(EntityFilter::FilterStrength::Suggestion));
}

void make_register(Entity& reg, vec2 pos) {
    furniture::make_furniture(reg, DebugOptions{.type = EntityType::Register},
                              pos, ui::color::grey, ui::color::grey);
    reg.addComponent<HasWaitingQueue>();
}

void make_itemcontainer(Entity& container, const DebugOptions& options,
                        vec2 pos, EntityType item_type) {
    furniture::make_furniture(container, options, pos, ui::color::white,
                              ui::color::white);
    container.addComponent<IsItemContainer>(item_type);
}

void make_squirter(Entity& squ, vec2 pos) {
    furniture::make_furniture(squ, {EntityType::Squirter}, pos);
    // TODO change how progress bar works to support this
    // squ.addComponent<ShowsProgressBar>();
}

void make_trash(Entity& trash, vec2 pos) {
    furniture::make_furniture(trash, {EntityType::Trash}, pos);
}

void make_pnumatic_pipe(Entity& pnumatic, vec2 pos) {
    furniture::make_conveyer(pnumatic, pos, {EntityType::PnumaticPipe});

    pnumatic.addComponent<IsPnumaticPipe>();
    pnumatic.get<CustomHeldItemPosition>().init(
        CustomHeldItemPosition::Positioner::PnumaticPipe);
}

void make_medicine_cabinet(Entity& container, vec2 pos) {
    furniture::make_itemcontainer(container, {EntityType::MedicineCabinet}, pos,
                                  EntityType::Alcohol);
    container.addComponent<Indexer>(ingredient::NUM_ALC);
    container.addComponent<HasWork>().init(
        [](Entity& owner, HasWork& hasWork, Entity&, float dt) {
            if (GameState::get().is_not(game::State::InRound)) return;
            const float amt = 2.f;
            hasWork.increase_pct(amt * dt);
            if (hasWork.is_work_complete()) {
                // TODO check if this new item we incremented to is
                // enabled
                // i tried to do this by a sophie fetch but that was causing map
                // generation to fail (silently)

                owner.get<Indexer>().increment();
                hasWork.reset_pct();
            }
        });
    container.addComponent<ShowsProgressBar>();
}

void make_fruit_basket(Entity& container, vec2 pos) {
    furniture::make_itemcontainer(container, {EntityType::PillDispenser}, pos,
                                  EntityType::Lemon);

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
    furniture::make_itemcontainer(cupboard, {EntityType::Cupboard}, pos,
                                  EntityType::Drink);
    cupboard.addComponent<HasDynamicModelName>().init(
        EntityType::Cupboard, HasDynamicModelName::DynamicType::OpenClosed);
}

void make_soda_machine(Entity& soda_machine, vec2 pos) {
    furniture::make_itemcontainer(soda_machine,
                                  DebugOptions{.type = EntityType::SodaMachine},
                                  pos, EntityType::SodaSpout);
    soda_machine.addComponent<HasRopeToItem>();
    soda_machine.get<IsItemContainer>().set_max_generations(1);
    soda_machine.get<CanHoldItem>().set_filter(
        EntityFilter()
            .set_enabled_flags(EntityFilter::FilterDatumType::Name)
            .set_filter_value_for_type(EntityFilter::FilterDatumType::Name,
                                       EntityType::SodaSpout)
            .set_filter_strength(EntityFilter::FilterStrength::Requirement));
}

// TODO what happens if the day ends and you are holding the mop still?
void make_mop_holder(Entity& mop_holder, vec2 pos) {
    furniture::make_itemcontainer(mop_holder,
                                  DebugOptions{.type = EntityType::MopHolder},
                                  pos, EntityType::Mop);
    mop_holder.get<IsItemContainer>().set_max_generations(1);
    mop_holder.get<CanHoldItem>().set_filter(
        EntityFilter()
            .set_enabled_flags(EntityFilter::FilterDatumType::Name)
            .set_filter_value_for_type(EntityFilter::FilterDatumType::Name,
                                       EntityType::Mop)
            .set_filter_strength(EntityFilter::FilterStrength::Requirement));
}

void make_trigger_area(Entity& trigger_area, vec3 pos, float width,
                       float height, IsTriggerArea::Type type) {
    make_entity(trigger_area, {EntityType::TriggerArea}, pos);

    trigger_area.get<Transform>().update_size({
        width,
        TILESIZE / 20.f,
        height,
    });

    trigger_area.addComponent<SimpleColoredBoxRenderer>().update(PINK, PINK);
    trigger_area.addComponent<IsTriggerArea>(type);
}

void make_blender(Entity& blender, vec2 pos) {
    furniture::make_furniture(blender,
                              DebugOptions{.type = EntityType::Blender}, pos,
                              ui::color::red, ui::color::yellow);
}

// This will be a catch all for anything that just needs to get updated
void make_sophie(Entity& sophie, vec3 pos) {
    make_entity(sophie, {EntityType::Sophie}, pos);

    sophie.addComponent<HasTimer>(HasTimer::Renderer::Round,
                                  round_settings::ROUND_LENGTH_S);
    sophie.addComponent<IsProgressionManager>().init();
}

void make_vomit(Entity& vomit, vec2 pos) {
    make_entity(vomit, {.type = EntityType::Vomit});

    vomit.get<Transform>().init({pos.x, 0, pos.y},
                                {TILESIZE, TILESIZE, TILESIZE});
    if (ENABLE_MODELS) {
        vomit.addComponent<ModelRenderer>(EntityType::Vomit);
    }

    vomit.addComponent<CanBeHighlighted>();

    // TODO please just add this to has work or something cmon
    vomit.addComponent<ShowsProgressBar>();

    vomit.addComponent<HasWork>().init(
        [](Entity& vom, HasWork& hasWork, const Entity& player, float dt) {
            if (GameState::get().is_not(game::State::InRound)) return;

            // He can mop without holding a mop
            if (check_type(player, EntityType::MopBuddy)) {
            } else {
                const CanHoldItem& playerCHI = player.get<CanHoldItem>();
                // not holding anything
                if (playerCHI.empty()) return;
                std::shared_ptr<Item> item = playerCHI.const_item();
                // Has to be holding mop
                if (!check_type(*item, EntityType::Mop)) return;
            }

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

    item.addComponent<ModelRenderer>(options.type);
}

void make_soda_spout(Item& soda_spout, vec2 pos) {
    make_item(soda_spout, {.type = EntityType::SodaSpout}, pos);

    soda_spout.get<IsItem>()
        .set_hb_filter(EntityType::SodaMachine)
        .set_hb_filter(EntityType::Player);

    // TODO :SODAWAND: right now theres no good way to change what is selected
    // in the soda wand, id like to have it automatically figure it out but it
    // doesnt really work because we dont know what the player is trying to make
    // and so its easier if everything is soda
    soda_spout.addComponent<AddsIngredient>(
        [](Entity&) { return Ingredient::Soda; });
}

void make_mop(Item& mop, vec2 pos) {
    make_item(mop, {.type = EntityType::Mop}, pos);

    mop.get<IsItem>()
        .set_hb_filter(EntityType::MopHolder)
        .set_hb_filter(EntityType::Player);
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
    if (GameState::get().is_not(game::State::InRound)) return;
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
    make_item(alc, {.type = EntityType::Alcohol}, pos);

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
        EntityType::Alcohol, HasDynamicModelName::DynamicType::Subtype,
        [](const Item& owner, const std::string&) -> std::string {
            const HasSubtype& hst = owner.get<HasSubtype>();
            Ingredient bottle = get_ingredient_from_index(
                ingredient::ALC_START + hst.get_type_index());
            return util::toLowerCase(magic_enum::enum_name<Ingredient>(bottle));
        });
}

void make_simple_syrup(Item& simple_syrup, vec2 pos) {
    make_item(simple_syrup, {.type = EntityType::SimpleSyrup}, pos);

    // TODO right now this is the only ingredient that doesnt have a spawner
    // but also doesnt get destroyed after one use
    simple_syrup
        .addComponent<AddsIngredient>(
            [](Entity&) { return Ingredient::SimpleSyrup; })
        .set_num_uses(-1);

    // Since theres only one of these and its inf uses, dont let it get deleted
    simple_syrup.get<IsItem>()
        .set_hb_filter(ETS_NON_DESTRUCTIVE)
        // TODO create a class of objects that "do work"
        .remove_hb_filter(EntityType::Blender);
}

void make_lemon(Item& lemon, vec2 pos, int index) {
    make_item(lemon, {.type = EntityType::Lemon}, pos);

    // TODO i dont like that you have to remember to do +1 here
    lemon.addComponent<HasSubtype>(ingredient::LEMON_START,
                                   ingredient::LEMON_END + 1, index);

    lemon
        .addComponent<AddsIngredient>([](const Entity& lemmy) {
            const HasSubtype& hst = lemmy.get<HasSubtype>();
            return get_ingredient_from_index(ingredient::LEMON_START +
                                             hst.get_type_index());
        })
        .set_num_uses(1);

    lemon.addComponent<HasDynamicModelName>().init(
        EntityType::Lemon,  //
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
                    return strings::model::LEMON;
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
        if (GameState::get().is_not(game::State::InRound)) return;
        const IsItem& ii = owner.get<IsItem>();
        HasSubtype& hasSubtype = owner.get<HasSubtype>();
        Ingredient lemon_type = get_ingredient_from_index(
            ingredient::LEMON_START + hasSubtype.get_type_index());

        // Can only handle lemon -> juice right now
        if (lemon_type != Ingredient::Lemon) return;
        // TODO we shouldnt blindly increment type but in this case its
        // okay i guess

        if (ii.is_not_held_by(EntityType::Blender)) {
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
    make_item(drink, {.type = EntityType::Drink}, pos);

    drink.addComponent<IsDrink>();
    drink.addComponent<HasWork>().init(std::bind(
        process_drink_working, std::placeholders::_1, std::placeholders::_2,
        std::placeholders::_3, std::placeholders::_4));
    // TODO should this just be part of has work?
    drink.addComponent<ShowsProgressBar>();

    drink.addComponent<HasDynamicModelName>().init(
        EntityType::Drink, HasDynamicModelName::DynamicType::Ingredients,
        [](const Item& owner, const std::string&) -> std::string {
            const IsDrink& isdrink = owner.get<IsDrink>();
            constexpr auto drinks = magic_enum::enum_values<Drink>();
            for (Drink d : drinks) {
                if (isdrink.matches_recipe(d))
                    return get_model_name_for_drink(d);
            }
            return util::convertToSnakeCase<EntityType>(EntityType::Drink);
        });
}

void make_item_type(Item& item, EntityType type, vec2 pos, int index) {
    // log_info("generating new item {} of type {} at {} subtype{}", item.id,
    // type_name, pos, index);
    switch (type) {
        case EntityType::SodaSpout:
            return make_soda_spout(item, pos);
        case EntityType::Alcohol:
            return make_alcohol(item, pos, index);
        case EntityType::Lemon:
            return make_lemon(item, pos, index);
        case EntityType::Drink:
            return make_drink(item, pos);
        case EntityType::Mop:
            return make_mop(item, pos);
        default:
            break;
    }
    log_warn(
        "Trying to make item with item type {} but not handled in "
        "make_item_type()",
        util::convertToSnakeCase<EntityType>(type));
}

}  // namespace items

void make_customer(Entity& customer, vec2 p, bool has_order) {
    make_aiperson(customer, DebugOptions{.type = EntityType::Customer},
                  vec::to3(p));

    customer.addComponent<HasName>().update(get_random_name());

    // TODO for now, eventually move to customer spawner
    if (has_order) customer.addComponent<CanOrderDrink>();

    customer.addComponent<HasSpeechBubble>();

    customer.get<HasBaseSpeed>().update(5.f);
    customer.get<CanPerformJob>().update(WaitInQueue, Wandering);

    // TODO if we do dirty-cups, we should have people leave them on any flat
    // surface but if they are too drunk... just smash on the ground
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
        // TODO only enable this once you start serving alcohol
        // TODO should we by default give the mop? or should you be able to
        // clean by hand but slowly?
        .set_total(10)
        .set_time_between(2.f);
}

namespace furniture {
void make_customer_spawner(Entity& customer_spawner, vec3 pos) {
    make_entity(customer_spawner, {EntityType::CustomerSpawner}, pos);

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

void convert_to_type(const EntityType& entity_type, Entity& entity,
                     vec2 location) {
    // TODO at some point just change all of these to match
    auto pos = vec::to3(location);
    switch (entity_type) {
        case EntityType::Unknown:
        case EntityType::x:
        case EntityType::y:
        case EntityType::z:
            make_entity(entity, DebugOptions{.type = entity_type}, pos);
            return;
        case EntityType::RemotePlayer: {
            make_remote_player(entity, pos);
        } break;
        case EntityType::Player: {
            make_player(entity, pos);
        } break;
        case EntityType::Table: {
            furniture::make_table(entity, location);
        } break;
        case EntityType::CharacterSwitcher: {
            furniture::make_character_switcher(entity, location);
        } break;
        case EntityType::MapRandomizer: {
            furniture::make_map_randomizer(entity, location);
        } break;
        case EntityType::Wall: {
            const auto d_color = Color{155, 75, 0, 255};
            (furniture::make_wall(entity, location, d_color));
        } break;
        case EntityType::Conveyer: {
            furniture::make_conveyer(entity, location);
        } break;
        case EntityType::Grabber: {
            furniture::make_grabber(entity, location);
        } break;
        case EntityType::Register: {
            furniture::make_register(entity, location);
        } break;
        case EntityType::MedicineCabinet: {
            furniture::make_medicine_cabinet(entity, location);
        } break;
        case EntityType::PillDispenser: {
            furniture::make_fruit_basket(entity, location);
        } break;
        case EntityType::CustomerSpawner: {
            furniture::make_customer_spawner(entity, pos);
        } break;
        case EntityType::Sophie: {
            furniture::make_sophie(entity, pos);
        } break;
        case EntityType::Blender: {
            furniture::make_blender(entity, location);
        } break;
        case EntityType::SodaMachine: {
            furniture::make_soda_machine(entity, location);
        } break;
        case EntityType::Cupboard: {
            furniture::make_cupboard(entity, location);
        } break;
        case EntityType::Squirter: {
            furniture::make_squirter(entity, location);
        } break;
        case EntityType::Trash: {
            furniture::make_trash(entity, location);
        } break;
        case EntityType::FilteredGrabber: {
            furniture::make_filtered_grabber(entity, location);
        } break;
        case EntityType::PnumaticPipe: {
            furniture::make_pnumatic_pipe(entity, location);
        } break;
        case EntityType::MopHolder: {
            furniture::make_mop_holder(entity, location);
        } break;
        case EntityType::FastForward: {
            furniture::make_fast_forward(entity, location);
        } break;
        case EntityType::SimpleSyrup: {
            items::make_simple_syrup(entity, location);
        } break;
        case EntityType::MopBuddy: {
            make_mop_buddy(entity, location);
        } break;
        case EntityType::TriggerArea:
        case EntityType::Vomit:
        case EntityType::SodaSpout:
        case EntityType::Drink:
        case EntityType::Alcohol:
        case EntityType::Customer:
        case EntityType::Lemon:
        case EntityType::Mop:
            log_warn("{} cant be created through 'convert_to_type'",
                     entity_type);
            break;
        case EntityType::MAX_ENTITY_TYPE:
            log_warn("{} should not be tried to create at all tbh",
                     entity_type);
            break;
    }
}
