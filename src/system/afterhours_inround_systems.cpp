#include "../ah.h"
#include "../components/can_be_taken_from.h"
#include "../components/can_grab_from_other_furniture.h"
#include "../components/can_hold_item.h"
#include "../components/can_order_drink.h"
#include "../components/conveys_held_item.h"
#include "../components/has_day_night_timer.h"
#include "../components/has_fishing_game.h"
#include "../components/has_patience.h"
#include "../components/has_rope_to_item.h"
#include "../components/has_work.h"
#include "../components/indexer.h"
#include "../components/is_item.h"
#include "../components/is_item_container.h"
#include "../components/is_pnumatic_pipe.h"
#include "../components/is_progression_manager.h"
#include "../components/is_round_settings_manager.h"
#include "../components/is_solid.h"
#include "../components/is_spawner.h"
#include "../components/transform.h"
#include "../dataclass/settings.h"
#include "../engine/pathfinder.h"
#include "../engine/statemanager.h"
#include "../entity_helper.h"
#include "../entity_id.h"
#include "../entity_makers.h"
#include "../entity_query.h"
#include "../entity_type.h"
#include "../log/log.h"
#include "../network/server.h"
#include "../vec_util.h"
#include "../vendor_include.h"
#include "afterhours_systems.h"
#include "ingredient_helper.h"
#include "input_process_manager.h"
#include "system_manager.h"

