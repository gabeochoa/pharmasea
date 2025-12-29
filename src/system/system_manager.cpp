

#include "system_manager.h"

#include "afterhours_systems.h"
#include "entity.h"
#include "store_management_helpers.h"

///
#include "../building_locations.h"
#include "../components/adds_ingredient.h"
#include "../components/ai_wait_in_queue.h"
#include "../components/base_component.h"
#include "../components/can_be_ghost_player.h"
#include "../components/can_be_held.h"
#include "../components/can_be_highlighted.h"
#include "../components/can_be_pushed.h"
#include "../components/can_be_taken_from.h"
#include "../components/can_grab_from_other_furniture.h"
#include "../components/can_highlight_others.h"
#include "../components/can_hold_furniture.h"
#include "../components/can_hold_handtruck.h"
#include "../components/can_hold_item.h"
#include "../components/can_order_drink.h"
#include "../components/can_perform_job.h"
#include "../components/collects_user_input.h"
#include "../components/conveys_held_item.h"
#include "../components/custom_item_position.h"
#include "../components/has_base_speed.h"
#include "../components/has_client_id.h"
#include "../components/has_dynamic_model_name.h"
#include "../components/has_fishing_game.h"
#include "../components/has_name.h"
#include "../components/has_patience.h"
#include "../components/has_progression.h"
#include "../components/has_rope_to_item.h"
#include "../components/has_speech_bubble.h"
#include "../components/has_subtype.h"
#include "../components/has_waiting_queue.h"
#include "../components/has_work.h"
#include "../components/indexer.h"
#include "../components/is_bank.h"
#include "../components/is_drink.h"
#include "../components/is_free_in_store.h"
#include "../components/is_item.h"
#include "../components/is_item_container.h"
#include "../components/is_nux_manager.h"
#include "../components/is_pnumatic_pipe.h"
#include "../components/is_progression_manager.h"
#include "../components/is_rotatable.h"
#include "../components/is_round_settings_manager.h"
#include "../components/is_snappable.h"
#include "../components/is_solid.h"
#include "../components/is_spawner.h"
#include "../components/is_squirter.h"
#include "../components/is_store_spawned.h"
#include "../components/is_toilet.h"
#include "../components/is_trigger_area.h"
#include "../components/model_renderer.h"
#include "../components/responds_to_day_night.h"
#include "../components/responds_to_user_input.h"
#include "../components/simple_colored_box_renderer.h"
#include "../components/transform.h"
#include "../components/uses_character_model.h"
#include "../dataclass/upgrades.h"
#include "../engine/util.h"
#include "raylib.h"
#include "sophie.h"
///
#include "../engine/pathfinder.h"
#include "../engine/tracy.h"
#include "../entity_helper.h"
#include "../entity_query.h"
#include "../map.h"
#include "../network/server.h"
#include "ai_system.h"
#include "ingredient_helper.h"
#include "input_process_manager.h"
#include "magic_enum/magic_enum.hpp"
#include "progression.h"
#include "rendering_system.h"
#include "ui_rendering_system.h"

