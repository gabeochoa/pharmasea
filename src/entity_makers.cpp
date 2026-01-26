
#include "entity_makers.h"

#include <ranges>

#include "afterhours/src/core/base_component.h"
#include "ah.h"
#include "components/all_components.h"
#include "components/bypass_automation_state.h"
#include "components/can_change_settings_interactively.h"
#include "components/can_hold_handtruck.h"
#include "components/can_pathfind.h"
#include "components/collects_customer_feedback.h"
#include "components/has_day_night_timer.h"
#include "components/has_fishing_game.h"
#include "components/has_last_interacted_customer.h"
#include "components/has_progression.h"
#include "components/has_rope_to_item.h"
#include "components/has_subtype.h"
#include "components/is_ai_controlled.h"
#include "components/is_bank.h"
#include "components/is_customer.h"
#include "components/is_floor_marker.h"
#include "components/is_free_in_store.h"
#include "components/is_nux_manager.h"
#include "components/is_pnumatic_pipe.h"
#include "components/is_progression_manager.h"
#include "components/is_round_settings_manager.h"
#include "components/is_squirter.h"
#include "components/is_store_spawned.h"
#include "components/is_toilet.h"
#include "components/responds_to_day_night.h"
#include "dataclass/ingredient.h"
#include "dataclass/upgrade_class.h"
#include "engine/runtime_globals.h"
#include "engine/sound_library.h"
#include "engine/ui/color.h"
#include "engine/util.h"
#include "entity.h"
#include "entity_helper.h"
#include "entity_id.h"
#include "entity_type.h"
#include "system/ai/ai_tags.h"
//
#include "client_server_comm.h"
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
#include "components/collects_user_input.h"
#include "components/conveys_held_item.h"
#include "components/custom_item_position.h"
#include "components/has_ai_cooldown.h"
#include "components/has_base_speed.h"
#include "components/has_client_id.h"
#include "components/has_dynamic_model_name.h"
#include "components/has_name.h"
#include "components/has_patience.h"
#include "components/has_speech_bubble.h"
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
#include "components/simple_colored_box_renderer.h"
#include "components/transform.h"
#include "components/uses_character_model.h"
#include "dataclass/names.h"
#include "entity_query.h"
#include "recipe_library.h"
#include "strings.h"
#include "system/core/system_manager.h"

namespace items {
// Returns true if item was cleaned up
bool _add_ingredient_to_drink_NO_VALIDATION(Entity& drink, Ingredient ing) {
    IsDrink& isdrink = drink.get<IsDrink>();
    isdrink.add_ingredient(ing);

    IngredientSoundType sound_type = ingredient::IngredientSoundType.at(ing);

    strings::sounds::SoundId sound;
    switch (sound_type) {
        case Viscous:
            // TODO add new sounds for other ingredient types
        case Solid:
            sound = strings::sounds::SoundId::SOLID;
            break;
        case Ice:
            sound = strings::sounds::SoundId::ICE;
            break;
        case Liquid:
            sound = strings::sounds::SoundId::WATER;
            break;
        default:
            return false;
    }
    server_only::play_sound(drink.get<Transform>().as2(), sound);
    return true;
}

bool _add_item_to_drink_NO_VALIDATION(Entity& drink, Item& toadd) {
    if (toadd.has<HasFishingGame>()) {
        const HasFishingGame& hfg = toadd.get<HasFishingGame>();
        if (hfg.has_score()) {
            drink.get<IsDrink>().fold_tip_multiplier(hfg.get_score());
        }
    }

    AddsIngredient& addsIG = toadd.get<AddsIngredient>().set_parent(toadd.id);
    IngredientBitSet ingredients = addsIG.get(toadd);

    bitset_utils::for_each_enabled_bit(ingredients, [&](size_t bit) {
        Ingredient ig = magic_enum::enum_value<Ingredient>(bit);
        _add_ingredient_to_drink_NO_VALIDATION(drink, ig);
        return bitset_utils::ForEachFlow::NormalFlow;
    });

    addsIG.decrement_uses();
    // We do == 0 because infinite is -1
    if (addsIG.uses_left() == 0) {
        toadd.cleanup = true;
        return true;
    }
    return false;
}

}  // namespace items

void register_all_components() {
    // Register component IDs in Afterhours by calling get_type_id for each
    // component type. This registers the types without instantiating them.
    // Keep the list centralized in `components/all_components.h`.
    [&]<size_t... Is>(std::index_sequence<Is...>) {
        (afterhours::components::get_type_id<
             std::tuple_element_t<Is, snapshot_blob::ComponentTypes>>(),
         ...);
    }(std::make_index_sequence<
        std::tuple_size_v<snapshot_blob::ComponentTypes>>{});
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
        person.addComponent<SimpleColoredBoxRenderer>()
            .update_base(RED)
            .update_face(PINK);
    }

    if (options.enableCharacterModel) {
        person.addComponent<UsesCharacterModel>();
    }
}

void make_entity(Entity& entity, const DebugOptions& options, vec3 p) {
    entity.enableTag(options.type);

    add_entity_components(entity);
    entity.get<Transform>().update(p);
}

void make_entity(Entity& entity, const DebugOptions& options, vec2 p) {
    // This function exists because without it calling
    // make_entity(..., {0,1}) would call the vec3 version
    // but with y = 1 instead of z = 1 as youd expect
    return make_entity(entity, options, vec::to3(p));
}

void add_player_components(Entity& player) {
    player.addComponent<CanHighlightOthers>();
    player.addComponent<CanHoldFurniture>();
    player.addComponent<CanHoldHandTruck>();
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
    player.addComponent<PlayerInputQueue>();
    player.addComponent<IsSolid>();

    // This one is only added to local players
    // player.addComponent<CanBeGhostPlayer>();

    player.addComponent<RespondsToUserInput>();
}

void make_aiperson(Entity& person, const DebugOptions& options, vec3 p) {
    make_entity(person, options, p);
    add_person_components(person, options);

    person.addComponent<IsAIControlled>();
    person.addComponent<HasAICooldown>();
    person.addComponent<CanPathfind>().set_parent(person.id);
}

void make_mop_buddy(Entity& mop_buddy, vec3 pos) {
    make_aiperson(mop_buddy,
                  DebugOptions{.type = EntityType::MopBuddy,
                               .enableCharacterModel = false},
                  pos);

    mop_buddy.get<ModelRenderer>().update_model_name(
        util::convertToSnakeCase(EntityType::MopBuddy));

    mop_buddy.get<HasBaseSpeed>().update(1.5f);
    mop_buddy.get<IsAIControlled>()
        .set_initial_state(IsAIControlled::State::CleanVomit)
        .enable_ability(IsAIControlled::AbilityCleanVomit);

    mop_buddy
        .addComponent<IsItem>()  //
        .clear_hb_filter()
        .set_hb_filter(EntityType::Player);
}