namespace system_manager {

// System for processing containers that should update items during in-round
// updates
struct ProcessIsContainerAndShouldUpdateItemSystem
    : public afterhours::System<Transform, IsItemContainer, Indexer,
                                CanHoldItem> {
    virtual ~ProcessIsContainerAndShouldUpdateItemSystem() = default;

    virtual bool should_run(const float) override {
        if (!GameState::get().is_game_like()) return false;
        try {
            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            const HasDayNightTimer& hastimer = sophie.get<HasDayNightTimer>();
            // Don't run during transitions to avoid spawners creating entities
            // before transition logic completes
            if (hastimer.needs_to_process_change) return false;
            return hastimer.is_bar_open();
        } catch (...) {
            return false;
        }
    }

    // TODO fold in function implementation
    virtual void for_each_with(Entity& entity, Transform& transform,
                               IsItemContainer& iic, Indexer& indexer,
                               CanHoldItem& canHold, float dt) override {
        (void) transform;  // Unused parameter
        (void) iic;        // Unused parameter
        (void) indexer;    // Unused parameter
        (void) canHold;    // Unused parameter
        process_is_container_and_should_update_item(entity, dt);
    }
};

// System for processing indexed containers holding incorrect items during
// in-round updates
struct ProcessIsIndexedContainerHoldingIncorrectItemSystem
    : public afterhours::System<Indexer, CanHoldItem> {
    virtual ~ProcessIsIndexedContainerHoldingIncorrectItemSystem() = default;

    virtual bool should_run(const float) override {
        if (!GameState::get().is_game_like()) return false;
        try {
            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            const HasDayNightTimer& hastimer = sophie.get<HasDayNightTimer>();
            // Don't run during transitions to avoid spawners creating entities
            // before transition logic completes
            if (hastimer.needs_to_process_change) return false;
            return hastimer.is_bar_open();
        } catch (...) {
            return false;
        }
    }

    // TODO fold in function implementation
    virtual void for_each_with(Entity& entity, Indexer& indexer,
                               CanHoldItem& canHold, float dt) override {
        (void) indexer;  // Unused parameter
        (void) canHold;  // Unused parameter
        process_is_indexed_container_holding_incorrect_item(entity, dt);
    }
};

// System for processing grabber items during in-round updates
struct ProcessGrabberItemsSystem
    : public afterhours::System<Transform, CanHoldItem, ConveysHeldItem,
                                CanGrabFromOtherFurniture> {
    virtual ~ProcessGrabberItemsSystem() = default;

    virtual bool should_run(const float) override {
        if (!GameState::get().is_game_like()) return false;
        try {
            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            const HasDayNightTimer& hastimer = sophie.get<HasDayNightTimer>();
            // Don't run during transitions to avoid spawners creating entities
            // before transition logic completes
            if (hastimer.needs_to_process_change) return false;
            return hastimer.is_bar_open();
        } catch (...) {
            return false;
        }
    }

    virtual void for_each_with(Entity& entity, Transform& transform,
                               CanHoldItem& canHold,
                               ConveysHeldItem& conveysHeldItem,
                               CanGrabFromOtherFurniture&, float) override {
        // we are already holding something so
        if (canHold.is_holding_item()) return;

        auto behind =
            transform.offsetFaceDirection(transform.face_direction(), 180);
        auto match = EQ().getMatchingEntityInFront(
            transform.as2(), behind, 1.f, [&entity](const Entity& furn) {
                // cant be us
                if (entity.id == furn.id) return false;
                // needs to be able to hold something
                if (furn.is_missing<CanHoldItem>()) return false;
                const CanHoldItem& furnCHI = furn.get<CanHoldItem>();
                // doesnt have anything
                if (furnCHI.empty()) return false;

                // Can we hold the item it has?
                OptEntity held_opt = furnCHI.const_item();
                if (!held_opt) return false;
                bool can_hold = entity.get<CanHoldItem>().can_hold(
                    held_opt.asE(), RespectFilter::All);

                // we cant
                if (!can_hold) return false;

                // We only check CanBe when it exists because everyone else can
                // always be taken from with a grabber
                if (furn.is_missing<CanBeTakenFrom>()) return true;
                return furn.get<CanBeTakenFrom>().can_take_from();
            });

        // No furniture behind us
        if (!match) return;

        // Grab from the furniture match
        CanHoldItem& matchCHI = match->get<CanHoldItem>();
        CanHoldItem& ourCHI = entity.get<CanHoldItem>();

        OptEntity grabbed_opt = matchCHI.item();
        if (!grabbed_opt) {
            matchCHI.update(nullptr, entity_id::INVALID);
            return;
        }
        ourCHI.update(grabbed_opt.asE(), entity.id);
        matchCHI.update(nullptr, entity_id::INVALID);

        conveysHeldItem.relative_item_pos = ConveysHeldItem::ITEM_START;
    }
};

// System for processing conveyer items during in-round updates
struct ProcessConveyerItemsSystem : public afterhours::System<> {
    virtual ~ProcessConveyerItemsSystem() = default;

    virtual bool should_run(const float) override {
        if (!GameState::get().is_game_like()) return false;
        try {
            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            const HasDayNightTimer& hastimer = sophie.get<HasDayNightTimer>();
            // Don't run during transitions to avoid spawners creating entities
            // before transition logic completes
            if (hastimer.needs_to_process_change) return false;
            return hastimer.is_bar_open();
        } catch (...) {
            return false;
        }
    }

