#pragma once

#include "../../ah.h"
#include "../../components/all_components.h"
#include "../../components/is_nux_manager.h"
#include "../../components/is_round_settings_manager.h"
#include "../../engine/statemanager.h"

namespace system_manager {

// TODO this is currently broken, need to fix it
struct ProcessNuxUpdatesSystem
    : public afterhours::System<IsNuxManager, IsRoundSettingsManager> {
    virtual bool should_run(const float) override { return true; }

    virtual void for_each_with(Entity& entity, IsNuxManager& inm,
                               IsRoundSettingsManager&, float dt) override {
        // Tutorial isnt on so dont do any nuxes
        if (!entity.get<IsRoundSettingsManager>()
                 .interactive_settings.is_tutorial_active) {
            return;
        }

        // only generate the nux once you leave the lobby
        if (!GameState::get().is_game_like()) return;

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
        if (active_nux.has_value() && active_nux->get<IsNux>().is_active)
            return;

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

    bool _create_nuxes(Entity&) {
        if (SystemManager::get().is_bar_closed()) return false;

        OptEntity player = EQ(SystemManager::get().oldAll)
                               .whereType(EntityType::Player)
                               .gen_first();
        if (!player.has_value()) return false;
        OptEntity reg =
            EntityQuery().whereType(EntityType::Register).gen_first();
        if (!reg.has_value()) return false;

        int player_id = player->id;
        int register_id = reg->id;

        // Planning mode tutorial
        if (1) {
            // Find register
            {
                // move the register out to the Planning_SpawnArea
                {
                    OptEntity purchase_area =
                        EQ().whereFloorMarkerOfType(
                                IsFloorMarker::Type::Planning_SpawnArea)
                            .gen_first();
                    reg->get<Transform>().update(
                        vec::to3(purchase_area->get<Transform>().as2()));
                }

                auto& entity = EntityHelper::createEntity();
                make_entity(entity, {EntityType::Unknown}, vec2{0, 0});

                entity.addComponent<IsNux>()
                    .should_attach_to(player_id)
                    .set_eligibility_fn(
                        [](const IsNux&) -> bool { return true; })
                    .set_completion_fn(
                        [register_id](const IsNux& inux) -> bool {
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
                const auto tex_name =
                    KeyMap::get().icon_for_input(valid_inputs[0]);

                auto& entity = EntityHelper::createEntity();
                make_entity(entity, {EntityType::Unknown}, vec2{0, 0});

                entity.addComponent<IsNux>()
                    .should_attach_to(player_id)
                    .set_eligibility_fn(
                        [](const IsNux&) -> bool { return true; })
                    .set_completion_fn(
                        [register_id](const IsNux& inux) -> bool {
                            return EQ(SystemManager::get().oldAll)
                                .whereID(inux.entityID)
                                .whereIsHoldingFurnitureID(register_id)
                                .has_values();
                        })
                    // TODO replace playerpickup with the actual control
                    .set_content(TODO_TRANSLATE(
                        fmt::format("Grab it with [{}]", tex_name),
                        TodoReason::SubjectToChange));
            }

            // Place register
            {
                auto& entity = EntityHelper::createEntity();
                make_entity(entity, {EntityType::Unknown}, vec2{-6.f, 1.f});

                entity.addComponent<IsNux>()
                    .set_eligibility_fn(
                        [](const IsNux&) -> bool { return true; })
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

                OptEntity ffd = EntityQuery()
                                    .whereType(EntityType::FastForward)
                                    .gen_first();
                int ffd_id = ffd->id;

                entity.addComponent<IsNux>()
                    .should_attach_to(ffd_id)
                    .set_eligibility_fn(
                        [](const IsNux&) -> bool { return true; })
                    .set_completion_fn(
                        [ffd_id, player_id](const IsNux&) -> bool {
                            OptEntity ffd =
                                EntityQuery().whereID(ffd_id).gen_first();
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
                const auto tex_name =
                    KeyMap::get().icon_for_input(valid_inputs[0]);

                auto& entity = EntityHelper::createEntity();
                make_entity(entity, {EntityType::Unknown}, vec2{-6.f, 1.f});

                OptEntity ffd = EntityQuery()
                                    .whereType(EntityType::FastForward)
                                    .gen_first();
                int ffd_id = ffd->id;

                entity.addComponent<IsNux>()
                    .should_attach_to(ffd_id)
                    .set_eligibility_fn(
                        [](const IsNux&) -> bool { return true; })
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

                        auto customer =
                            EntityHelper::getEntityForID(inux.entityID);
                        if (!customer) return false;

                        if (customer->is_missing<HasAIQueueState>())
                            return false;
                        const HasAIQueueState& aiQState =
                            customer->get<HasAIQueueState>();
                        return aiQState.line_wait.queue_index == 0;
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
                    .set_content(TODO_TRANSLATE("Grab a cup",
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
                        auto table = EntityQuery()
                                         .whereType(EntityType::Table)
                                         .gen_first();
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
                const auto tex_name =
                    KeyMap::get().icon_for_input(valid_inputs[0]);

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
                        auto drink = EntityQuery()
                                         .whereType(EntityType::Drink)
                                         .gen_first();
                        inux.should_attach_to(drink->id);
                    })
                    .set_completion_fn([](const IsNux& inux) -> bool {
                        return EntityQuery()
                            .whereID(inux.entityID)
                            .whereIsDrinkAndMatches(Drink::coke)
                            .has_values();
                    })
                    .set_content(TODO_TRANSLATE(
                        fmt::format("Use [{}] to fill the cup with soda",
                                    tex_name),
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
                                if (!item.hasTag(EntityType::Drink))
                                    return false;
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

                OptEntity ffd = EntityQuery()
                                    .whereType(EntityType::FastForward)
                                    .gen_first();
                int ffd_id = ffd->id;

                entity.addComponent<IsNux>()
                    .should_attach_to(ffd_id)
                    .set_eligibility_fn([](const IsNux&) -> bool {
                        if (!GameState::get().is_game_like()) return false;

                        bool has_customers =
                            EntityQuery()
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

                        return all_customers_at_gather &&
                               lt_halfway_through_day;
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
                    .set_content(
                        TODO_TRANSLATE("Since customers are all gone, \n"
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
};

}  // namespace system_manager