void make_face(Entity& face, vec3 pos) {
    make_entity(face, {EntityType::Face}, pos);

    face.addComponent<CanBeHeld>();
}

void make_ai_target_location(Entity& entity, vec3 pos) {
    make_entity(entity, {EntityType::AITargetLocation}, pos);
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
    furniture.addComponent<SimpleColoredBoxRenderer>()
        .update_face(face)
        .update_base(base);

    if (ENABLE_MODELS) {
        auto model_name = util::convertToSnakeCase<EntityType>(options.type);
        if (ModelInfoLibrary::get().has(model_name)) {
            furniture.addComponent<ModelRenderer>(options.type);
        }
    }

    if (furniture.is_missing<ModelRenderer>()) {
        furniture.get<Transform>().update_visual_offset({0, -0.25f, 0});
        furniture.get<Transform>().update_size(
            {TILESIZE, TILESIZE * 0.5f, TILESIZE});
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

    OptEntity item_opt = tableCHI.const_item();
    if (!item_opt) {
        tableCHI.update(nullptr, table.id);
        return;
    }
    if (!item_opt->has<HasWork>()) return;

    Item& item = item_opt.asE();
    // We have to call the item's hasWork because the table
    // doesnt actually do anything, its the item that we are working
    //
    // It has to be this way otherwise you would be able to do work while just
    // standing around which wouldnt make sense
    HasWork& itemHasWork = item.get<HasWork>();
    itemHasWork.call(hasWork, item, player, dt);
}

void make_table(Entity& table, vec2 pos) {
    furniture::make_furniture(table, DebugOptions{.type = EntityType::Table},
                              pos, ui::color::brown, ui::color::brown);

    table.addComponent<HasWork>().init(std::bind(
        process_table_working, std::placeholders::_1, std::placeholders::_2,
        std::placeholders::_3, std::placeholders::_4));
}

void make_character_switcher(Entity& character_switcher, vec2 pos) {
    furniture::make_furniture(
        character_switcher, DebugOptions{.type = EntityType::CharacterSwitcher},
        pos, ui::color::green, ui::color::yellow);

    // TODO add a wayt to let it know to translate
    character_switcher.addComponent<HasName>().update(
        // TODO instead of HasName this should be HasI18nName
        strings::pre_translation.at(strings::i18n::CHARACTER_SWITCHER));

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
}

void make_map_randomizer(Entity& map_randomizer, vec2 pos) {
    furniture::make_furniture(map_randomizer,
                              DebugOptions{.type = EntityType::MapRandomizer},
                              pos, ui::color::baby_blue, ui::color::baby_pink);

    map_randomizer.addComponent<HasName>().update("");
    map_randomizer.addComponent<CanBeHighlighted>();

    map_randomizer.addComponent<HasWork>().init(
        [](Entity& randomizer, HasWork& hasWork, Entity&, float dt) {
            if (GameState::get().is_not(game::State::Lobby)) return;
            const float amt = 1.5f;
            hasWork.increase_pct(amt * dt);
            if (hasWork.is_work_complete()) {
                hasWork.reset_pct();
                server_only::update_seed(get_random_name_rot13());
            }
            HasName& hasName = randomizer.get<HasName>();
            hasName.update(server_only::get_current_seed());
        });
}

void make_fast_forward(Entity& fast_forward, vec2 pos) {
    // TODO only show when you are nearby, hide the progress bar otherwise
    //
    furniture::make_furniture(fast_forward,
                              DebugOptions{.type = EntityType::FastForward},
                              pos, ui::color::apricot, ui::color::apricot);

    fast_forward.removeComponent<CanHoldItem>();

    // TODO translate
    fast_forward.addComponent<HasName>().update("Fast-Forward Day");

    fast_forward.addComponent<HasWork>().init(
        [](Entity&, HasWork& hasWork, Entity&, float dt) {
            const auto debug_mode_on = globals::debug_ui_enabled();

            const float amt = debug_mode_on ? 100.f : 15.f;

            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            HasDayNightTimer& ht = sophie.get<HasDayNightTimer>();

            // can only ffwd if theres a good maount of the day left
            if (ht.pct() <= 0.02f) return;

            {
                ht.pass_time(amt * dt);
                hasWork.update_pct(1.f - ht.pct());
            }

            // TODO i dont think the spawner is working correctly
            {
                IsSpawner& isp = EntityQuery()
                                     .whereType(EntityType::CustomerSpawner)
                                     .gen_first()
                                     ->get<IsSpawner>();
                isp.pass_time(amt * dt);
            }

            if (hasWork.is_work_complete()) {
                hasWork.reset_pct();
            }
        });
}

void make_door(Entity& door, vec2 pos, Color) {
    furniture::make_furniture(door, DebugOptions{.type = EntityType::Door}, pos,
                              ui::color::ugly_yellow, ui::color::ugly_blue,
                              true);

    door.addComponent<RespondsToDayNight>()
        // TODO this is a hack because the parent is not set automatically with
        // afterhours
        .set_parent(door.id)
        .registerOnDayStarted(
            [](Entity& door) { door.removeComponentIfExists<IsSolid>(); })
        .registerOnNightStarted(
            [](Entity& door) { door.addComponentIfMissing<IsSolid>(); });
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
    furniture::make_furniture(container, options, pos, ui::color::brown,
                              ui::color::brown);
    container.addComponent<IsItemContainer>(item_type);
}

void make_squirter(Entity& squ, vec2 pos) {
    furniture::make_furniture(squ, {EntityType::Squirter}, pos);
    squ.addComponent<IsSquirter>();
}

void make_trash(Entity& trash, vec2 pos) {
    furniture::make_furniture(trash, {EntityType::Trash}, pos);
}

void make_guitar(Entity& guitar, vec2 pos) {
    furniture::make_furniture(guitar, {EntityType::Guitar}, pos);
}

void make_toilet(Entity& toilet, vec2 pos) {
    furniture::make_furniture(toilet, {EntityType::Toilet}, pos);

    toilet.addComponent<IsToilet>();

    // TODO should this happen with an upgrade?
    toilet.addComponent<HasWaitingQueue>();

    toilet.addComponent<HasWork>().init(
        [](Entity& toilet, HasWork& hasWork, Entity& /*player*/, float dt) {
            IsToilet& istoilet = toilet.get<IsToilet>();

            // Dont allow cleaning when someone is currently using it
            if (istoilet.state == IsToilet::State::InUse) {
                return;
            }

            // TODO figure out a better number
            // recently cleaned
            if (istoilet.pct_full() <= 0.25f) {
                hasWork.reset_pct();
                return;
            }

            // TODO maybe if you have the mop or something it should be faster

            const float amt = 1.0f;
            hasWork.increase_pct(amt * dt);

            if (hasWork.is_work_complete()) {
                hasWork.reset_pct();

                istoilet.reset();
            }
        });
}

void make_pnumatic_pipe(Entity& pnumatic, vec2 pos) {
    furniture::make_conveyer(pnumatic, pos, {EntityType::PnumaticPipe});

    pnumatic.addComponent<IsPnumaticPipe>();
    pnumatic.get<CustomHeldItemPosition>().init(
        CustomHeldItemPosition::Positioner::PnumaticPipe);
}

void make_ice_machine(Entity& machine, vec2 pos) {
    furniture::make_furniture(
        machine, DebugOptions{.type = EntityType::IceMachine}, pos);

    machine.addComponent<HasWork>().init(
        [](Entity&, HasWork& hasWork, Entity& player, float dt) {
            CanHoldItem& chi = player.get<CanHoldItem>();
            OptEntity item_opt = chi.item();
            if (!item_opt) {
                chi.update(nullptr, player.id);
                return;
            }
            Item& item = item_opt.asE();
            if (!check_if_drink(item)) return;

            Ingredient ing = Ingredient::IceCubes;

            IsDrink& isdrink = item.get<IsDrink>();
            if (isdrink.has_ingredient(ing)) return;

            const float amt = 1.0f;
            hasWork.increase_pct(amt * dt);

            if (hasWork.is_work_complete()) {
                hasWork.reset_pct();
                items::_add_ingredient_to_drink_NO_VALIDATION(item, ing);
            }
        });
}

void make_single_alcohol(Entity& container, vec2 pos, int alcohol_index) {
    furniture::make_itemcontainer(container, {EntityType::SingleAlcohol}, pos,
                                  EntityType::Alcohol);

    container.get<IsItemContainer>().set_uses_indexer(true);
    container.addComponent<Indexer>((int) ingredient::AlcoholsInCycle.size())
        .set_value(alcohol_index);
}

void make_medicine_cabinet(Entity& container, vec2 pos) {
    furniture::make_itemcontainer(container, {EntityType::AlcoholCabinet}, pos,
                                  EntityType::Alcohol);
    container.get<IsItemContainer>().set_uses_indexer(true);

    container.addComponent<Indexer>((int) ingredient::AlcoholsInCycle.size());
    container.addComponent<HasWork>().init([](Entity& owner, HasWork& hasWork,
                                              Entity&, float dt) {
        if (SystemManager::get().is_bar_open()) return;
        const Indexer& indexer = owner.get<Indexer>();
        if (indexer.max() < 2) return;

        const float amt = 2.f;
        hasWork.increase_pct(amt * dt);
        if (hasWork.is_work_complete()) {
            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            const IsProgressionManager& ipp =
                sophie.get<IsProgressionManager>();

            size_t i = 0;
            do {
                i++;
                owner.get<Indexer>().increment();
                // TODO will show something even if you dont have it
                // unlocked, we just need a way to say "hey theres no
                // alcohol unlocked, just skip all of this
                if (i > ingredient::AlcoholsInCycle.size()) break;

            } while (ipp.is_ingredient_locked(
                ingredient::AlcoholsInCycle[owner.get<Indexer>().value()]));

            hasWork.reset_pct();
        }
    });
}

void make_fruit_basket(Entity& container, vec2 pos, int starting_index = 0) {
    furniture::make_itemcontainer(container, {EntityType::FruitBasket}, pos,
                                  EntityType::Fruit);

    container.addComponent<Indexer>((int) ingredient::Fruits.size())
        .set_value(starting_index);
    container.get<IsItemContainer>().set_uses_indexer(true);
    //
    container.addComponent<HasWork>().init([](Entity& owner, HasWork& hasWork,
                                              Entity&, float dt) {
        const float amt = 2.f;
        hasWork.increase_pct(amt * dt);
        if (hasWork.is_work_complete()) {
            // TODO :backfill-correct: This should match whats in backfill
            // container

            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            const IsProgressionManager& ipp =
                sophie.get<IsProgressionManager>();

            Indexer& indexer = owner.get<Indexer>();
            indexer.increment_until_valid([&](int index) {
                return !ipp.is_ingredient_locked(ingredient::Fruits[index]);
            });

            hasWork.reset_pct();
        }
    });
}

void make_cupboard(Entity& cupboard, vec2 pos, int index) {
    EntityType held_type = EntityType::Drink;
    CupType ct = magic_enum::enum_value<CupType>(index);
    switch (ct) {
        case Normal:
            held_type = EntityType::Drink;
            break;
        case MultiCup:
            held_type = EntityType::Pitcher;
            break;
    }

    furniture::make_itemcontainer(cupboard, {EntityType::Cupboard}, pos,
                                  held_type);
    cupboard.addComponent<HasSubtype>(
        0, (int) magic_enum::enum_count<CupType>(), index);

    cupboard.addComponent<HasDynamicModelName>().init(
        EntityType::Cupboard, HasDynamicModelName::DynamicType::OpenClosed);
    cupboard.get<CanHoldItem>().set_filter(
        EntityFilter()
            .set_enabled_flags(EntityFilter::FilterDatumType::Name |
                               EntityFilter::FilterDatumType::Ingredients)
            .set_filter_value_for_type(EntityFilter::FilterDatumType::Name,
                                       held_type)
            .set_filter_value_for_type(
                EntityFilter::FilterDatumType::Ingredients,
                // Only allow empty drinks
                IngredientBitSet())
            .set_filter_strength(EntityFilter::FilterStrength::Requirement));
}

void make_soda_fountain(Entity& soda_fountain, vec2 pos) {
    furniture::make_furniture(soda_fountain,
                              DebugOptions{.type = EntityType::SodaFountain},
                              pos, ui::color::brown, ui::color::brown);
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

void make_simple_syrup_holder(Entity& simple_syrup_holder, vec2 pos) {
    furniture::make_itemcontainer(
        simple_syrup_holder,
        DebugOptions{.type = EntityType::SimpleSyrupHolder}, pos,
        EntityType::SimpleSyrup);

    simple_syrup_holder.get<IsItemContainer>()
        .set_max_generations(1)
        .enable_table_when_enable();
    // We are not setting a filter because we want this to just act like a
    // normal table if its empty
}

void make_mopbuddy_holder(Entity& mopbuddy_holder, vec2 pos) {
    furniture::make_itemcontainer(
        mopbuddy_holder, DebugOptions{.type = EntityType::MopBuddyHolder}, pos,
        EntityType::MopBuddy);
    mopbuddy_holder.get<IsItemContainer>()
        .set_max_generations(1)
        .enable_table_when_enable();

    // TODO If we decide to make it only the roomba
    // mopbuddy_holder.get<CanHoldItem>().set_filter(
    // EntityFilter()
    // .set_enabled_flags(EntityFilter::FilterDatumType::Name)
    // .set_filter_value_for_type(EntityFilter::FilterDatumType::Name,
    // EntityType::MopBuddy)
    // .set_filter_strength(EntityFilter::FilterStrength::Requirement));
}

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

void make_floor_marker(Entity& floor_marker, vec3 pos, float width,
                       float height, IsFloorMarker::Type type) {
    make_entity(floor_marker, {EntityType::FloorMarker}, pos);

    floor_marker.get<Transform>().update_size({
        width,
        TILESIZE / 20.f,
        height,
    });

    floor_marker.addComponent<SimpleColoredBoxRenderer>()
        .update_face(PINK)
        .update_base(PINK);

    floor_marker.addComponent<IsFloorMarker>(type);
}

void make_trigger_area(Entity& trigger_area, vec3 pos, float width,
                       float height, IsTriggerArea::Type type) {
    make_entity(trigger_area, {EntityType::TriggerArea}, pos);

    trigger_area.get<Transform>().update_size({
        width,
        TILESIZE / 20.f,
        height,
    });

    trigger_area.addComponent<SimpleColoredBoxRenderer>()
        .update_face(PINK)
        .update_base(PINK);
    trigger_area.addComponent<IsTriggerArea>(type);
    trigger_area.enableTag(
        afterhours::tags::TriggerTag::TriggerAreaHasDynamicText);

    switch (type) {
        case IsTriggerArea::LoadSave_ToggleDeleteMode:
        case IsTriggerArea::Planning_SaveSlot:
            trigger_area.enableTag(
                afterhours::tags::TriggerTag::GateTriggerWhileOccupied);
            break;
        default:
            break;
    }
}

void make_draft(Entity& draft, vec2 pos) {
    furniture::make_furniture(draft, DebugOptions{.type = EntityType::DraftTap},
                              pos, ui::color::red, ui::color::yellow);

    draft.addComponent<HasWork>().init(
        [](Entity& draftmachine, HasWork& hasWork, Entity&, float dt) {
            // TODO this logic is duplicated with _process_if_beer_tap
            CanHoldItem& chi = draftmachine.get<CanHoldItem>();
            OptEntity item_opt = chi.item();
            if (!item_opt) {
                chi.update(nullptr, draftmachine.id);
                return;
            }
            Item& item = item_opt.asE();
            if (!check_if_drink(item)) {
                return;
            }

            Ingredient ing = Ingredient::Beer;

            IsDrink& isdrink = item.get<IsDrink>();
            if (isdrink.has_ingredient(ing)) {
                if (!isdrink.supports_multiple()) return;
            }

            const float amt = 0.75f;
            hasWork.increase_pct(amt * dt);

            server_only::play_sound(item.get<Transform>().as2(),
                                    // TODO replace with draft tap sound
                                    strings::sounds::SoundId::BLENDER);

            if (hasWork.is_work_complete()) {
                hasWork.reset_pct();
                items::_add_ingredient_to_drink_NO_VALIDATION(item, ing);
            }
        });
}

void make_blender(Entity& blender, vec2 pos) {
    furniture::make_furniture(blender,
                              DebugOptions{.type = EntityType::Blender}, pos,
                              ui::color::red, ui::color::yellow);
    blender.get<CustomHeldItemPosition>().init(
        CustomHeldItemPosition::Positioner::Blender);
}

// This will be a catch all for anything that just needs to get updated
void make_sophie(Entity& sophie, vec3 pos) {
    make_entity(sophie, {EntityType::Sophie}, pos);

    sophie.addComponent<IsRoundSettingsManager>();
    IsRoundSettingsManager& irsm = sophie.get<IsRoundSettingsManager>();

    sophie.addComponent<HasDayNightTimer>(
        irsm.get_for_init<float>(ConfigKey::RoundLength));

    sophie.addComponent<CollectsCustomerFeedback>();

    sophie.addComponent<IsProgressionManager>().init();
    sophie.addComponent<IsBank>();
    sophie.addComponent<IsNuxManager>();
}

void make_vomit(Entity& vomit, const SpawnInfo& info) {
    make_entity(vomit, {.type = EntityType::Vomit});

    vomit.get<Transform>().init({info.location.x, 0, info.location.y},
                                {TILESIZE, TILESIZE, TILESIZE});
    if (ENABLE_MODELS) {
        vomit.addComponent<ModelRenderer>(EntityType::Vomit);
    }

    vomit.addComponent<CanBeHighlighted>();

    vomit.addComponent<HasWork>().init(
        [](Entity& vom, HasWork& hasWork, const Entity& player, float dt) {
            // if (SystemManager::get().is_bar_open()) return;

            const auto validate =
                [](const Entity& entity) -> std::pair<bool, float> {
                // He can mop without holding a mop
                if (check_type(entity, EntityType::MopBuddy))
                    return {true, 1.f};

                const CanHoldItem& playerCHI = entity.get<CanHoldItem>();
                // not holding anything
                if (playerCHI.empty()) return {true, 0.25f};
                OptEntity item_opt = playerCHI.const_item();
                if (!item_opt) return {true, 0.25f};
                const Item& item = item_opt.asE();
                // Has to be holding mop
                if (check_type(item, EntityType::Mop)) return {true, 2.f};

                // holding something other than mop
                return {false, 0.f};
            };

            const auto [canMop, mopSpeed] = validate(player);

            if (!canMop) return;

            const float amt = 1.f * mopSpeed;
            hasWork.increase_pct(amt * dt);
            if (hasWork.is_work_complete()) {
                hasWork.reset_pct();
                // Clean it up
                vom.cleanup = true;
            }
        });
}

}  // namespace furniture

void make_hand_truck(Entity& hand_truck, vec2 pos) {
    const DebugOptions options = {EntityType::HandTruck};
    make_entity(hand_truck, options, pos);

    hand_truck.get<Transform>().init({pos.x, 0, pos.y},
                                     {TILESIZE, TILESIZE, TILESIZE});
    hand_truck.addComponent<IsSolid>();
    hand_truck.addComponent<SimpleColoredBoxRenderer>()
        .update_face(BLUE)
        .update_base(BLUE);

    if (ENABLE_MODELS) {
        auto model_name = util::convertToSnakeCase<EntityType>(options.type);
        if (ModelInfoLibrary::get().has(model_name)) {
            hand_truck.addComponent<ModelRenderer>(options.type);
        }
    }

    if (hand_truck.is_missing<ModelRenderer>()) {
        hand_truck.get<Transform>().update_visual_offset({0, -0.25f, 0});
        hand_truck.get<Transform>().update_size(
            {TILESIZE, TILESIZE * 0.5f, TILESIZE});
    }

    // we need to add it to set a default, so its here
    hand_truck.addComponent<CustomHeldItemPosition>().init(
        CustomHeldItemPosition::Positioner::Table);

    hand_truck.addComponent<CanBeHeld_HT>();

    // TODO this is needed to process "reach" for pickup/drop
    hand_truck.addComponent<CanHighlightOthers>();

    hand_truck.addComponent<CanBeHighlighted>();
    hand_truck.addComponent<CanHoldFurniture>();
}

namespace items {

void make_item(Item& item, const DebugOptions& options, vec3 p) {
    make_entity(item, options, p);
    item.addComponent<IsItem>();
    // TODO Not everyone needs this but easier for now
    item.addComponent<CustomHeldItemPosition>().init(
        CustomHeldItemPosition::Positioner::ItemHoldingItem);

    item.addComponent<ModelRenderer>(options.type);
}

void make_soda_spout(Item& soda_spout, vec3 pos) {
    make_item(soda_spout, {.type = EntityType::SodaSpout}, pos);

    soda_spout.get<IsItem>()
        .clear_hb_filter()
        .set_hb_filter(EntityType::SodaMachine)
        .set_hb_filter(EntityType::Player);

    // TODO :SODAWAND: right now theres no good way to change what is selected
    // in the soda wand, id like to have it automatically figure it out but it
    // doesnt really work because we dont know what the player is trying to make
    // and so its easier if everything is soda
    soda_spout
        .addComponent<AddsIngredient>(
            [](const Entity&, Entity&) -> IngredientBitSet {
                return IngredientBitSet().reset().set(Ingredient::Soda);
            })
        .set_parent(soda_spout.id);
}

void make_mop(Item& mop, vec3 pos) {
    make_item(mop, {.type = EntityType::Mop}, pos);

    mop.get<IsItem>()
        .clear_hb_filter()
        .set_hb_filter(EntityType::MopHolder)
        .set_hb_filter(EntityType::Player);
}
void process_drink_working(Entity& drink, HasWork& hasWork, Entity& player,
                           float dt) {
    // TODO should we only allow making drinks during the night time?
    // if (SystemManager::get().is_bar_open()) return;
    //
    if (drink.is_missing<IsItem>()) return;
    if (drink.is_missing<IsDrink>()) return;

    const IsDrink& isdrink = drink.get<IsDrink>();

    IsItem& ii = drink.get<IsItem>();
    if (ii.is_held_by(EntityType::Cupboard)) {
        // if the cup is still the in the cupboard
        // dont let us add ingredients
        return;
    }

    const auto _process_if_beer_tap = [&]() {
        // TODO this logic is duplicated with make_draft hasWork
        //
        // TODO reset progress when taking out if not done
        if (!ii.is_held_by(EntityType::DraftTap)) return;

        // TODO we need way better ingredient validation across these kinds of
        // additive machines
        if (isdrink.has_ingredient(Ingredient::Beer)) {
            if (!isdrink.supports_multiple()) return;
        }

        const float amt = 0.75f;
        hasWork.increase_pct(amt * dt);
        server_only::play_sound(drink.get<Transform>().as2(),
                                // TODO replace with draft tap sound
                                strings::sounds::SoundId::BLENDER);

        if (hasWork.is_work_complete()) {
            hasWork.reset_pct();
            _add_ingredient_to_drink_NO_VALIDATION(drink, Ingredient::Beer);
        }
        return;
    };

    auto _process_add_ingredient = [&]() {
        CanHoldItem& playerCHI = player.get<CanHoldItem>();
        // not holding anything
        if (playerCHI.empty()) return;
        OptEntity item_opt = playerCHI.item();
        if (!item_opt) {
            playerCHI.update(nullptr, player.id);
            return;
        }
        Item& item = item_opt.asE();
        // not holding item that adds ingredients
        if (item.is_missing<AddsIngredient>()) return;
        const AddsIngredient& addsIG = item.get<AddsIngredient>();

        bool valid = addsIG.validate(drink);
        if (!valid) return;

        IngredientBitSet ingredients = addsIG.get(drink);
        const IsDrink& isdrink = drink.get<IsDrink>();

        bool any = false;
        bitset_utils::for_each_enabled_bit(ingredients, [&](size_t bit) {
            Ingredient ig = magic_enum::enum_value<Ingredient>(bit);
            any |= isdrink.can_add(ig);
            return bitset_utils::ForEachFlow::NormalFlow;
        });
        if (!any) return;

        const float amt = 1.f;
        hasWork.increase_pct(amt * dt);
        if (hasWork.is_work_complete()) {
            hasWork.reset_pct();

            bool cleaned_up = _add_item_to_drink_NO_VALIDATION(drink, item);
            if (cleaned_up) playerCHI.update(nullptr, entity_id::INVALID);
        }
    };

    _process_if_beer_tap();
    _process_add_ingredient();
}

void make_champagne(Item& alc, vec3 pos) {
    make_item(alc, {.type = EntityType::Champagne}, pos);

    alc.addComponent<HasFishingGame>();

    alc.addComponent<AddsIngredient>(
           [](const Entity&, const Entity&) -> IngredientBitSet {
               return IngredientBitSet().reset().set(Ingredient::Champagne);
           })
        .set_validator([](const Entity& bottle, const Entity&) -> bool {
            // Only allow adding the ingredient if you opened the bottle
            return bottle.get<HasFishingGame>().has_score();
        })
        .set_parent(alc.id)
        .set_num_uses(3);

    alc.addComponent<HasDynamicModelName>().init(
        EntityType::Champagne, HasDynamicModelName::DynamicType::Ingredients,
        [](const Item& owner, const std::string&) -> std::string {
            return owner.get<HasFishingGame>().has_score() ? "champagne_open"
                                                           : "champagne";
        });
}

void make_alcohol(Item& alc, vec3 pos, int index) {
    make_item(alc, {.type = EntityType::Alcohol}, pos);

    // TODO have to change this to just be 0>size
    alc.addComponent<HasSubtype>((int) ingredient::AlcoholsInCycle[0],
                                 (int) ingredient::AlcoholsInCycle[0] +
                                     (int) ingredient::AlcoholsInCycle.size(),
                                 index);
    alc.addComponent<AddsIngredient>(
           [](const Entity& alcohol, const Entity&) -> IngredientBitSet {
               const HasSubtype& hst = alcohol.get<HasSubtype>();
               return IngredientBitSet().reset().set(get_ingredient_from_index(
                   ingredient::AlcoholsInCycle[0] + hst.get_type_index()));
           })
        .set_parent(alc.id)
        .set_num_uses(1);

    alc.addComponent<HasDynamicModelName>().init(
        EntityType::Alcohol, HasDynamicModelName::DynamicType::Subtype,
        [](const Item& owner, const std::string&) -> std::string {
            const HasSubtype& hst = owner.get<HasSubtype>();
            Ingredient bottle = get_ingredient_from_index(
                ingredient::AlcoholsInCycle[0] + hst.get_type_index());
            return util::toLowerCase(magic_enum::enum_name<Ingredient>(bottle));
        });
}

void make_simple_syrup(Item& simple_syrup, vec3 pos) {
    make_item(simple_syrup, {.type = EntityType::SimpleSyrup}, pos);

    simple_syrup
        .addComponent<AddsIngredient>(
            [](const Entity&, Entity&) -> IngredientBitSet {
                return IngredientBitSet().reset().set(Ingredient::SimpleSyrup);
            })
        .set_parent(simple_syrup.id)
        .set_num_uses(-1);

    // Since theres only one of these and its inf uses, dont let it get deleted
    simple_syrup.get<IsItem>()
        .clear_hb_filter()
        .set_hb_filter(ETS_NON_DESTRUCTIVE)
        // TODO create a class of objects that "do work"
        .remove_hb_filter(EntityType::Blender);
}

void make_juice(Item& juice, vec3 pos, Ingredient fruit) {
    make_item(juice, {.type = EntityType::FruitJuice}, pos);

    juice
        .addComponent<AddsIngredient>(
            [fruit](const Entity&, const Entity&) -> IngredientBitSet {
                return IngredientBitSet().reset().set(
                    ingredient::BlendConvert.at(fruit));
            })
        .set_parent(juice.id)
        .set_num_uses(1);

    juice.addComponent<HasDynamicModelName>().init(
        EntityType::FruitJuice, HasDynamicModelName::DynamicType::Subtype,
        [fruit](const Item&, const std::string&) -> std::string {
            return util::convertToSnakeCase<Ingredient>(
                ingredient::BlendConvert.at(fruit));
        });
}

void make_fruit(Item& fruit, vec3 pos, int index) {
    make_item(fruit, {.type = EntityType::Fruit}, pos);

    fruit.addComponent<HasSubtype>(0, (int) ingredient::Fruits.size(), index);

    fruit
        .addComponent<AddsIngredient>(
            [](const Entity& fruit, const Entity&) -> IngredientBitSet {
                const HasSubtype& hst = fruit.get<HasSubtype>();
                return IngredientBitSet().reset().set(
                    ingredient::Fruits[0 + hst.get_type_index()]);
            })
        .set_parent(fruit.id)
        .set_num_uses(1);

    fruit.addComponent<HasDynamicModelName>().init(
        EntityType::Fruit, HasDynamicModelName::DynamicType::Subtype,
        [](const Item& owner, const std::string&) -> std::string {
            const HasSubtype& hst = owner.get<HasSubtype>();
            Ingredient fruit = ingredient::Fruits[0 + hst.get_type_index()];
            return util::toLowerCase(magic_enum::enum_name<Ingredient>(fruit));
        });
    fruit.addComponent<HasWork>().init([](Entity& owner, HasWork& hasWork,
                                          Entity& /* person */, float dt) {
        if (SystemManager::get().is_bar_open()) return;
        const IsItem& ii = owner.get<IsItem>();
        const HasSubtype& hasSubtype = owner.get<HasSubtype>();
        Ingredient fruit_type = ingredient::Fruits[hasSubtype.get_type_index()];

        // Can only handle blendables right now
        if (!ingredient::BlendConvert.contains(fruit_type)) return;

        if (ii.is_not_held_by(EntityType::Blender)) {
            hasWork.reset_pct();
            return;
        }

        server_only::play_sound(owner.get<Transform>().as2(),
                                strings::sounds::SoundId::BLENDER);

        const float amt = 0.75f;
        hasWork.increase_pct(amt * dt);
        if (hasWork.is_work_complete()) {
            hasWork.reset_pct();

            OptEntity blender = EntityHelper::getEntityForID(ii.holder());
            if (!blender) {
                log_warn(
                    "trying to spawn juice but the fruit is not being held by "
                    "a valid entity");
                return;
            }

            // delete fruit
            owner.cleanup = true;

            // create juice
            auto& juice = EntityHelper::createEntity();
            make_juice(juice, owner.get<Transform>().pos(), fruit_type);

            CanHoldItem& blenderCHI = blender->get<CanHoldItem>();
            blenderCHI.update(juice, blender->id);
        }
    });
}

void make_drink(Item& drink, vec3 pos) {
    make_item(drink, {.type = EntityType::Drink}, pos);

    drink.addComponent<IsDrink>();
    drink.addComponent<HasWork>().init(std::bind(
        process_drink_working, std::placeholders::_1, std::placeholders::_2,
        std::placeholders::_3, std::placeholders::_4));

    drink.addComponent<HasDynamicModelName>().init(
        EntityType::Drink, HasDynamicModelName::DynamicType::Ingredients,
        [](const Item& owner, const std::string&) -> std::string {
            const IsDrink& isdrink = owner.get<IsDrink>();
            constexpr auto drinks = magic_enum::enum_values<Drink>();
            for (Drink d : drinks) {
                if (isdrink.matches_drink(d))
                    return get_model_name_for_drink(d);
            }
            return util::convertToSnakeCase<EntityType>(EntityType::Drink);
        });
}

void make_pitcher(Item& pitcher, vec3 pos) {
    make_item(pitcher, {.type = EntityType::Pitcher}, pos);

    pitcher.addComponent<IsDrink>().turn_on_support_multiple(10);
    pitcher
        .addComponent<AddsIngredient>(
            [](const Entity& pitcher, Entity&) -> IngredientBitSet {
                return pitcher.get<IsDrink>().ing();
            })
        .set_parent(pitcher.id)
        .set_validator([](const Entity& pitcher, const Entity& drink) -> bool {
            if (drink.is_missing<IsDrink>()) return false;
            const IsDrink& into_isdrink = drink.get<IsDrink>();
            // Only add if its empty
            if (into_isdrink.has_anything()) return false;

            if (pitcher.is_missing<IsDrink>()) return false;
            const IsDrink& isdrink = pitcher.get<IsDrink>();
            return isdrink.get_num_complete() > 0;
        })
        .set_on_decrement([](Entity& pitcher) {
            pitcher.get<IsDrink>().remove_one_completed();
        })
        // we dont want to have the cup deleted
        .set_num_uses(-1);

    pitcher.addComponent<HasWork>().init(std::bind(
        process_drink_working, std::placeholders::_1, std::placeholders::_2,
        std::placeholders::_3, std::placeholders::_4));

    pitcher.addComponent<HasDynamicModelName>().init(
        EntityType::Pitcher, HasDynamicModelName::DynamicType::Ingredients,
        [](const Item& owner, const std::string&) -> std::string {
            const IsDrink& isdrink = owner.get<IsDrink>();
            constexpr auto drinks = magic_enum::enum_values<Drink>();
            for (Drink d : drinks) {
                if (isdrink.matches_drink(d))
                    return get_model_name_for_drink(d);
            }
            // TODO uhhhhhh add a new cup type for this eventually
            return util::convertToSnakeCase<EntityType>(EntityType::DraftTap);
        });
}

void make_item_type(Item& item, EntityType type, vec3 location, int index) {
    // log_info("generating new item {} of type {} at {} subtype{}", item.id,
    // type_name, pos, index);

    vec3 pos = (location);

    switch (type) {
        case EntityType::SodaSpout:
            return make_soda_spout(item, pos);
        case EntityType::Champagne:
            return make_champagne(item, pos);
        case EntityType::Alcohol:
            return make_alcohol(item, pos, index);
        case EntityType::Fruit:
            return make_fruit(item, pos, index);
        case EntityType::Drink:
            return make_drink(item, pos);
        case EntityType::Pitcher:
            return make_pitcher(item, pos);
        case EntityType::Mop:
            return make_mop(item, pos);
        case EntityType::MopBuddy:
            return make_mop_buddy(item, pos);
        case EntityType::SimpleSyrup:
            return make_simple_syrup(item, pos);
            //
        case EntityType::Unknown:
        case EntityType::x:
        case EntityType::y:
        case EntityType::z:
        case EntityType::RemotePlayer:
        case EntityType::Player:
        case EntityType::Customer:
        case EntityType::Table:
        case EntityType::CharacterSwitcher:
        case EntityType::MapRandomizer:
        case EntityType::Wall:
        case EntityType::Conveyer:
        case EntityType::Grabber:
        case EntityType::Register:
        case EntityType::SingleAlcohol:
        case EntityType::AlcoholCabinet:
        case EntityType::FruitBasket:
        case EntityType::TriggerArea:
        case EntityType::FloorMarker:
        case EntityType::CustomerSpawner:
        case EntityType::Sophie:
        case EntityType::Blender:
        case EntityType::SodaMachine:
        case EntityType::Cupboard:
        case EntityType::PitcherCupboard:
        case EntityType::Squirter:
        case EntityType::FilteredGrabber:
        case EntityType::PnumaticPipe:
        case EntityType::Vomit:
        case EntityType::MopHolder:
        case EntityType::FastForward:
        case EntityType::MopBuddyHolder:
        case EntityType::SimpleSyrupHolder:
        case EntityType::IceMachine:
        case EntityType::Face:
        case EntityType::DraftTap:
        case EntityType::Toilet:
        case EntityType::Guitar:
        case EntityType::ChampagneHolder:
        case EntityType::Jukebox:
        case EntityType::AITargetLocation:
        case EntityType::InteractiveSettingChanger:
        case EntityType::Door:
        case EntityType::SodaFountain:
        case EntityType::FruitJuice:
        case EntityType::HandTruck:
        case EntityType::Trash:
            break;
    }
    log_error(
        "Trying to make item with item type {} but not handled in "
        "make_item_type() - this indicates a serialization bug or incorrect "
        "entity setup",
        util::convertToSnakeCase<EntityType>(type));
}

}  // namespace items

void make_customer(Entity& customer, const SpawnInfo& info, bool has_order) {
    make_aiperson(customer, DebugOptions{.type = EntityType::Customer},
                  vec::to3(info.location));

    customer.get<UsesCharacterModel>().switch_to_random_model();
    customer.addComponent<HasName>().update(get_random_name());

    const Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
    const IsRoundSettingsManager& irsm = sophie.get<IsRoundSettingsManager>();

    customer.addComponent<IsCustomer>();
    customer.get<IsAIControlled>()
        .set_initial_state(IsAIControlled::State::QueueForRegister)
        .set_ability_state(
            IsAIControlled::AbilityUseBathroom,
            irsm.has_upgrade_unlocked(UpgradeClass::UnlockToilet))
        .set_ability_state(IsAIControlled::AbilityPlayJukebox,
                           irsm.has_upgrade_unlocked(UpgradeClass::Jukebox));

    // TODO for now, eventually move to customer spawner
    if (has_order) {
        CanOrderDrink& cod = customer.addComponent<CanOrderDrink>();
        // If we are the first guy spawned this round, force the drink to be the
        // most recently unlocked one
        if (info.is_first_this_round) {
            const IsProgressionManager& ipp =
                sophie.get<IsProgressionManager>();

            cod.set_first_order(ipp.get_last_unlocked());
        }
    }

    customer.addComponent<HasPatience>().update_max(20.f);

    customer.addComponent<HasSpeechBubble>();

    const auto debug_mode_on = globals::debug_ui_enabled();
    customer.get<HasBaseSpeed>().update(debug_mode_on ? 20.f : 5.f);

    // TODO if we do dirty-cups, we should have people leave them on any flat
    // surface but if they are too drunk... just smash on the ground
    customer
        .addComponent<IsSpawner>()  //
        .set_fn(&furniture::make_vomit)
        .set_spawn_sound(strings::sounds::SoundId::VOMIT)
        .set_validation_fn([](Entity& entity, const SpawnInfo&) {
            const CanOrderDrink& cod = entity.get<CanOrderDrink>();
            // not vomiting since didnt have anything to drink yet

            if (cod.num_drinks_drank() <= 0) return false;
            if (cod.num_alcoholic_drinks_drank() <= 0) return false;

            const Entity& sophie =
                EntityHelper::getNamedEntity(NamedEntity::Sophie);
            const IsRoundSettingsManager& irsm =
                sophie.get<IsRoundSettingsManager>();

            float vomit_amount_multiplier =
                irsm.get<float>(ConfigKey::VomitAmountMultiplier);
            float vomit_freq_multiplier =
                irsm.get<float>(ConfigKey::VomitFreqMultiplier);

            IsSpawner& vom_spewer = entity.get<IsSpawner>();
            vom_spewer.set_total(static_cast<int>(
                cod.num_alcoholic_drinks_drank() * vomit_amount_multiplier));
            vom_spewer.set_time_between(5.f * vomit_freq_multiplier);

            bool should_vomit = true;

            // Before we return true, should we vomit in a toilet
            if (irsm.has_upgrade_unlocked(UpgradeClass::PottyProtocol)) {
                // Are there any toilets?

                OptEntity closest_available_toilet =
                    EntityQuery()
                        .whereHasComponent<IsToilet>()
                        .whereLambda([](const Entity& entity) {
                            const IsToilet& toilet = entity.get<IsToilet>();
                            return toilet.available();
                        })
                        .orderByDist(entity.get<Transform>().as2())
                        .gen_first();

                // We found an empty toilet, go go go
                if (closest_available_toilet) {
                    vom_spewer.post_spawn_reset();
                    // TODO probably also lower max?

                    should_vomit = false;
                    // TODO make the person go to the bathroom isntead
                    //
                    // not really a TODO but if the person is outside at the
                    // despawn postition and needs to vomit and the bathroom is
                    // full or in use then they will vomit over there, probably
                    // we'd want them to walk to the bathroom first to check so
                    // they vomit closer? but then now we have to have some
                    // kinda 3 strike system or something idk
                    log_warn(
                        "I was gonna vomit but seeing that the bathroom is "
                        "empty, actually ive decided against it PLS FIX");
                }
            }
            return should_vomit;
        })
        // check if there is already vomit in that spot
        .enable_prevent_duplicates()
        // This has to be 1 so that the above validation function runs at least
        // once
        .set_total(1)
        .set_time_between(5.f);
}

namespace furniture {
void make_customer_spawner(Entity& customer_spawner, vec3 pos) {
    make_entity(customer_spawner, {EntityType::CustomerSpawner}, pos);

    const auto sfn = std::bind(&make_customer, std::placeholders::_1,
                               std::placeholders::_2, true);

    customer_spawner.addComponent<SimpleColoredBoxRenderer>()
        .update_face(PINK)
        .update_base(PINK);

    customer_spawner.addComponent<IsSpawner>()
        .set_fn(sfn)
        .set_total(2)
        .set_time_between(5.f)
        .enable_show_progress();

    customer_spawner.addComponent<HasProgression>();
}

void make_champagne_holder(Entity& container, vec2 pos) {
    furniture::make_itemcontainer(container, {EntityType::ChampagneHolder}, pos,
                                  EntityType::Champagne);
}

void make_jukebox(Entity& jukebox, vec2 pos) {
    furniture::make_furniture(jukebox, {EntityType::Jukebox}, pos);
    jukebox.addComponent<HasWaitingQueue>();
    jukebox.addComponent<HasLastInteractedCustomer>();
}

void make_interactive_settings_changet(
    Entity& isc, vec2 pos, CanChangeSettingsInteractively::Style style) {
    furniture::make_furniture(isc, {EntityType::InteractiveSettingChanger}, pos,
                              PINK, PINK,
                              // we make this static so its not highlightable
                              // but we want it to be a little
                              true);

    isc.addComponent<HasName>();
    isc.addComponent<CanChangeSettingsInteractively>(style);

    isc.addComponent<HasWork>().init(
        [](Entity& isc, HasWork& hasWork, Entity& /*player*/, float dt) {
            const float amt = 2.f;
            hasWork.increase_pct(amt * dt);
            if (!hasWork.is_work_complete()) return;
            hasWork.reset_pct();

            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            IsRoundSettingsManager& irsm = sophie.get<IsRoundSettingsManager>();

            auto style = isc.get<CanChangeSettingsInteractively>().style;

            switch (style) {
                case CanChangeSettingsInteractively::ToggleIsTutorial: {
                    irsm.interactive_settings.is_tutorial_active =
                        !irsm.interactive_settings.is_tutorial_active;
                } break;
                case CanChangeSettingsInteractively::Unknown:
                    break;
            }
        });
}

}  // namespace furniture

bool convert_to_type(const EntityType& entity_type, Entity& entity,
                     vec2 location) {
    // TODO at some point just change all of these to match
    auto pos = vec::to3(location);
    switch (entity_type) {
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
        case EntityType::Door: {
            const auto d_color = Color{155, 75, 0, 255};
            (furniture::make_door(entity, location, d_color));
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
        case EntityType::ChampagneHolder: {
            furniture::make_champagne_holder(entity, location);
        } break;
        case EntityType::AlcoholCabinet: {
            furniture::make_medicine_cabinet(entity, location);
        } break;
        case EntityType::FruitBasket: {
            furniture::make_fruit_basket(entity, location);
        } break;
        case EntityType::Blender: {
            furniture::make_blender(entity, location);
        } break;
        case EntityType::SodaMachine: {
            furniture::make_soda_machine(entity, location);
        } break;
        case EntityType::SodaFountain: {
            furniture::make_soda_fountain(entity, location);
        } break;
        case EntityType::DraftTap: {
            furniture::make_draft(entity, location);
        } break;
        case EntityType::Cupboard: {
            furniture::make_cupboard(entity, location);
        } break;
        case EntityType::PitcherCupboard: {
            furniture::make_cupboard(entity, location, 1);
        } break;
        case EntityType::Squirter: {
            furniture::make_squirter(entity, location);
        } break;
        case EntityType::Trash: {
            furniture::make_trash(entity, location);
        } break;
        case EntityType::Toilet: {
            furniture::make_toilet(entity, location);
        } break;
        case EntityType::Guitar: {
            furniture::make_guitar(entity, location);
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
        case EntityType::MopBuddyHolder: {
            furniture::make_mopbuddy_holder(entity, location);
        } break;
        case EntityType::FastForward: {
            furniture::make_fast_forward(entity, location);
        } break;
        case EntityType::SimpleSyrupHolder: {
            furniture::make_simple_syrup_holder(entity, location);
        } break;
        case EntityType::IceMachine: {
            furniture::make_ice_machine(entity, location);
        } break;
        case EntityType::Face: {
            make_face(entity, pos);
        } break;
        case EntityType::Jukebox: {
            furniture::make_jukebox(entity, location);
        } break;
        case EntityType::HandTruck: {
            make_hand_truck(entity, location);
        } break;

        // These return false
        case EntityType::Unknown:
        case EntityType::x:
        case EntityType::y:
        case EntityType::z:
            make_entity(entity, DebugOptions{.type = entity_type}, pos);
            return false;
        case EntityType::Sophie: {
            furniture::make_sophie(entity, pos);
            return false;
        } break;

        case EntityType::CustomerSpawner: {
            furniture::make_customer_spawner(entity, pos);
        } break;

        case EntityType::AITargetLocation: {
            make_ai_target_location(entity, pos);
        } break;
        case EntityType::InteractiveSettingChanger: {
            log_warn(
                "You should call 'make_interactive_setting_changer() manually "
                "instead of using convert to type");
            return false;
        } break;

        // TODO is anyone even doing this?
        case EntityType::SimpleSyrup: {
            items::make_simple_syrup(entity, pos);
        } break;
        case EntityType::MopBuddy: {
            make_mop_buddy(entity, pos);
            return false;
        } break;
        case EntityType::SingleAlcohol:
        case EntityType::TriggerArea:
        case EntityType::FloorMarker:
        case EntityType::Vomit:
        case EntityType::SodaSpout:
        case EntityType::Drink:
        case EntityType::Alcohol:
        case EntityType::Customer:
        case EntityType::Fruit:
        case EntityType::FruitJuice:
        case EntityType::Mop:
        case EntityType::Pitcher:
        case EntityType::Champagne:
            log_warn("{} cant be created through 'convert_to_type'",
                     entity_type);
            return false;
            break;
    }
    if (entity.has<CanHoldItem>()) {
        if (entity.get<CanHoldItem>().hb_type() == EntityType::Unknown) {
            log_warn(
                "Created an entity with canhold item {} but didnt set heldby "
                "type",
                entity_type);
        }
    }

    if (get_entity_type(entity) == EntityType::Unknown) {
        log_error(
            "Created an entity but somehow didnt get a type {}"
            "type",
            entity_type);
    }

    return true;
}