    // TODO use system filters
    virtual void for_each_with(Entity& entity, float dt) override {
        if (entity
                .is_missing_any<CanHoldItem, ConveysHeldItem, CanBeTakenFrom>())
            return;

        const Transform& transform = entity.get<Transform>();

        CanHoldItem& canHold = entity.get<CanHoldItem>();
        CanBeTakenFrom& canBeTakenFrom = entity.get<CanBeTakenFrom>();
        ConveysHeldItem& conveysHeldItem = entity.get<ConveysHeldItem>();

        // we are not holding anything
        if (canHold.empty()) return;

        OptEntity carried_opt = canHold.const_item();
        if (!carried_opt) {
            canHold.update(nullptr, entity.id);
            canBeTakenFrom.update(true);
            return;
        }
        Entity& carried = carried_opt.asE();

        // make sure no one can insta-grab from us
        canBeTakenFrom.update(false);

        // if the item is less than halfway, just keep moving it along
        // 0 is halfway btw
        if (conveysHeldItem.relative_item_pos <= 0.f) {
            conveysHeldItem.relative_item_pos += conveysHeldItem.SPEED * dt;
            return;
        }

        bool is_ipp = entity.has<IsPnumaticPipe>();

        const auto _conveyer_filter = [&entity,
                                       &carried](const Entity& furn) -> bool {
            // cant be us
            if (entity.id == furn.id) return false;
            // needs to be able to hold something
            if (furn.is_missing<CanHoldItem>()) return false;
            const CanHoldItem& furnCHI = furn.get<CanHoldItem>();
            // has to be empty
            if (furnCHI.is_holding_item()) return false;
            // can this furniture hold the item we are passing?
            // some have filters
            bool can_hold = furnCHI.can_hold(carried, RespectFilter::ReqOnly);

            return can_hold;
        };

        const auto _ipp_filter =
            [&entity, _conveyer_filter](const Entity& furn) -> bool {
            // if we are a pnumatic pipe, filter only down to our guy
            if (furn.is_missing<IsPnumaticPipe>()) return false;
            const IsPnumaticPipe& mypp = entity.get<IsPnumaticPipe>();
            if (mypp.paired.id != furn.id) return false;
            if (mypp.recieving) return false;
            return _conveyer_filter(furn);
        };

        OptEntity match;
        if (is_ipp) {
            auto pos = transform.as2();
            match = EQ().whereLambda(_ipp_filter)
                        .whereInRange(pos, MAX_SEARCH_RANGE)
                        .orderByDist(pos)
                        .gen_first();
        } else {
            match = EQ().getMatchingEntityInFront(transform.as2(),
                                                  transform.face_direction(),
                                                  1.f, _conveyer_filter);
        }

        // no match means we can't continue, stay in the middle
        if (!match) {
            conveysHeldItem.relative_item_pos = 0.f;
            canBeTakenFrom.update(true);
            return;
        }

        if (is_ipp) {
            entity.get<IsPnumaticPipe>().recieving = false;
        }

        // we got something that will take from us,
        // but only once we get close enough

        // so keep moving forward
        if (conveysHeldItem.relative_item_pos <= ConveysHeldItem::ITEM_END) {
            conveysHeldItem.relative_item_pos += conveysHeldItem.SPEED * dt;
            return;
        }

        // we reached the end, pass ownership

        CanHoldItem& ourCHI = entity.get<CanHoldItem>();

        CanHoldItem& matchCHI = match->get<CanHoldItem>();
        matchCHI.update(carried, entity.id);

        ourCHI.update(nullptr, entity_id::INVALID);

        canBeTakenFrom.update(
            true);  // we are ready to have someone grab from us
        // reset so that the next item we get starts from beginning
        conveysHeldItem.relative_item_pos = ConveysHeldItem::ITEM_START;

        if (match->has<CanBeTakenFrom>()) {
            match->get<CanBeTakenFrom>().update(false);
        }

        if (is_ipp && match->has<IsPnumaticPipe>()) {
            match->get<IsPnumaticPipe>().recieving = true;
        }

        if (match->is_missing<IsPnumaticPipe>() &&
            match->has<ConveysHeldItem>()) {
            // if we are pushing onto a conveyer, we need to make sure
            // we are keeping track of the orientations
            //
            //  --> --> in this case we want to place at 0.5f
            //
            //          ^
            //    -->-> |     in this we want to place at 0.f instead of -0.5
            bool send_to_middle = false;
            auto other_face = match->get<Transform>().face_direction();
            // TODO theres gotta be a math way to do this
            switch (transform.face_direction()) {
                case Transform::FORWARD:
                case Transform::BACK:
                    if (other_face == Transform::RIGHT ||
                        other_face == Transform::LEFT) {
                        send_to_middle = true;
                    }
                    break;
                case Transform::RIGHT:
                case Transform::LEFT:
                    if (other_face == Transform::FORWARD ||
                        other_face == Transform::BACK) {
                        send_to_middle = true;
                    }
                    break;
            }
            if (send_to_middle) {
                match->get<ConveysHeldItem>().relative_item_pos =
                    ConveysHeldItem::ITEM_START +
                    ((ConveysHeldItem::ITEM_END - ConveysHeldItem::ITEM_START) /
                     2.f);
            }
        }
    }
};

struct ProcessGrabberFilterSystem : public afterhours::System<> {
    virtual ~ProcessGrabberFilterSystem() = default;