namespace system_manager {

vec3 get_new_held_position_custom(Entity& entity);
vec3 get_new_held_position_default(Entity& entity);

namespace store {
void move_purchased_furniture();
}  // namespace store

void move_player_SERVER_ONLY(Entity& entity, game::State location) {
    if (!is_server()) {
        log_warn(
            "you are calling a server only function from a client context, "
            "this is best case a no-op and worst case a visual desync");
    }

    vec3 position;
    switch (location) {
        case game::Paused:  // fall through
        case game::InMenu:
            return;
            break;
        case game::Lobby: {
            position = LOBBY_BUILDING.to3();
        } break;
        case game::InGame: {
            OptEntity spawn_area = EntityHelper::getMatchingFloorMarker(
                IsFloorMarker::Planning_SpawnArea);
            if (!spawn_area) {
                position = {0, 0, 0};
            } else {
                // this is a guess based off the current size of the trigger
                // area
                // TODO read the actual size?
                // TODO validate nothing is already there
                vec2 pos = spawn_area.asE().get<Transform>().as2();
                position = {pos.x, 0, pos.y + 3};
            }
        } break;
        case game::ModelTest: {
            position = MODEL_TEST_BUILDING.to3();
        } break;
        case game::LoadSaveRoom: {
            position = LOAD_SAVE_BUILDING.to3();
        } break;
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

    server->send_player_location_packet(client_id, position, transform.facing,
                                        entity.get<HasName>().name());
}

template<typename... TArgs>
void backfill_empty_container(const EntityType& match_type, Entity& entity,
                              vec3 pos, TArgs&&... args) {
    if (entity.is_missing<IsItemContainer>()) return;
    IsItemContainer& iic = entity.get<IsItemContainer>();
    if (iic.type() != match_type) return;
    CanHoldItem& canHold = entity.get<CanHoldItem>();
    if (canHold.is_holding_item()) return;

    if (iic.hit_max()) return;
    iic.increment();

    // create item
    Entity& item =
        EntityHelper::createItem(iic.type(), pos, std::forward<TArgs>(args)...);
    // ^ cannot be const because converting to SharedPtr v

    // TODO do we need shared pointer here? (vs just id?)
    canHold.update(EntityHelper::getEntityAsSharedPtr(item), entity.id);
}

void fix_container_item_type(Entity& entity) {
    if (entity.is_missing<IsItemContainer>()) return;

    IsItemContainer& iic = entity.get<IsItemContainer>();
    EntityType entity_type = get_entity_type(entity);

    // Map container entity types to the item types they should hold
    switch (entity_type) {
        case EntityType::FruitBasket:
            iic.set_item_type(EntityType::Fruit).set_uses_indexer(true);
            break;
        case EntityType::MopHolder:
            iic.set_item_type(EntityType::Mop).set_max_generations(1);
            break;
        case EntityType::MopBuddyHolder:
            iic.set_item_type(EntityType::MopBuddy);
            break;
        case EntityType::SimpleSyrupHolder:
            iic.set_item_type(EntityType::SimpleSyrup);
            break;
        case EntityType::ChampagneHolder:
            iic.set_item_type(EntityType::Champagne);
            break;
        case EntityType::AlcoholCabinet:
            iic.set_item_type(EntityType::Alcohol).set_uses_indexer(true);
            break;
        case EntityType::Cupboard:
            iic.set_item_type(EntityType::Pitcher);
            break;
        case EntityType::PitcherCupboard:
            iic.set_item_type(EntityType::Pitcher);
            break;
        case EntityType::SodaMachine:
            iic.set_item_type(EntityType::SodaSpout);
            break;
        case EntityType::Blender:
            iic.set_item_type(EntityType::Drink);
            break;
        case EntityType::DraftTap:
            iic.set_item_type(EntityType::Alcohol);
            break;
        case EntityType::Trash:
            iic.set_item_type(EntityType::Trash);
            break;
        default:
            // Unknown container type, leave as Unknown
            break;
    }
}

void process_is_container_and_should_backfill_item(Entity& entity, float) {
    if (entity.is_missing<IsItemContainer>()) return;
    const IsItemContainer& iic = entity.get<IsItemContainer>();

    if (entity.is_missing<CanHoldItem>()) return;
    const CanHoldItem& canHold = entity.get<CanHoldItem>();
    if (canHold.is_holding_item()) return;

    auto pos = entity.get<Transform>().pos();

    if (iic.should_use_indexer() && entity.has<Indexer>()) {
        Indexer& indexer = entity.get<Indexer>();

        // TODO :backfill-correct: This should match whats in container_haswork

        // TODO For now we are okay doing this because the other indexer
        // (alcohol) always unlocks rum first which is index 0. if that changes
        // we gotta update this
        //  --> should we just add an assert here so we catch it quickly?
        if (check_type(entity, EntityType::FruitBasket)) {
            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            const IsProgressionManager& ipp =
                sophie.get<IsProgressionManager>();

            indexer.increment_until_valid([&](int index) {
                return !ipp.is_ingredient_locked(ingredient::Fruits[index]);
            });
        }

        backfill_empty_container(iic.type(), entity, pos, indexer.value());
        entity.get<Indexer>().mark_change_completed();
        return;
    }

    backfill_empty_container(iic.type(), entity, pos);
}

void process_is_container_and_should_update_item(Entity& entity, float) {
    if (entity.is_missing<IsItemContainer>()) return;
    const IsItemContainer& iic = entity.get<IsItemContainer>();

    if (!iic.should_use_indexer()) return;
    if (entity.is_missing<Indexer>()) return;
    Indexer& indexer = entity.get<Indexer>();
    // user didnt change the index so we are good to wait
    if (indexer.value_same_as_last_render()) return;

    if (entity.is_missing<CanHoldItem>()) return;
    CanHoldItem& canHold = entity.get<CanHoldItem>();

    // Delete the currently held item
    if (canHold.is_holding_item()) {
        canHold.item().cleanup = true;
        canHold.update(nullptr, -1);
    }

    auto pos = entity.get<Transform>().pos();
    backfill_empty_container(iic.type(), entity, pos, indexer.value());
    indexer.mark_change_completed();
}

void process_is_indexed_container_holding_incorrect_item(Entity& entity,
                                                         float) {
    // This function is for when you have an indexed container and you put the
    // item back in but the index had changed.
    //
    // in this case we need to clear the item because otherwise they will both
    // live there which will cause overlap and grab issues.

    if (entity.is_missing<Indexer>()) return;
    const Indexer& indexer = entity.get<Indexer>();

    if (entity.is_missing<CanHoldItem>()) return;
    CanHoldItem& canHold = entity.get<CanHoldItem>();

    if (canHold.empty()) return;

    int current_value = indexer.value();
    int item_value = canHold.item().get<HasSubtype>().get_type_index();

    if (current_value != item_value) {
        canHold.item().cleanup = true;
        canHold.update(nullptr, -1);
    }
}

void spawn_machines_for_newly_unlocked_drink_DONOTCALL(
    IsProgressionManager& ipm, Drink option) {
    // today we dont have a way to statically know which machines
    // provide which ingredients because they are dynamic
    IngredientBitSet possibleNewIGs = get_req_ingredients_for_drink(option);

    // Because prereqs are handled, we dont need do check for them and can
    // assume that those bits will handle checking for it

    OptEntity spawn_area = EntityHelper::getMatchingFloorMarker(
        // Note we spawn free items in the purchase area so its more obvious
        // that they are free
        IsFloorMarker::Type::Planning_SpawnArea);

    if (!spawn_area) {
        // need to guarantee this exists long before we get here
        log_error("Could not find spawn area entity");
    }

    bitset_utils::for_each_enabled_bit(possibleNewIGs, [&ipm, spawn_area](
                                                           size_t index) {
        Ingredient ig = magic_enum::enum_value<Ingredient>(index);

        const auto make_free_machine = []() -> Entity& {
            auto& entity = EntityHelper::createEntity();
            entity.addComponent<IsFreeInStore>();
            return entity;
        };

        // Already has the machine so we good
        if (IngredientHelper::has_machines_required_for_ingredient(ig))
            return bitset_utils::ForEachFlow::Continue;

        switch (ig) {
            case Soda: {
                // Nothing is needed to do for this since
                // the soda machine is required to play the game
            } break;
            case Beer: {
                auto et = EntityType::DraftTap;
                auto& entity = make_free_machine();
                convert_to_type(et, entity, spawn_area->get<Transform>().as2());
                ipm.unlock_entity(et);
            } break;
            case Champagne: {
                auto et = EntityType::ChampagneHolder;
                auto& entity = make_free_machine();
                convert_to_type(et, entity, spawn_area->get<Transform>().as2());
                ipm.unlock_entity(et);
            } break;
            case Rum:
            case Tequila:
            case Vodka:
            case Whiskey:
            case Gin:
            case TripleSec:
            case Bitters: {
                int alc_index =
                    index_of<Ingredient, ingredient::AlcoholsInCycle.size()>(
                        ingredient::AlcoholsInCycle, ig);
                auto& entity = make_free_machine();
                furniture::make_single_alcohol(
                    entity, spawn_area->get<Transform>().as2(), alc_index);
            } break;
            case Pineapple:
            case Orange:
            case Coconut:
            case Cranberries:
            case Lime:
            case Lemon: {
                auto et = EntityType::FruitBasket;
                auto& entity = make_free_machine();
                convert_to_type(et, entity, spawn_area->get<Transform>().as2());
                ipm.unlock_entity(et);
            } break;
            case PinaJuice:
            case OrangeJuice:
            case CoconutCream:
            case CranberryJuice:
            case LimeJuice:
            case LemonJuice: {
                auto et = EntityType::Blender;
                auto& entity = make_free_machine();
                convert_to_type(et, entity, spawn_area->get<Transform>().as2());

                ipm.unlock_entity(et);
            } break;
            case SimpleSyrup: {
                auto et = EntityType::SimpleSyrupHolder;
                auto& entity = make_free_machine();
                convert_to_type(et, entity, spawn_area->get<Transform>().as2());
                ipm.unlock_entity(et);
            } break;
            case IceCubes:
            case IceCrushed: {
                auto et = EntityType::IceMachine;
                auto& entity = make_free_machine();
                convert_to_type(et, entity, spawn_area->get<Transform>().as2());
                ipm.unlock_entity(et);
            } break;
            case Salt:
            case MintLeaf:
                // TODO implement for these once thye have spawners
                break;
            case Invalid:
                break;
        }
        return bitset_utils::ForEachFlow::NormalFlow;
    });
}

void generate_machines_for_new_upgrades() {
    Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
    IsRoundSettingsManager& irsm = sophie.get<IsRoundSettingsManager>();

    OptEntity purchase_area = EntityHelper::getMatchingFloorMarker(
        IsFloorMarker::Type::Planning_SpawnArea);

    for (EntityType et : irsm.config.store_to_spawn) {
        auto& entity = EntityHelper::createEntity();
        bool success =
            convert_to_type(et, entity, purchase_area->get<Transform>().as2());
        if (!success) {
            entity.cleanup = true;
            log_error(
                "Store spawn of newly unlocked item failed to "
                "generate");
        }
    }
    irsm.config.store_to_spawn.clear();
}

void update_visuals_for_settings_changer(Entity& entity, float) {
    if (entity.is_missing<CanChangeSettingsInteractively>()) return;

    Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
    IsRoundSettingsManager& irsm = sophie.get<IsRoundSettingsManager>();

    auto get_name = [](CanChangeSettingsInteractively::Style style,
                       bool bool_value) {
        auto bool_str = bool_value ? "Enabled" : "Disabled";

        switch (style) {
            case CanChangeSettingsInteractively::ToggleIsTutorial:
                return fmt::format("Tutorial: {}", bool_str);
                break;
            case CanChangeSettingsInteractively::Unknown:
                break;
        }
        log_warn(
            "Tried to get_name for Interactive Setting Style {} but it was "
            "not "
            "handled",
            magic_enum::enum_name<CanChangeSettingsInteractively::Style>(
                style));
        return std::string("");
    };

    auto update_color_for_bool = [](Entity& isc, bool value) {
        // TODO eventually read from theme... (imports)
        // auto color = value ? UI_THEME.from_usage(theme::Usage::Primary)
        // : UI_THEME.from_usage(theme::Usage::Error);

        auto color = value ?  //
                         ::ui::color::green_apple
                           : ::ui::color::red;

        isc.get<SimpleColoredBoxRenderer>()  //
            .update_face(color)
            .update_base(color);
    };

    auto style = entity.get<CanChangeSettingsInteractively>().style;

    switch (style) {
        case CanChangeSettingsInteractively::ToggleIsTutorial: {
            entity.get<HasName>().update(
                get_name(style, irsm.interactive_settings.is_tutorial_active));
            update_color_for_bool(entity,
                                  irsm.interactive_settings.is_tutorial_active);
        } break;
        case CanChangeSettingsInteractively::Unknown:
            break;
    }
}

bool _create_nuxes(Entity&) {
    if (SystemManager::get().is_bar_closed()) return false;

    OptEntity player = EQ(SystemManager::get().oldAll)
                           .whereType(EntityType::Player)
                           .gen_first();
    if (!player.has_value()) return false;
    OptEntity reg = EntityQuery().whereType(EntityType::Register).gen_first();
    if (!reg.has_value()) return false;

    int player_id = player->id;
    int register_id = reg->id;

    // Planning mode tutorial
    if (1) {
        // Find register
        {
            // move the register out to the Planning_SpawnArea
            {
                OptEntity purchase_area = EntityHelper::getMatchingFloorMarker(
                    IsFloorMarker::Type::Planning_SpawnArea);
                reg->get<Transform>().update(
                    vec::to3(purchase_area->get<Transform>().as2()));
            }

            auto& entity = EntityHelper::createEntity();
            make_entity(entity, {EntityType::Unknown}, vec2{0, 0});

            entity.addComponent<IsNux>()
                .should_attach_to(player_id)
                .set_eligibility_fn([](const IsNux&) -> bool { return true; })
                .set_completion_fn([register_id](const IsNux& inux) -> bool {
                    OptEntity reg =
                        EntityQuery().whereID(register_id).gen_first();
                    if (!reg.has_value()) return false;

                    // We have to do oldAll because players
                    // are not in the normal entity list
                    return EQ(SystemManager::get().oldAll)
                        .whereID(inux.entityID)
                        .whereInRange(reg->get<Transform>().as2(), 2.f)
                        .has_values();
                })
                .set_content(TODO_TRANSLATE("Look for the Register",
                                            TodoReason::SubjectToChange));
        }

        // Grab register
        {
            const AnyInputs valid_inputs = KeyMap::get_valid_inputs(
                menu::State::Game, InputName::PlayerHandTruckInteract);
            const auto tex_name = KeyMap::get().icon_for_input(valid_inputs[0]);

            auto& entity = EntityHelper::createEntity();
            make_entity(entity, {EntityType::Unknown}, vec2{0, 0});

            entity.addComponent<IsNux>()
                .should_attach_to(player_id)
                .set_eligibility_fn([](const IsNux&) -> bool { return true; })
                .set_completion_fn([register_id](const IsNux& inux) -> bool {
                    return EQ(SystemManager::get().oldAll)
                        .whereID(inux.entityID)
                        .whereIsHoldingFurnitureID(register_id)
                        .has_values();
                })
                // TODO replace playerpickup with the actual control
                .set_content(
                    TODO_TRANSLATE(fmt::format("Grab it with [{}]", tex_name),
                                   TodoReason::SubjectToChange));
        }

        // Place register
        {
            auto& entity = EntityHelper::createEntity();
            make_entity(entity, {EntityType::Unknown}, vec2{-6.f, 1.f});

            entity.addComponent<IsNux>()
                .set_eligibility_fn([](const IsNux&) -> bool { return true; })
                .set_completion_fn(
                    [register_id, &entity](const IsNux&) -> bool {
                        return EntityQuery()
                            .whereID(register_id)
                            .whereIsNotBeingHeld()
                            .whereSnappedPositionMatches(entity)
                            .has_values();
                    })
                .set_ghost(EntityType::Register)
                .set_content(
                    TODO_TRANSLATE("Place it on the highlighted square",
                                   TodoReason::SubjectToChange));
        }

        // Find FFD
        {
            auto& entity = EntityHelper::createEntity();
            make_entity(entity, {EntityType::Unknown}, vec2{-6.f, 1.f});

            OptEntity ffd =
                EntityQuery().whereType(EntityType::FastForward).gen_first();
            int ffd_id = ffd->id;

            entity.addComponent<IsNux>()
                .should_attach_to(ffd_id)
                .set_eligibility_fn([](const IsNux&) -> bool { return true; })
                .set_completion_fn([ffd_id, player_id](const IsNux&) -> bool {
                    OptEntity ffd = EntityQuery().whereID(ffd_id).gen_first();
                    if (!ffd.has_value()) return false;

                    // We have to do oldAll because players
                    // are not in the normal entity list
                    return EQ(SystemManager::get().oldAll)
                        .whereID(player_id)
                        .whereInRange(ffd->get<Transform>().as2(), 2.f)
                        .has_values();
                })
                .set_content(TODO_TRANSLATE(
                    "You get all night to setup your pub.\nYou can "
                    "use the FastForward Box to skip ahead",
                    TodoReason::SubjectToChange));
        }

        // Use FFD
        {
            const AnyInputs valid_inputs = KeyMap::get_valid_inputs(
                menu::State::Game, InputName::PlayerDoWork);
            const auto tex_name = KeyMap::get().icon_for_input(valid_inputs[0]);

            auto& entity = EntityHelper::createEntity();
            make_entity(entity, {EntityType::Unknown}, vec2{-6.f, 1.f});

            OptEntity ffd =
                EntityQuery().whereType(EntityType::FastForward).gen_first();
            int ffd_id = ffd->id;

            entity.addComponent<IsNux>()
                .should_attach_to(ffd_id)
                .set_eligibility_fn([](const IsNux&) -> bool { return true; })
                .set_completion_fn([](const IsNux&) -> bool {
                    auto e_ht = EntityQuery()
                                    .whereHasComponent<HasDayNightTimer>()
                                    .gen_first();
                    if (!e_ht.has_value()) return false;
                    const HasDayNightTimer& timer =
                        e_ht->get<HasDayNightTimer>();
                    return timer.pct() >= 0.5f;
                })
                .set_content(TODO_TRANSLATE(
                    fmt::format("Use [{}] to skip time", tex_name),
                    TodoReason::SubjectToChange));
        }
    }

    // During round tutorial
    if (1) {
        // Explain customer
        {
            auto& entity = EntityHelper::createEntity();
            make_entity(entity, {EntityType::Unknown}, vec2{-6.f, 1.f});

            entity.addComponent<IsNux>()
                .should_cleanup_on_parent_death()
                .set_eligibility_fn([](const IsNux&) -> bool {
                    // Wait until theres at least one customer
                    return EntityQuery()
                        .whereType(EntityType::Customer)
                        .has_values();
                })
                .set_on_trigger([](IsNux& inux) {
                    // Now that the customer exists, we can attach to it
                    auto customer = EntityQuery()
                                        .whereType(EntityType::Customer)
                                        .gen_first();
                    inux.should_attach_to(customer->id);
                })
                .set_completion_fn([](const IsNux& inux) -> bool {
                    if (inux.time_shown < 5.f) return false;

                    auto customer = EntityHelper::getEntityForID(inux.entityID);
                    if (!customer) return false;

                    AIWaitInQueue& ai_wiq = customer->get<AIWaitInQueue>();
                    return ai_wiq.line_wait.last_line_position == 0;
                })
                .set_content(TODO_TRANSLATE("This is a customer, they will "
                                            "wait in line, \nand once at "
                                            "the front will order a drink",
                                            TodoReason::SubjectToChange));
        }

        // Grab a cup
        {
            auto& entity = EntityHelper::createEntity();
            make_entity(entity, {EntityType::Unknown}, vec2{0, 0});

            entity.addComponent<IsNux>()
                .set_eligibility_fn([](const IsNux&) -> bool {
                    // Wait until theres at least one customer
                    return EntityQuery()
                        .whereType(EntityType::Customer)
                        .has_values();
                })
                .set_on_trigger([](IsNux& inux) {
                    auto cups = EntityQuery()
                                    .whereType(EntityType::Cupboard)
                                    .gen_first();
                    inux.should_attach_to(cups->id);
                })
                .set_completion_fn([player_id](const IsNux&) -> bool {
                    return EQ(SystemManager::get().oldAll)
                        .whereID(player_id)
                        .whereIsHoldingItemOfType(EntityType::Drink)
                        .has_values();
                })
                .set_content(
                    TODO_TRANSLATE("Grab a cup", TodoReason::SubjectToChange));
        }

        {
            auto& entity = EntityHelper::createEntity();
            make_entity(entity, {EntityType::Unknown}, vec2{0, 0});

            entity.addComponent<IsNux>()
                .set_eligibility_fn([](const IsNux&) -> bool {
                    // Wait until theres at least one customer
                    return EntityQuery()
                        .whereType(EntityType::Customer)
                        .has_values();
                })
                .set_on_trigger([](IsNux& inux) {
                    auto table =
                        EntityQuery().whereType(EntityType::Table).gen_first();
                    inux.should_attach_to(table->id);
                })
                .set_completion_fn([](const IsNux&) -> bool {
                    return EntityQuery()
                        // Instead of finding the exact table we marked,
                        // just find any table with a cup
                        .whereType(EntityType::Table)
                        .whereIsHoldingItemOfType(EntityType::Drink)
                        .has_values();
                })
                .set_content(TODO_TRANSLATE("Place it down on a table",
                                            TodoReason::SubjectToChange));
        }

        {
            auto& entity = EntityHelper::createEntity();
            make_entity(entity, {EntityType::Unknown}, vec2{0, 0});

            entity.addComponent<IsNux>()
                .set_eligibility_fn([](const IsNux&) -> bool {
                    // Wait until theres at least one customer
                    return EntityQuery()
                        .whereType(EntityType::Customer)
                        .has_values();
                })
                .set_on_trigger([](IsNux& inux) {
                    auto sodamach = EntityQuery()
                                        .whereType(EntityType::SodaMachine)
                                        .gen_first();
                    inux.should_attach_to(sodamach->id);
                })
                .set_completion_fn([player_id](const IsNux&) -> bool {
                    return EQ(SystemManager::get().oldAll)
                        .whereID(player_id)
                        .whereIsHoldingItemOfType(EntityType::SodaSpout)
                        .has_values();
                })
                .set_content(TODO_TRANSLATE("Grab the soda wand",
                                            TodoReason::SubjectToChange));
        }

        {
            const AnyInputs valid_inputs = KeyMap::get_valid_inputs(
                menu::State::Game, InputName::PlayerDoWork);
            const auto tex_name = KeyMap::get().icon_for_input(valid_inputs[0]);

            auto& entity = EntityHelper::createEntity();
            make_entity(entity, {EntityType::Unknown}, vec2{0, 0});

            entity.addComponent<IsNux>()
                .set_eligibility_fn([](const IsNux&) -> bool {
                    // Wait until theres at least one customer
                    return EntityQuery()
                        .whereType(EntityType::Customer)
                        .has_values();
                })
                .set_on_trigger([](IsNux& inux) {
                    auto drink =
                        EntityQuery().whereType(EntityType::Drink).gen_first();
                    inux.should_attach_to(drink->id);
                })
                .set_completion_fn([](const IsNux& inux) -> bool {
                    return EntityQuery()
                        .whereID(inux.entityID)
                        .whereIsDrinkAndMatches(Drink::coke)
                        .has_values();
                })
                .set_content(TODO_TRANSLATE(
                    fmt::format("Use [{}] to fill the cup with soda", tex_name),
                    TodoReason::SubjectToChange));
        }

        {
            auto& entity = EntityHelper::createEntity();
            make_entity(entity, {EntityType::Unknown}, vec2{0, 0});

            entity.addComponent<IsNux>()
                .set_eligibility_fn([](const IsNux&) -> bool {
                    bool filled_cup_exists =
                        EntityQuery()
                            .whereIsDrinkAndMatches(Drink::coke)
                            .has_values();

                    bool player_holding_spout =
                        EQ(SystemManager::get().oldAll)
                            .whereType(EntityType::Player)
                            .whereHasComponent<CanHoldItem>()
                            .whereIsHoldingItemOfType(EntityType::SodaSpout)
                            .has_values();
                    return filled_cup_exists && player_holding_spout;
                })
                .set_on_trigger([](IsNux& inux) {
                    auto sodamach = EntityQuery()
                                        .whereType(EntityType::SodaMachine)
                                        .gen_first();
                    inux.should_attach_to(sodamach->id);
                })
                .set_completion_fn([](const IsNux&) -> bool {
                    // No players holding spouts anymore
                    return EQ(SystemManager::get().oldAll)
                        .whereType(EntityType::Player)
                        .whereIsHoldingItemOfType(EntityType::SodaSpout)
                        .is_empty();
                })
                .set_content(TODO_TRANSLATE("Place the soda wand back down",
                                            TodoReason::SubjectToChange));
        }

        {
            auto& entity = EntityHelper::createEntity();
            make_entity(entity, {EntityType::Unknown}, vec2{0, 0});

            entity.addComponent<IsNux>()
                .set_eligibility_fn([](const IsNux&) -> bool {
                    // Wait until theres at least one customer
                    return EntityQuery()
                        .whereType(EntityType::Customer)
                        .has_values();
                })
                .set_on_trigger([](IsNux& inux) {
                    auto reg = EntityQuery()
                                   .whereType(EntityType::Register)
                                   .gen_first();
                    inux.should_attach_to(reg->id);
                })
                .set_completion_fn([](const IsNux&) -> bool {
                    return EntityQuery()
                        .whereType(EntityType::Register)
                        .whereIsHoldingItemOfType(EntityType::Drink)
                        .whereHeldItemMatches([](const Entity& item) {
                            if (!item.hasTag(EntityType::Drink)) return false;
                            return item.get<IsDrink>().matches_drink(
                                Drink::coke);
                        })
                        .has_values();
                })
                .set_content(TODO_TRANSLATE(
                    "Place it on the register to serve the customer",
                    TodoReason::SubjectToChange));
        }

        {
            auto& entity = EntityHelper::createEntity();
            make_entity(entity, {EntityType::Unknown}, vec2{0, 0});

            OptEntity ffd =
                EntityQuery().whereType(EntityType::FastForward).gen_first();
            int ffd_id = ffd->id;

            entity.addComponent<IsNux>()
                .should_attach_to(ffd_id)
                .set_eligibility_fn([](const IsNux&) -> bool {
                    if (!GameState::get().is_game_like()) return false;

                    bool has_customers = EntityQuery()
                                             .whereType(EntityType::Customer)
                                             .has_values();
                    if (!has_customers) return false;

                    // TODO :DUPE: used as well for sophie checks
                    const auto endpos = vec2{GATHER_SPOT, GATHER_SPOT};
                    bool all_customers_at_gather =
                        EntityQuery()
                            .whereType(EntityType::Customer)
                            .whereNotInRange(endpos, TILESIZE * 2.f)
                            .is_empty();

                    auto e_ht = EntityQuery()
                                    .whereHasComponent<HasDayNightTimer>()
                                    .gen_first();
                    if (!e_ht.has_value()) return false;
                    const HasDayNightTimer& timer =
                        e_ht->get<HasDayNightTimer>();
                    bool lt_halfway_through_day = timer.pct() <= 0.5f;

                    return all_customers_at_gather && lt_halfway_through_day;
                })
                .set_completion_fn([](const IsNux&) -> bool {
                    // hide when 80% through the day
                    auto e_ht = EntityQuery()
                                    .whereHasComponent<HasDayNightTimer>()
                                    .gen_first();
                    if (!e_ht.has_value()) return false;
                    const HasDayNightTimer& timer =
                        e_ht->get<HasDayNightTimer>();
                    return timer.pct() >= 0.8f;
                })
                .set_content(TODO_TRANSLATE("Since customers are all gone, \n"
                                            "Fast Forward to the next day",
                                            TodoReason::SubjectToChange));
        }

        // this is the upgrade room, you will either get a new recipe or a
        // new gimmic for your restaurant
        //
        // often new upgrades, unlock new furniture. for the ones that are
        // required, you will get one for free
        //
        // why is the "cant start until" showing so early in the day
    }

    log_info("created nuxes");
    return true;
}

void process_nux_updates(Entity& entity, float dt) {
    if (entity.is_missing<IsNuxManager>()) return;

    // Tutorial isnt on so dont do any nuxes
    if (!entity.get<IsRoundSettingsManager>()
             .interactive_settings.is_tutorial_active) {
        return;
    }

    // only generate the nux once you leave the lobby
    if (!GameState::get().is_game_like()) return;

    IsNuxManager& inm = entity.get<IsNuxManager>();
    if (!inm.initialized) {
        bool init = _create_nuxes(entity);
        if (!init) return;
        inm.initialized = init;
    }

    OptEntity active_nux = EntityQuery()
                               .whereHasComponent<IsNux>()
                               .whereLambda([](const Entity& entity) {
                                   return entity.get<IsNux>().is_active;
                               })
                               // there should only ever be one
                               .gen_first();

    // Process updates for current showing nux
    if (active_nux.has_value()) {
        Entity& nux = active_nux.asE();
        IsNux& inux = nux.get<IsNux>();

        inux.pass_time(dt);

        if (inux.whileShowing) inux.whileShowing(inux, dt);

        bool parent_died = false;
        if (inux.cleanup_on_parent_death && inux.entityID != -1) {
            auto exi = EntityHelper::getEntityForID(inux.entityID);
            parent_died = !exi.has_value();
        }

        if (parent_died || inux.isComplete(inux)) {
            nux.cleanup = true;
            inux.is_active = false;
            active_nux = {};
        }
    }

    // if that one is still active, nothing else to do
    if (active_nux.has_value() && active_nux->get<IsNux>().is_active) return;

    // find next active nux
    OptEntity next_active = EntityQuery()
                                .whereHasComponent<IsNux>()
                                .whereLambda([](const Entity& entity) {
                                    const IsNux& inux = entity.get<IsNux>();
                                    return inux.shouldTrigger(inux);
                                })
                                .gen_first();

    // if we found one, then make it active
    if (next_active.has_value()) {
        IsNux& inux = next_active->get<IsNux>();
        if (inux.onTrigger) inux.onTrigger(inux);
        inux.is_active = true;
    }
}

namespace store {
void move_purchased_furniture() {
    // Grab the overlap area so we can see what it marked
    OptEntity purchase_area = EntityHelper::getMatchingFloorMarker(
        IsFloorMarker::Type::Store_PurchaseArea);
    const IsFloorMarker& ifm = purchase_area->get<IsFloorMarker>();

    // Grab the plannig spawn area so we can place in the right spot
    OptEntity spawn_area = EntityHelper::getMatchingFloorMarker(
        IsFloorMarker::Type::Planning_SpawnArea);
    vec3 spawn_position = spawn_area->get<Transform>().pos();

    OptEntity sophie = EntityQuery().whereType(EntityType::Sophie).gen_first();
    VALIDATE(sophie.valid(), "sophie should exist when moving furniture");
    IsBank& bank = sophie->get<IsBank>();

    int amount_in_cart = 0;

    // for every marked, move them over
    for (size_t i = 0; i < ifm.num_marked(); i++) {
        EntityID id = ifm.marked_ids()[i];
        OptEntity marked_entity = EntityHelper::getEntityForID(id);
        if (!marked_entity) continue;

        // Move it to the new spot
        Transform& transform = marked_entity->get<Transform>();
        transform.update(spawn_position);
        transform.update_y(0);

        // Remove the 'store_cleanup' marker
        if (marked_entity->has<IsStoreSpawned>()) {
            marked_entity->removeComponent<IsStoreSpawned>();
        }

        // Some items can hold other items; move the held item with the
        // furniture
        if (marked_entity->has<CanHoldItem>()) {
            CanHoldItem& chi = marked_entity->get<CanHoldItem>();
            if (!chi.empty()) {
                vec3 new_pos =
                    marked_entity->has<CustomHeldItemPosition>()
                        ? get_new_held_position_custom(marked_entity.asE())
                        : get_new_held_position_default(marked_entity.asE());
                chi.item().get<Transform>().update(new_pos);
            }
        }

        // Its not free!
        if (marked_entity->is_missing<IsFreeInStore>()) {
            amount_in_cart +=
                std::max(0, get_price_for_entity_type(
                                get_entity_type(marked_entity.asE())));
        }
    }

    VALIDATE(amount_in_cart == bank.cart(),
             "Amount we computed for in cart should always match the actual "
             "amount in our cart")

    // Commit the withdraw
    bank.withdraw(amount_in_cart);
    bank.update_cart(0);
}

}  // namespace store

}  // namespace system_manager

void SystemManager::for_each_old(const std::function<void(Entity&)>& cb) {
    for (const std::shared_ptr<Entity>& entity : oldAll) {
        if (!entity) continue;
        cb(*entity);
    }
}

void SystemManager::for_each_old(const std::function<void(Entity&)>& cb) const {
    for (const std::shared_ptr<Entity>& entity : oldAll) {
        if (!entity) continue;
        cb(*entity);
    }
}

void SystemManager::on_game_state_change(game::State new_state,
                                         game::State old_state) {
    log_warn("system manager on gamestate change from {} to {}", old_state,
             new_state);

    transitions.emplace_back(std::make_pair(old_state, new_state));
}

void SystemManager::update_all_entities(const Entities& players, float dt) {
    // TODO speed?
    Entities entities;
    Entities ents = EntityHelper::get_entities();

    entities.reserve(players.size() + ents.size());

    // Filter out null and cleanup entities when building the list
    for (const auto& ent : ents) {
        if (ent && !ent->cleanup) {
            entities.push_back(ent);
        }
    }
    for (const auto& player : players) {
        if (player && !player->cleanup) {
            entities.push_back(player);
        }
    }

    oldAll = entities;

    // should we not do any updates for client?
    // any changes will get overwritten by server every frame
    // but maybe itll matter
    //
    // TODO maybe this should only NOT run for the host? since the latency
    // is decent
    //
    // This check might cause lots of issues when high latency
    if (!is_server()) return;

    timePassed += dt;

    // NOTE: Old system functions are now handled by afterhours systems
    // The systems have should_run() methods that conditionally enable them
    // based on game state, matching the original conditional logic.
    //
    // Note: SixtyFpsUpdateSystem runs every frame (not just at 60fps) for
    // better responsiveness, especially for trigger areas. The timePassed
    // accumulator is kept for potential future use but is no longer needed for
    // system timing. We still reset it to maintain the original timing pattern.
    if (timePassed >= 0.016f) {
        timePassed = 0;
    }

    // actual update
    {
        // TODO add num entities to debug overlay
        // log_info("num entities {}", entities.size());

        // NOTE: System updates are now handled by afterhours systems:
        // - ModelTestUpdateSystem (runs when State::ModelTest)
        // - InRoundUpdateSystem (runs when is_game_like() && is_bar_open())
        // - PlanningUpdateSystem (runs when is_game_like() && is_bar_closed())
        // - GameLikeUpdateSystem (runs when is_game_like())
        // - SixtyFpsUpdateSystem (runs in all states)
        // All systems have should_run() methods that match the original
        // conditional logic, so we no longer need the conditional blocks here.
        every_frame_update(entities, dt);
        process_state_change(entities, dt);

        // Run afterhours systems
        // Note: systems.tick() expects a non-const Entities&
        // We use oldAll which contains the same entities but is mutable
        systems.tick(oldAll, dt);
    }
}

void SystemManager::update_remote_players(const Entities& players, float) {
    remote_players = players;
}

void SystemManager::update_local_players(const Entities& players, float dt) {
    local_players = players;
    for (const auto& entity : players) {
        system_manager::input_process_manager::collect_user_input(*entity, dt);
    }
}

void SystemManager::process_inputs(const Entities& entities,
                                   const UserInputs& inputs) {
    for (const auto& entity : entities) {
        if (entity->is_missing<RespondsToUserInput>()) continue;
        for (auto input : inputs) {
            system_manager::input_process_manager::process_input(*entity,
                                                                 input);
        }
    }
}

void SystemManager::process_state_change(
    const std::vector<std::shared_ptr<Entity>>&, float) {
    if (transitions.empty()) return;

    for (const auto& transition : transitions) {
        const auto [old_state, new_state] = transition;
        // We just always ignore paused transitions since
        // the game state didnt actually change in a meaningful way
        if (old_state == game::State::Paused) continue;
        if (new_state == game::State::Paused) continue;
    }

    transitions.clear();
}

void SystemManager::every_frame_update(const Entities& entity_list, float) {
    PathRequestManager::process_responses(entity_list);

    // for_each(entity_list, dt, [](Entity& entity, float dt) {});
}

void SystemManager::render_entities(const Entities& entities, float dt) const {
    // NOTE: Rendering is now handled by RenderEntitiesSystem
    // The system's once() method handles on_frame_start() and
    // render_walkable_spots() The system's for_each_with() method handles
    // rendering each entity We use systems.render() which expects const
    // Entities&
    systems.render(entities, dt);
}

void SystemManager::render_ui(const Entities& entities, float dt) const {
    // const auto debug_mode_on =
    // GLOBALS.get_or_default<bool>("debug_ui_enabled", false);
    system_manager::ui::render_normal(entities, dt);
}

bool SystemManager::is_bar_open() const {
    for (const auto& entity : oldAll) {
        if (entity->is_missing<HasDayNightTimer>()) continue;
        return entity->get<HasDayNightTimer>().is_bar_open();
    }
    return false;
}
bool SystemManager::is_bar_closed() const { return !is_bar_open(); }

bool SystemManager::is_some_player_near(vec2 spot, float distance) const {
    bool someone_close = false;
    for (auto& player : remote_players) {
        auto pos = player->get<Transform>().as2();
        if (vec::distance(pos, spot) < distance) {
            someone_close = true;
            break;
        }
    }
    return someone_close;
}