    virtual bool should_run(const float) override {
        if (!GameState::get().is_game_like()) return false;
        try {
            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            const HasDayNightTimer& hastimer = sophie.get<HasDayNightTimer>();
            // Don't run during transitions to avoid spawners creating entities
            // before transition logic completes
            if (hastimer.needs_to_process_change) return false;
            return hastimer.is_bar_open();
        } catch (...) {
            return false;
        }
    }

    // TODO use system filters
    virtual void for_each_with(Entity& entity, float) override {
        if (!check_type(entity, EntityType::FilteredGrabber)) return;
        if (entity.is_missing<CanHoldItem>()) return;
        CanHoldItem& canHold = entity.get<CanHoldItem>();
        if (canHold.empty()) return;

        // If we are holding something, then:
        // - either its already in the filter (and setting it wont be a big
        // deal)
        // - or we should set the filter

        OptEntity held_opt = canHold.item();
        if (!held_opt) {
            canHold.update(nullptr, entity.id);
            return;
        }
        EntityFilter& ef = canHold.get_filter();
        ef.set_filter_with_entity(held_opt.asE());
    }
};

struct ProcessSpawnerSystem : public afterhours::System<Transform, IsSpawner> {
    virtual bool should_run(const float) override {
        if (!GameState::get().is_game_like()) return false;
        try {
            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            const HasDayNightTimer& hastimer = sophie.get<HasDayNightTimer>();
            // Don't run during transitions to avoid spawners creating entities
            // before transition logic completes
            if (hastimer.needs_to_process_change) return false;
            return hastimer.is_bar_open();
        } catch (...) {
            return false;
        }
    }
    void for_each_with(Entity& entity, Transform& transform, IsSpawner& spawner,
                       float dt) override {
        vec2 pos = transform.as2();

        bool is_time_to_spawn = spawner.pass_time(dt);
        if (!is_time_to_spawn) return;

        SpawnInfo info{
            .location = pos,
            .is_first_this_round = (spawner.get_num_spawned() == 0),
        };

        // If there is a validation function check that first
        bool can_spawn_here_and_now = spawner.validate(entity, info);
        if (!can_spawn_here_and_now) return;

        bool should_prev_dupes = spawner.prevent_dupes();
        if (should_prev_dupes) {
            for (const Entity& e : EQ().whereInRange(pos, TILESIZE).gen()) {
                if (e.id == entity.id) continue;

                // Other than invalid and Us, is there anything else there?
                // log_info(
                // "was ready to spawn but then there was someone there
                // already");
                return;
            }
        }

        auto& new_ent = EntityHelper::createEntity();
        spawner.spawn(new_ent, info);
        spawner.post_spawn_reset();

        if (spawner.has_spawn_sound()) {
            network::Server::play_sound(pos, spawner.get_spawn_sound());
        }
    }
};

struct ReduceImpatientCustomersSystem : public afterhours::System<HasPatience> {
    virtual ~ReduceImpatientCustomersSystem() = default;

    virtual bool should_run(const float) override {
        if (!GameState::get().is_game_like()) return false;
        try {
            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            const HasDayNightTimer& hastimer = sophie.get<HasDayNightTimer>();
            // Don't run during transitions to avoid spawners creating entities
            // before transition logic completes
            if (hastimer.needs_to_process_change) return false;
            return hastimer.is_bar_open();
        } catch (...) {
            return false;
        }
    }

    virtual void for_each_with(Entity& entity, HasPatience& patience,
                               float dt) override {
        if (!check_type(entity, EntityType::Customer)) return;

        if (patience.should_pass_time()) {
            patience.pass_time(dt);

            // TODO actually do something when they get mad
            if (patience.pct() <= 0) {
                patience.reset();
                log_warn("You wont like me when im angry");
            }
        }
    }
};

struct ProcessPnumaticPipeMovementSystem : public afterhours::System<> {
    virtual ~ProcessPnumaticPipeMovementSystem() = default;

    virtual bool should_run(const float) override {
        if (!GameState::get().is_game_like()) return false;
        try {
            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            const HasDayNightTimer& hastimer = sophie.get<HasDayNightTimer>();
            // Don't run during transitions to avoid spawners creating entities
            // before transition logic completes
            if (hastimer.needs_to_process_change) return false;
            return hastimer.is_bar_open();
        } catch (...) {
            return false;
        }
    }

    virtual void for_each_with(Entity& entity, float dt) override {
        (void) dt;  // Unused parameter
        if (!check_type(entity, EntityType::PnumaticPipe)) return;
        if (entity.is_missing<IsPnumaticPipe>()) return;
        const IsPnumaticPipe& ipp = entity.get<IsPnumaticPipe>();

        if (!ipp.has_pair()) return;

        OptEntity paired_pipe = EntityQuery()
                                    .whereID(ipp.paired.id)
                                    .whereHasComponent<IsPnumaticPipe>()
                                    .gen_first();

        if (!paired_pipe) return;

        const IsPnumaticPipe& other_ipp = paired_pipe->get<IsPnumaticPipe>();

        // if they are both recieving or both sending, do nothing
        if (ipp.recieving == other_ipp.recieving) return;

        // we are recieving, they are sending
        if (ipp.recieving) {
            // we are the reciever, we should take the item
            if (paired_pipe->is_missing<CanHoldItem>()) return;
            CanHoldItem& other_chi = paired_pipe->get<CanHoldItem>();

            if (other_chi.empty()) return;

            if (entity.is_missing<CanHoldItem>()) return;
            CanHoldItem& our_chi = entity.get<CanHoldItem>();

            if (our_chi.is_holding_item()) return;

            OptEntity other_item_opt = other_chi.const_item();
            if (!other_item_opt) {
                other_chi.update(nullptr, entity_id::INVALID);
                return;
            }
            Entity& other_item = other_item_opt.asE();

            // can we hold it?
            if (!our_chi.can_hold(other_item, RespectFilter::ReqOnly)) return;

            // take it
            our_chi.update(other_item, entity.id);
            other_chi.update(nullptr, entity_id::INVALID);

            // we are done recieving
            entity.get<IsPnumaticPipe>().recieving = false;
        }
    }
};

struct ProcessHasRopeSystem : public afterhours::System<> {
    virtual ~ProcessHasRopeSystem() = default;

    virtual bool should_run(const float) override {
        if (!GameState::get().is_game_like()) return false;
        try {
            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            const HasDayNightTimer& hastimer = sophie.get<HasDayNightTimer>();
            // Don't run during transitions to avoid spawners creating entities
            // before transition logic completes
            if (hastimer.needs_to_process_change) return false;
            return hastimer.is_bar_open();
        } catch (...) {
            return false;
        }
    }

    // TODO use system filters
    virtual void for_each_with(Entity& entity, float) override {
        if (entity.is_missing<CanHoldItem>()) return;
        if (entity.is_missing<HasRopeToItem>()) return;

        HasRopeToItem& hrti = entity.get<HasRopeToItem>();

        // No need to have rope if spout is put away
        const CanHoldItem& chi = entity.get<CanHoldItem>();
        if (chi.is_holding_item()) {
            hrti.clear();
            return;
        }

        // Find the player who is holding __OUR__ spout

        OptEntity player;
        for (const std::shared_ptr<Entity>& e : SystemManager::get().oldAll) {
            if (!e) continue;
            // only route to players
            if (!check_type(*e, EntityType::Player)) continue;
            const CanHoldItem& e_chi = e->get<CanHoldItem>();
            if (!e_chi.is_holding_item()) continue;
            OptEntity held_opt = e_chi.const_item();
            if (!held_opt) continue;
            const Item& i = held_opt.asE();
            // that are holding spouts
            if (!check_type(i, EntityType::SodaSpout)) continue;
            // that match the one we were holding
            if (i.id != chi.last_id()) continue;
            player = *e;
        }
        if (!player) return;

        auto pos = player->get<Transform>().as2();

        // If we moved more then regenerate
        if (vec::distance(pos, hrti.goal()) > TILESIZE) {
            hrti.clear();
        }

        // Already generated
        if (hrti.was_generated()) return;

        auto new_path = pathfinder::find_path(entity.get<Transform>().as2(),
                                              pos, [](vec2) { return true; });

        std::vector<vec2> extended_path;
        std::optional<vec2> prev;
        for (auto p : new_path) {
            if (prev.has_value()) {
                extended_path.push_back(vec::lerp(prev.value(), p, 0.33f));
                extended_path.push_back(vec::lerp(prev.value(), p, 0.66f));
                extended_path.push_back(vec::lerp(prev.value(), p, 0.99f));
            }
            extended_path.push_back(p);
            prev = p;
        }

        for (auto p : extended_path) {
            Entity& item =
                EntityHelper::createItem(EntityType::SodaSpout, vec::to3(p));
            item.get<IsItem>().set_held_by(EntityType::Player, player->id);
            item.addComponent<IsSolid>();
            hrti.add(item);
        }
        hrti.mark_generated(pos);
    }
};

struct ResetCustomersThatNeedResettingSystem
    : public afterhours::System<CanOrderDrink> {
    IsRoundSettingsManager* irsm;
    IsProgressionManager* progressionManager;

    virtual ~ResetCustomersThatNeedResettingSystem() = default;

    virtual bool should_run(const float) override {
        if (!GameState::get().is_game_like()) return false;
        try {
            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            const HasDayNightTimer& hastimer = sophie.get<HasDayNightTimer>();
            // Don't run during transitions to avoid spawners creating entities
            // before transition logic completes
            if (hastimer.needs_to_process_change) return false;
            return hastimer.is_bar_open();
        } catch (...) {
            return false;
        }
    }

    virtual void once(float) override {
        Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
        irsm = &sophie.get<IsRoundSettingsManager>();
        progressionManager = &sophie.get<IsProgressionManager>();
    }

    virtual void for_each_with(Entity& entity, CanOrderDrink& cod,
                               float) override {
        if (cod.order_state != CanOrderDrink::OrderState::NeedsReset) return;

        {
            int max_num_orders =
                // max() here to avoid a situation where we get 0 after an
                // upgrade
                (int) fmax(1, irsm->get<int>(ConfigKey::MaxNumOrders));

            cod.reset_customer(max_num_orders,
                               progressionManager->get_random_unlocked_drink());
        }

        {
            // Set the patience based on how many ingredients there are
            // TODO add a map of ingredient to how long it probably takes to
            // make

            auto ingredients = get_req_ingredients_for_drink(cod.get_order());
            float patience_multiplier =
                irsm->get<float>(ConfigKey::PatienceMultiplier);
            entity.get<HasPatience>().update_max(ingredients.count() * 30.f *
                                                 patience_multiplier);
            entity.get<HasPatience>().reset();
        }
    }
};

};  // namespace system_manager

struct UpgradeInRoundUpdateSystem
    : public afterhours::System<IsRoundSettingsManager, IsProgressionManager,
                                HasDayNightTimer> {
    virtual bool should_run(const float) override {
        if (!GameState::get().is_game_like()) return false;
        try {
            Entity& sophie = EntityHelper::getNamedEntity(NamedEntity::Sophie);
            const HasDayNightTimer& hastimer = sophie.get<HasDayNightTimer>();
            // Don't run during transitions to avoid spawners creating entities
            // before transition logic completes
            if (hastimer.needs_to_process_change) return false;
            return hastimer.is_bar_open();
        } catch (...) {
            return false;
        }
    }

    virtual void for_each_with(Entity&, IsRoundSettingsManager& irsm,
                               IsProgressionManager& ipm,
                               HasDayNightTimer& hasTimer, float) override {
        int hour = 100 - static_cast<int>(hasTimer.pct() * 100.f);

        // Make sure we only run this once an hour
        if (hour <= irsm.ran_for_hour) return;

        int hours_missed = (hour - irsm.ran_for_hour);
        if (hours_missed > 1) {
            // 1 means normal, so >1 means we actually missed one
            // this currently only happens in debug mode so lets just log it
            log_warn("missed {} hours", hours_missed);

            //  TODO when you ffwd in debug mode it skips some of the hours
            //  should we instead run X times at least for acitvities?
        }

        const auto& collect_mods = [&]() {
            Mods mods;
            for (const auto& upgrade : irsm.selected_upgrades) {
                if (!upgrade->onHourMods) continue;
                auto new_mods = upgrade->onHourMods(irsm.config, ipm, hour);
                mods.insert(mods.end(), new_mods.begin(), new_mods.end());
            }
            irsm.config.this_hours_mods = mods;
        };
        collect_mods();

        // Run actions...
        // we need to run for every hour we missed

        const auto spawn_customer_action = []() {
            auto spawner =
                EQ().whereType(EntityType::CustomerSpawner).gen_first();
            if (!spawner) {
                log_warn("Could not find customer spawner?");
                return;
            }
            auto& new_ent = EntityHelper::createEntity();
            make_customer(new_ent,
                          SpawnInfo{.location = spawner->get<Transform>().as2(),
                                    .is_first_this_round = false},
                          true);
            return;
        };

        for (const auto& upgrade : irsm.selected_upgrades) {
            if (!upgrade->onHourActions) continue;

            // We start at 1 since its normal to have 1 hour missed ^^ see
            // above
            int i = 1;
            while (i < hours_missed) {
                log_info("running actions for {} for hour {} (currently {})",
                         upgrade->name.debug(), irsm.ran_for_hour + i, hour);
                i++;

                auto actions = upgrade->onHourActions(irsm.config, ipm,
                                                      irsm.ran_for_hour + i);
                for (auto action : actions) {
                    switch (action) {
                        case SpawnCustomer:
                            spawn_customer_action();
                            break;
                    }
                }
            }
        }

        irsm.ran_for_hour = hour;
    }
};

void SystemManager::register_inround_systems() {
    systems.register_update_system(
        std::make_unique<
            system_manager::ResetCustomersThatNeedResettingSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::ProcessGrabberItemsSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::ProcessConveyerItemsSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::ProcessGrabberFilterSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::ProcessHasRopeSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::ProcessPnumaticPipeMovementSystem>());
    // should move all the container functions into its own
    // function?
    systems.register_update_system(
        std::make_unique<
            system_manager::ProcessIsContainerAndShouldUpdateItemSystem>());
    // This one should be after the other container ones
    systems.register_update_system(
        std::make_unique<
            system_manager::
                ProcessIsIndexedContainerHoldingIncorrectItemSystem>());

    systems.register_update_system(
        std::make_unique<system_manager::ProcessSpawnerSystem>());
    systems.register_update_system(
        std::make_unique<system_manager::ReduceImpatientCustomersSystem>());
    system_manager::input_process_manager::inround::register_input_systems(
        systems);
}