

#include "system_manager.h"

#include "../camera.h"  /// probably needed by map and not included in there
#include "../components/can_be_ghost_player.h"
#include "../components/can_grab_from_other_furniture.h"
#include "../components/can_highlight_others.h"
#include "../components/can_hold_furniture.h"
#include "../components/can_perform_job.h"
#include "../components/collects_user_input.h"
#include "../components/conveys_held_item.h"
#include "../components/custom_item_position.h"
#include "../components/is_snappable.h"
#include "../components/responds_to_user_input.h"
#include "../components/transform.h"
#include "../engine/tracy.h"
#include "../entity.h"
#include "../entityhelper.h"
#include "../map.h"

namespace system_manager {
void transform_snapper(Entity& entity, float) {
    if (entity.is_missing<Transform>()) return;
    Transform& transform = entity.get<Transform>();
    transform.update(entity.has<IsSnappable>() ? transform.snap_position()
                                               : transform.raw());
}

// TODO if cannot be placed in this spot make it obvious to the user
void update_held_furniture_position(Entity& entity, float) {
    if (entity.is_missing_any<Transform, CanHoldFurniture>()) return;

    const Transform& transform = entity.get<Transform>();

    CanHoldFurniture& can_hold_furniture = entity.get<CanHoldFurniture>();
    if (can_hold_furniture.empty()) return;

    auto new_pos = transform.pos();
    if (transform.face_direction() & Transform::FrontFaceDirection::FORWARD) {
        new_pos.z += TILESIZE;
    }
    if (transform.face_direction() & Transform::FrontFaceDirection::RIGHT) {
        new_pos.x += TILESIZE;
    }
    if (transform.face_direction() & Transform::FrontFaceDirection::BACK) {
        new_pos.z -= TILESIZE;
    }
    if (transform.face_direction() & Transform::FrontFaceDirection::LEFT) {
        new_pos.x -= TILESIZE;
    }

    can_hold_furniture.furniture()->get<Transform>().update(new_pos);
}

// TODO should held item position be a physical movement or just visual?
// does it matter if the reach/pickup is working as expected
void update_held_item_position(Entity& entity, float) {
    if (entity.is_missing<CanHoldItem>()) return;

    CanHoldItem& can_hold_item = entity.get<CanHoldItem>();
    if (can_hold_item.empty()) return;

    const Transform& transform = entity.get<Transform>();

    vec3 new_pos = transform.pos();

    if (entity.has<CustomHeldItemPosition>()) {
        CustomHeldItemPosition& custom_item_position =
            entity.get<CustomHeldItemPosition>();

        switch (custom_item_position.positioner) {
            case CustomHeldItemPosition::Positioner::Table:
                new_pos.y += TILESIZE / 2;
                break;
            case CustomHeldItemPosition::Positioner::ItemHoldingItem:
                new_pos.x += 0;
                new_pos.y += 0;
                break;
            case CustomHeldItemPosition::Positioner::Conveyer: {
                if (entity.is_missing<ConveysHeldItem>()) {
                    log_warn(
                        "A conveyer positioned item needs ConveysHeldItem");
                    break;
                }
                ConveysHeldItem& conveysHeldItem =
                    entity.get<ConveysHeldItem>();
                if (transform.face_direction() &
                    Transform::FrontFaceDirection::FORWARD) {
                    new_pos.z += TILESIZE * conveysHeldItem.relative_item_pos;
                }
                if (transform.face_direction() &
                    Transform::FrontFaceDirection::RIGHT) {
                    new_pos.x += TILESIZE * conveysHeldItem.relative_item_pos;
                }
                if (transform.face_direction() &
                    Transform::FrontFaceDirection::BACK) {
                    new_pos.z -= TILESIZE * conveysHeldItem.relative_item_pos;
                }
                if (transform.face_direction() &
                    Transform::FrontFaceDirection::LEFT) {
                    new_pos.x -= TILESIZE * conveysHeldItem.relative_item_pos;
                }
                new_pos.y += TILESIZE / 4;
            } break;
            case CustomHeldItemPosition::Positioner::PnumaticPipe: {
                if (entity.is_missing<IsPnumaticPipe>()) {
                    log_warn("pipe positioned item needs ispnumaticpipe");
                    break;
                }
                if (entity.is_missing<ConveysHeldItem>()) {
                    log_warn("pipe positioned item needs ConveysHeldItem");
                    break;
                }
                ConveysHeldItem& conveysHeldItem =
                    entity.get<ConveysHeldItem>();
                int mult = entity.get<IsPnumaticPipe>().recieving ? 1 : -1;
                new_pos.y +=
                    mult * TILESIZE * conveysHeldItem.relative_item_pos;
            } break;
        }
        can_hold_item.item()->get<Transform>().update(new_pos);
        return;
    }

    // Default
    if (transform.face_direction() & Transform::FrontFaceDirection::FORWARD) {
        new_pos.z += TILESIZE;
    }
    if (transform.face_direction() & Transform::FrontFaceDirection::RIGHT) {
        new_pos.x += TILESIZE;
    }
    if (transform.face_direction() & Transform::FrontFaceDirection::BACK) {
        new_pos.z -= TILESIZE;
    }
    if (transform.face_direction() & Transform::FrontFaceDirection::LEFT) {
        new_pos.x -= TILESIZE;
    }
    can_hold_item.item()->get<Transform>().update(new_pos);
}

void reset_highlighted(Entity& entity, float) {
    if (entity.is_missing<CanBeHighlighted>()) return;
    CanBeHighlighted& cbh = entity.get<CanBeHighlighted>();
    cbh.update(false);
}

void highlight_facing_furniture(Entity& entity, float) {
    Transform& transform = entity.get<Transform>();
    if (entity.is_missing<CanHighlightOthers>()) return;
    // TODO add a player reach component
    CanHighlightOthers& cho = entity.get<CanHighlightOthers>();

    auto match = EntityHelper::getClosestMatchingFurniture(
        transform, cho.reach(),
        [](auto e) { return e->template has<CanBeHighlighted>(); });
    if (!match) return;

    match->get<CanBeHighlighted>().update(true);
}

// TODO We need like a temporary storage for this
void move_entity_based_on_push_force(Entity& entity, float, vec3& new_pos_x,
                                     vec3& new_pos_z) {
    CanBePushed& cbp = entity.get<CanBePushed>();

    new_pos_x.x += cbp.pushed_force().x;
    cbp.update_x(0.0f);

    new_pos_z.z += cbp.pushed_force().z;
    cbp.update_z(0.0f);
}

void process_conveyer_items(Entity& entity, float dt) {
    Transform& transform = entity.get<Transform>();
    if (entity.is_missing_any<CanHoldItem, ConveysHeldItem, CanBeTakenFrom>())
        return;

    CanHoldItem& canHold = entity.get<CanHoldItem>();
    CanBeTakenFrom& canBeTakenFrom = entity.get<CanBeTakenFrom>();
    ConveysHeldItem& conveysHeldItem = entity.get<ConveysHeldItem>();

    // we are not holding anything
    if (canHold.empty()) return;

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
                                   &canHold](std::shared_ptr<Furniture> furn) {
        // cant be us
        if (entity.id == furn->id) return false;
        // needs to be able to hold something
        if (furn->is_missing<CanHoldItem>()) return false;
        CanHoldItem& furnCHI = furn->get<CanHoldItem>();
        // has to be empty
        if (furnCHI.is_holding_item()) return false;
        // can this furniture hold the item we are passing?
        // some have filters
        bool can_hold =
            furnCHI.can_hold(*(canHold.item()), RespectFilter::ReqOnly);

        return can_hold;
    };

    const auto _ipp_filter =
        [&entity, _conveyer_filter](std::shared_ptr<Furniture> furn) {
            // if we are a pnumatic pipe, filter only down to our guy
            if (furn->is_missing<IsPnumaticPipe>()) return false;
            const IsPnumaticPipe& mypp = entity.get<IsPnumaticPipe>();
            if (mypp.paired_id != furn->id) return false;
            if (mypp.recieving) return false;
            return _conveyer_filter(furn);
        };

    auto match = is_ipp ? EntityHelper::getClosestMatchingEntity<Furniture>(
                              transform.as2(), MAX_SEARCH_RANGE, _ipp_filter)
                        : EntityHelper::getClosestMatchingFurniture(
                              transform, 1.f, _conveyer_filter);

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
    matchCHI.update(ourCHI.item());

    ourCHI.update(nullptr);

    canBeTakenFrom.update(true);  // we are ready to have someone grab from us
    // reset so that the next item we get starts from beginning
    conveysHeldItem.relative_item_pos = ConveysHeldItem::ITEM_START;

    if (match->has<CanBeTakenFrom>()) {
        match->get<CanBeTakenFrom>().update(false);
    }

    if (is_ipp && match->has<IsPnumaticPipe>()) {
        match->get<IsPnumaticPipe>().recieving = true;
    }

    // TODO if we are pushing onto a conveyer, we need to make sure
    // we are keeping track of the orientations
    //
    //  --> --> in this case we want to place at 0.5f
    //
    //          ^
    //    -->-> |     in this we want to place at 0.f instead of -0.5
}

// TODO This function is 33% of our run time
// getMatchingEntity is a pretty large chunk of that
// processconveyer is 10x faster ...
void process_grabber_items(Entity& entity, float) {
    Transform& transform = entity.get<Transform>();

    if (entity.is_missing<CanHoldItem>()) return;
    CanHoldItem& canHold = entity.get<CanHoldItem>();
    // we are already holding something so
    if (canHold.is_holding_item()) return;

    // Should only run this for conveyers
    if (entity.is_missing<ConveysHeldItem>()) return;
    // Should only run for grabbers
    if (entity.is_missing<CanGrabFromOtherFurniture>()) return;

    ConveysHeldItem& conveysHeldItem = entity.get<ConveysHeldItem>();

    auto behind =
        transform.offsetFaceDirection(transform.face_direction(), 180);
    auto match = EntityHelper::getMatchingEntityInFront<Furniture>(
        transform.as2(), 1.f, behind,
        [&entity](std::shared_ptr<Furniture> furn) {
            // cant be us
            if (entity.id == furn->id) return false;
            // needs to be able to hold something
            if (furn->is_missing<CanHoldItem>()) return false;
            CanHoldItem& furnCHI = furn->get<CanHoldItem>();
            // doesnt have anything
            if (furnCHI.empty()) return false;

            // Can we hold the item it has?
            bool can_hold = entity.get<CanHoldItem>().can_hold(
                *(furnCHI.item()), RespectFilter::All);

            // we cant
            if (!can_hold) return false;

            // We only check CanBe when it exists because everyone else can
            // always be taken from with a grabber
            if (furn->is_missing<CanBeTakenFrom>()) return true;
            return furn->get<CanBeTakenFrom>().can_take_from();
        });

    // No furniture behind us
    if (!match) return;

    // Grab from the furniture match
    CanHoldItem& matchCHI = match->get<CanHoldItem>();
    CanHoldItem& ourCHI = entity.get<CanHoldItem>();

    ourCHI.update(matchCHI.item());
    matchCHI.update(nullptr);

    conveysHeldItem.relative_item_pos = ConveysHeldItem::ITEM_START;
}

void process_grabber_filter(Entity& entity, float) {
    if (!check_name(entity, strings::entity::FILTERED_GRABBER)) return;
    if (entity.is_missing<CanHoldItem>()) return;
    CanHoldItem& canHold = entity.get<CanHoldItem>();
    if (canHold.empty()) return;

    // If we are holding something, then:
    // - either its already in the filter (and setting it wont be a big deal)
    // - or we should set the filter

    EntityFilter& ef = canHold.get_filter();
    ef.set_filter_with_entity(*(canHold.const_item()));
}

template<typename... TArgs>
void backfill_empty_container(const std::string& match_type, Entity& entity,
                              TArgs&&... args) {
    if (entity.is_missing<IsItemContainer>()) return;
    IsItemContainer& iic = entity.get<IsItemContainer>();
    if (iic.type() != match_type) return;
    CanHoldItem& canHold = entity.get<CanHoldItem>();
    if (canHold.is_holding_item()) return;

    if (iic.hit_max()) return;
    iic.increment();

    // create item
    std::shared_ptr<Item> item =
        EntityHelper::createItem(iic.type(), std::forward<TArgs>(args)...);

    canHold.update(item);
}

void process_is_container_and_should_backfill_item(Entity& entity, float) {
    if (entity.is_missing<CanHoldItem>()) return;
    CanHoldItem& canHold = entity.get<CanHoldItem>();
    if (canHold.is_holding_item()) return;

    // TODO speed have each <> return true/false if it worked
    //      then can skip the rest, are there any that can hold mixed ?
    // TODO can we dynamically figure out what to run?

    auto pos = entity.get<Transform>().as2();

    backfill_empty_container(strings::item::SODA_SPOUT, entity, pos);
    backfill_empty_container(strings::item::DRINK, entity, pos);

    if (entity.is_missing<Indexer>()) return;
    backfill_empty_container(strings::item::ALCOHOL, entity, pos,
                             entity.get<Indexer>().value());
    backfill_empty_container(strings::item::LEMON, entity, pos,
                             entity.get<Indexer>().value());
    entity.get<Indexer>().mark_change_completed();
}

void process_is_container_and_should_update_item(Entity& entity, float) {
    if (entity.is_missing<Indexer>()) return;
    Indexer& indexer = entity.get<Indexer>();
    // user didnt change the index so we are good to wait
    if (indexer.value_same_as_last_render()) return;

    if (entity.is_missing<CanHoldItem>()) return;
    CanHoldItem& canHold = entity.get<CanHoldItem>();

    // Delete the currently held item
    if (canHold.is_holding_item()) {
        canHold.item()->cleanup = true;
        canHold.update(nullptr);
    }

    auto pos = entity.get<Transform>().as2();

    backfill_empty_container(strings::item::ALCOHOL, entity, pos,
                             indexer.value());
    backfill_empty_container(strings::item::LEMON, entity, pos,
                             entity.get<Indexer>().value());
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
    Indexer& indexer = entity.get<Indexer>();

    if (entity.is_missing<CanHoldItem>()) return;
    CanHoldItem& canHold = entity.get<CanHoldItem>();

    if (canHold.empty()) return;

    int current_value = indexer.value();
    int item_value = canHold.item()->get<HasSubtype>().get_type_index();

    if (current_value != item_value) {
        canHold.item()->cleanup = true;
        canHold.update(nullptr);
    }
}

void handle_autodrop_furniture_when_exiting_planning(Entity& entity) {
    if (entity.is_missing<CanHoldFurniture>()) return;

    CanHoldFurniture& ourCHF = entity.get<CanHoldFurniture>();
    if (ourCHF.empty()) return;

    // TODO need to find a spot it can go in using EntityHelper::isWalkable
    input_process_manager::planning::drop_held_furniture(entity);
}

void delete_customers_when_leaving_inround(Entity& entity) {
    // TODO im thinking this might not be enough if we have
    // robots that can order for people or something
    if (entity.is_missing<CanOrderDrink>()) return;
    if (!check_name(entity, strings::entity::CUSTOMER)) return;

    entity.cleanup = true;
}

void delete_held_items_when_leaving_inround(Entity& entity) {
    // TODO this doesnt seem to work
    // you keep holding it even after the transition

    if (entity.is_missing<CanHoldItem>()) return;

    CanHoldItem& canHold = entity.get<CanHoldItem>();
    if (canHold.empty()) return;

    // Mark it as deletable
    std::shared_ptr<Item>& item = canHold.item();

    // let go of the item
    item->cleanup = true;
    canHold.update(nullptr);
}

void reset_max_gen_when_after_deletion(Entity& entity) {
    if (entity.is_missing<CanHoldItem>()) return;
    if (entity.is_missing<IsItemContainer>()) return;

    CanHoldItem& canHold = entity.get<CanHoldItem>();
    // If something wasnt deleted, then just ignore it for now
    if (canHold.is_holding_item()) return;

    entity.get<IsItemContainer>().reset_generations();
}

void refetch_dynamic_model_names(Entity& entity, float) {
    if (entity.is_missing<ModelRenderer>()) return;
    if (entity.is_missing<HasDynamicModelName>()) return;

    HasDynamicModelName& hDMN = entity.get<HasDynamicModelName>();
    ModelRenderer& renderer = entity.get<ModelRenderer>();
    renderer.update_model_name(hDMN.fetch(entity));
}

void count_max_trigger_area_entrants(Entity& entity, float) {
    if (entity.is_missing<IsTriggerArea>()) return;

    int count = 0;
    for (auto& e : SystemManager::get().oldAll) {
        if (!e) continue;
        if (!check_name(*e, strings::entity::PLAYER)) continue;
        count++;
    }
    entity.get<IsTriggerArea>().update_max_entrants(count);
}

void count_trigger_area_entrants(Entity& entity, float) {
    if (entity.is_missing<IsTriggerArea>()) return;

    int count = 0;
    for (auto& e : SystemManager::get().oldAll) {
        if (!e) continue;
        if (!check_name(*e, strings::entity::PLAYER)) continue;
        if (CheckCollisionBoxes(
                e->get<Transform>().bounds(),
                entity.get<Transform>().expanded_bounds({0, TILESIZE, 0}))) {
            count++;
        }
    }
    entity.get<IsTriggerArea>().update_entrants(count);
}

void update_trigger_area_percent(Entity& entity, float dt) {
    if (entity.is_missing<IsTriggerArea>()) return;
    IsTriggerArea& ita = entity.get<IsTriggerArea>();
    if (ita.should_wave()) {
        ita.increase_progress(dt);
    } else {
        ita.decrease_progress(dt);
    }
}

void trigger_cb_on_full_progress(Entity& entity, float) {
    if (entity.is_missing<IsTriggerArea>()) return;
    IsTriggerArea& ita = entity.get<IsTriggerArea>();
    if (ita.progress() < 1.f) return;
    auto cb = ita.get_complete_fn();
    if (cb) cb();
}

void process_trigger_area(Entity& entity, float dt) {
    count_max_trigger_area_entrants(entity, dt);
    count_trigger_area_entrants(entity, dt);
    update_trigger_area_percent(entity, dt);
    trigger_cb_on_full_progress(entity, dt);
}

void process_spawner(Entity& entity, float dt) {
    if (entity.is_missing<IsSpawner>()) return;
    auto pos = entity.get<Transform>().as2();
    entity.get<IsSpawner>().pass_time(pos, dt);
}

void run_timer(Entity& entity, float dt) {
    if (entity.is_missing<HasTimer>()) return;
    HasTimer& ht = entity.get<HasTimer>();

    ht.pass_time(dt);

    // If we round isnt over yet, then thats all for now
    if (ht.round_not_over()) return;

    // Round is over so pass time for the round switch countdown

    ht.pass_time_round_switch(dt);

    // player still has time to do things
    if (ht.round_switch_not_ready()) return;

    // the timer expired, time to switch rounds
    // but first need to check if we can or not

    const auto _validate_if_round_can_end = [&hastimer = std::as_const(ht)]() {
        switch (GameState::get().read()) {
            case game::State::Planning: {
                // For this one, we need to wait until everyone drops the things
                if (hastimer.read_reason(
                        HasTimer::WaitingReason::HoldingFurniture))
                    return false;
                return true;
            } break;
            case game::State::InRound: {
                // For this one, we need to wait until everyone is done leaving
                if (hastimer.read_reason(
                        HasTimer::WaitingReason::CustomersInStore))
                    return false;
                return true;
            } break;
            default:
                log_warn(
                    "validating round switch timer but no state handler {}",
                    GameState::get().read());
                return false;
        }
        log_warn("validating round switch timer finished switch {}",
                 GameState::get().read());
        return false;
    };

    bool can_round_finish = _validate_if_round_can_end();
    if (!can_round_finish) return;

    // Round is actually over, reset timers

    switch (GameState::get().read()) {
        case game::State::Planning: {
            GameState::get().set(game::State::InRound);
        } break;
        case game::State::InRound: {
            GameState::get().set(game::State::Planning);
        } break;
        default:
            log_warn("processing round switch timer but no state handler {}",
                     GameState::get().read());
            return;
    }

    ht.reset_round_switch_timer().reset_timer();
}

void sophie(Entity& entity, float) {
    if (entity.is_missing<HasTimer>()) return;

    const auto debug_mode_on =
        GLOBALS.get_or_default<bool>("debug_ui_enabled", false);
    const HasTimer& ht = entity.get<HasTimer>();
    if (ht.currentRoundTime > 0 && !debug_mode_on) return;

    // Handle customers finally leaving the store
    auto _customers_in_store = [&entity]() {
        // TODO with the others siwtch to something else... customer
        // spawner?
        const auto endpos = vec2{GATHER_SPOT, GATHER_SPOT};

        bool all_gone = true;
        std::vector<std::shared_ptr<Entity>> customers =
            EntityHelper::getAllWithName(strings::entity::CUSTOMER);
        for (auto& e : customers) {
            if (!e) continue;
            if (vec::distance(e->get<Transform>().as2(), endpos) >
                TILESIZE * 2.f) {
                all_gone = false;
                break;
            }
        }
        entity.get<HasTimer>().write_reason(
            HasTimer::WaitingReason::CustomersInStore, !all_gone);
        return;
    };

    // Handle some player is holding furniture
    auto _player_holding_furniture = [&entity]() {
        bool all_empty = true;
        // TODO i want to to do it this way: but players are not in
        // entities, so its not possible
        //
        // auto players =
        // EntityHelper::getAllWithComponent<CanHoldFurniture>();

        for (std::shared_ptr<Entity>& e : SystemManager::get().oldAll) {
            if (!e) continue;
            if (e->is_missing<CanHoldFurniture>()) continue;
            if (e->get<CanHoldFurniture>().is_holding_furniture()) {
                all_empty = false;
                break;
            }
        }
        entity.get<HasTimer>().write_reason(
            HasTimer::WaitingReason::HoldingFurniture, !all_empty);
        return;
    };

    // TODO merge with map generation validation?
    // Run lightweight map validation
    auto _lightweight_map_validation = [&entity]() {
        // find customer
        auto customer_opt =
            EntityHelper::getFirstMatching([](const Entity& e) -> bool {
                return check_name(e, strings::entity::CUSTOMER_SPAWNER);
            });
        // TODO we are validating this now, but we shouldnt have to worry about
        // this in the future
        VALIDATE(valid(customer_opt),
                 "map needs to have at least one customer spawn point");
        auto& customer = asE(customer_opt);

        // ensure customers can make it to the register

        auto reg_opt =
            EntityHelper::getFirstMatching([&customer](const Entity& e) {
                if (!check_name(e, strings::entity::REGISTER)) return false;
                // TODO need a better way to do this
                // 0 makes sense but is the position of the entity, when its
                // infront?
                auto new_path = astar::find_path(
                    customer.get<Transform>().as2(),
                    e.get<Transform>().tile_infront(1),
                    std::bind(EntityHelper::isWalkable, std::placeholders::_1));
                return new_path.size() > 0;
            });

        entity.get<HasTimer>().write_reason(
            HasTimer::WaitingReason::NoPathToRegister, !valid(reg_opt));

        return;
    };

    // doing it this way so that if we wanna make them return bool itll be easy
    typedef std::function<void()> WaitingFn;

    std::vector<WaitingFn> fns{
        _customers_in_store,
        _player_holding_furniture,
        _lightweight_map_validation,
    };

    for (auto fn : fns) {
        fn();
    }
}

void reset_empty_work_furniture(Entity& entity, float) {
    if (entity.is_missing<HasWork>()) return;
    if (entity.is_missing<CanHoldItem>()) return;

    HasWork& hasWork = entity.get<HasWork>();
    if (!hasWork.should_reset_on_empty()) return;

    const CanHoldItem& chi = entity.get<CanHoldItem>();
    if (chi.empty()) {
        hasWork.reset_pct();
        return;
    }

    // TODO if its not empty, we have to see if its an item that can be worked
    return;
}

void process_has_rope(Entity& entity, float) {
    if (entity.is_missing<CanHoldItem>()) return;
    if (entity.is_missing<HasRopeToItem>()) return;

    HasRopeToItem& hrti = entity.get<HasRopeToItem>();

    // No need to have rope if spout is put away
    const CanHoldItem& chi = entity.get<CanHoldItem>();
    if (chi.is_holding_item()) {
        hrti.clear();
        return;
    }

    OptEntity opt_player;
    for (std::shared_ptr<Entity>& e : SystemManager::get().oldAll) {
        if (!e) continue;
        if (!check_name(*e, strings::entity::PLAYER)) continue;
        auto i = e->get<CanHoldItem>().item();
        if (!i) continue;
        if (!check_name(*i, strings::item::SODA_SPOUT)) continue;
        opt_player = *e;
    }
    if (!valid(opt_player)) return;

    auto pos = asE(opt_player).get<Transform>().as2();

    // If we moved more then regenerate
    if (vec::distance(pos, hrti.goal()) > TILESIZE) {
        hrti.clear();
    }

    // Already generated
    if (hrti.was_generated()) return;

    auto new_path = astar::find_path(entity.get<Transform>().as2(), pos,
                                     [](vec2) { return true; });

    for (auto p : new_path) {
        std::shared_ptr<Item> item =
            EntityHelper::createItem(strings::item::SODA_SPOUT, p);
        item->get<IsItem>().set_held_by(IsItem::HeldBy::PLAYER);
        item->addComponent<IsSolid>();
        hrti.add(item);
    }
    hrti.mark_generated(pos);
}

void process_squirter(Entity& entity, float) {
    // TODO this normally would be an IsComponent but for those where theres
    // only one probably check_name is easier/ cheaper? idk
    if (!check_name(entity, strings::entity::SQUIRTER)) return;

    CanHoldItem& sqCHI = entity.get<CanHoldItem>();

    // If we arent holding anything, nothing to squirt into
    if (sqCHI.empty()) return;

    // cant squirt into this !
    if (sqCHI.item()->is_missing<IsDrink>()) return;

    // so we got something, lets see if anyone around can give us something to
    // use

    std::shared_ptr<Furniture> closest_furniture =
        EntityHelper::getClosestMatchingEntity<Furniture>(
            entity.get<Transform>().as2(), 1.25f,
            [](std::shared_ptr<Furniture> f) {
                if (f->is_missing<CanHoldItem>()) return false;
                const CanHoldItem& fchi = f->get<CanHoldItem>();
                if (fchi.empty()) return false;

                std::shared_ptr<Item> item = fchi.const_item();

                // TODO should we instead check for <AddsIngredient>?
                if (!check_name(*item, strings::item::ALCOHOL)) return false;
                return true;
            });
    if (!closest_furniture) return;

    std::shared_ptr<Entity> drink = sqCHI.item();
    std::shared_ptr<Item> item = closest_furniture->get<CanHoldItem>().item();

    bool cleanup = items::_add_ingredient_to_drink_NO_VALIDATION(*drink, *item);
    if (cleanup) {
        closest_furniture->get<CanHoldItem>().update(nullptr);
    }
}

// TODO not everything can be trashed !
void process_trash(Entity& entity, float) {
    // TODO this normally would be an IsComponent but for those where theres
    // only one probably check_name is easier/ cheaper? idk
    if (!check_name(entity, strings::entity::TRASH)) return;

    CanHoldItem& trashCHI = entity.get<CanHoldItem>();

    // If we arent holding anything, nothing to delete
    if (trashCHI.empty()) return;

    trashCHI.item()->cleanup = true;
    trashCHI.update(nullptr);
}

void process_pnumatic_pipe_pairing(Entity& entity, float) {
    if (entity.is_missing<IsPnumaticPipe>()) return;

    IsPnumaticPipe& ipp = entity.get<IsPnumaticPipe>();

    if (ipp.has_pair()) return;

    for (auto other : EntityHelper::getAllWithComponent<IsPnumaticPipe>()) {
        if (other->cleanup) continue;
        if (other->id == entity.id) continue;
        IsPnumaticPipe& otherpp = other->get<IsPnumaticPipe>();
        if (otherpp.has_pair()) continue;

        otherpp.paired_id = entity.id;
        ipp.paired_id = other->id;
        break;
    }
    // still dont have a pair, we probably just have an odd number
    if (!ipp.has_pair()) return;
}
void process_pnumatic_pipe_movement(Entity& entity, float) {
    if (entity.is_missing<IsPnumaticPipe>()) return;

    IsPnumaticPipe& ipp = entity.get<IsPnumaticPipe>();
    CanHoldItem& chi = entity.get<CanHoldItem>();

    if (chi.empty()) {
        ipp.item_id = -1;
        ipp.recieving = false;
        return;
    }

    int cur_id = chi.const_item()->id;
    if (ipp.item_id != cur_id) {
        ipp.item_id = cur_id;
    }
}

void increment_day_count(Entity& entity, float) {
    if (entity.is_missing<HasTimer>()) return;
    entity.get<HasTimer>().dayCount++;
}

void reset_customer_spawner_when_leaving_inround(Entity& entity) {
    if (entity.is_missing<IsSpawner>()) return;
    entity.get<IsSpawner>().reset_num_spawned();
}

void reset_customers_that_need_resetting(Entity& entity) {
    if (entity.is_missing<CanOrderDrink>()) return;
    CanOrderDrink& cod = entity.get<CanOrderDrink>();

    if (cod.order_state != CanOrderDrink::OrderState::NeedsReset) return;

    std::shared_ptr<Entity> sophie =
        EntityHelper::getFirstWithComponent<IsProgressionManager>();
    VALIDATE(sophie, "sophie should exist for sure");

    const IsProgressionManager& progressionManager =
        sophie->get<IsProgressionManager>();

    {
        // TODO eventually read from game settings
        cod.num_orders_rem = randIn(0, 5);
        cod.current_order = progressionManager.get_random_drink();
        cod.order_state = CanOrderDrink::OrderState::Ordering;
    }
}

}  // namespace system_manager

void SystemManager::on_game_state_change(game::State new_state,
                                         game::State old_state) {
    // log_warn("system manager on gamestate change from {} to {}",
    // old_state, new_state);

    if (old_state == game::State::InRound &&
        new_state == game::State::Planning) {
        state_transitioned_round_to_planning = true;
    }

    if (old_state == game::State::Planning &&
        new_state == game::State::InRound) {
        state_transitioned_planning_to_round = true;
    }
}

void SystemManager::update(const Entities& entities, float dt) {
    // TODO add num entities to debug overlay
    // log_info("num entities {}", entities.size());
    // TODO do we run game updates during paused?

    if (GameState::s_in_round()) {
        in_round_update(entities, dt);
    } else {
        planning_update(entities, dt);
    }

    always_update(entities, dt);
    process_state_change(entities, dt);
}

void SystemManager::update_all_entities(const Entities& players, float dt) {
    // TODO speed?
    Entities all;
    Entities ents = EntityHelper::get_entities();

    all.reserve(players.size() + ents.size());

    all.insert(all.end(), players.begin(), players.end());
    all.insert(all.end(), ents.begin(), ents.end());

    oldAll = all;

    update(all, dt);
}

void SystemManager::render_all_entities(const Entities&, float dt) const {
    // TODO figure out if its okay for us to throw out the updated players,
    // and use oldAll
    render_entities(oldAll, dt);
}

void SystemManager::render_all_ui(const Entities&, float dt) const {
    // TODO figure out if its okay for us to throw out the updated players,
    // and use oldAll
    render_ui(oldAll, dt);
}

void SystemManager::process_inputs(const Entities& entities,
                                   const UserInputs& inputs) {
    for (auto& entity : entities) {
        if (entity->is_missing<RespondsToUserInput>()) continue;
        for (auto input : inputs) {
            system_manager::input_process_manager::process_input(entity, input);
        }
    }
}

void SystemManager::process_state_change(
    const std::vector<std::shared_ptr<Entity>>& entities, float dt) {
    if (state_transitioned_round_to_planning) {
        state_transitioned_round_to_planning = false;

        for_each(entities, dt, [](std::shared_ptr<Entity> e_ptr, float dt) {
            Entity& entity = *e_ptr;
            // TODO make a namespace for transition functions
            system_manager::delete_held_items_when_leaving_inround(entity);
            system_manager::delete_customers_when_leaving_inround(entity);
            system_manager::reset_customer_spawner_when_leaving_inround(entity);
            system_manager::reset_max_gen_when_after_deletion(entity);
            system_manager::increment_day_count(entity, dt);
            // TODO reset haswork's
        });
    }

    if (state_transitioned_planning_to_round) {
        state_transitioned_planning_to_round = false;
        for_each(entities, dt, [](std::shared_ptr<Entity> e_ptr, float) {
            Entity& entity = *e_ptr;
            system_manager::handle_autodrop_furniture_when_exiting_planning(
                entity);
        });
    }

    // All transitions
    for_each(entities, dt, [](std::shared_ptr<Entity> e_ptr, float dt) {
        Entity& entity = *e_ptr;
        system_manager::refetch_dynamic_model_names(entity, dt);
    });
}

void SystemManager::always_update(const Entities& entities, float dt) {
    for_each(entities, dt, [](std::shared_ptr<Entity> e_ptr, float dt) {
        Entity& entity = *e_ptr;
        system_manager::reset_highlighted(entity, dt);
        // TODO should be just planning + lobby?
        // maybe a second one for highlighting items?
        system_manager::highlight_facing_furniture(entity, dt);
        system_manager::transform_snapper(entity, dt);
        system_manager::update_held_item_position(entity, dt);

        system_manager::process_trigger_area(entity, dt);

        // TODO this is in the render manager but its not really a
        // render thing but at the same time it kinda is idk This could
        // run only in lobby if we wanted to distinguish
        system_manager::render_manager::update_character_model_from_index(e_ptr,
                                                                          dt);

        system_manager::run_timer(entity, dt);
        system_manager::process_pnumatic_pipe_pairing(entity, dt);

        // TODO these eventually should move into their own functions but
        // for now >:)
        if (check_name(entity, strings::entity::SOPHIE))
            system_manager::sophie(entity, dt);
    });
}

void SystemManager::in_round_update(
    const std::vector<std::shared_ptr<Entity>>& entities, float dt) {
    for_each(entities, dt, [](std::shared_ptr<Entity> e_ptr, float dt) {
        Entity& entity = *e_ptr;
        //
        system_manager::reset_customers_that_need_resetting(entity);
        //
        system_manager::job_system::in_round_update(e_ptr, dt);
        system_manager::process_grabber_items(entity, dt);
        system_manager::process_conveyer_items(entity, dt);
        system_manager::process_grabber_filter(entity, dt);
        system_manager::process_pnumatic_pipe_movement(entity, dt);
        // should move all the container functions into its own
        // function?
        system_manager::process_is_container_and_should_backfill_item(entity,
                                                                      dt);
        system_manager::process_is_container_and_should_update_item(entity, dt);
        // This one should be after the other container ones
        system_manager::process_is_indexed_container_holding_incorrect_item(
            entity, dt);

        system_manager::process_has_rope(entity, dt);
        system_manager::process_spawner(entity, dt);
        system_manager::process_squirter(entity, dt);
        system_manager::process_trash(entity, dt);
        system_manager::reset_empty_work_furniture(entity, dt);
    });
}

void SystemManager::planning_update(
    const std::vector<std::shared_ptr<Entity>>& entities, float dt) {
    for_each(entities, dt, [](std::shared_ptr<Entity> entity_ptr, float dt) {
        Entity& entity = *entity_ptr;
        system_manager::update_held_furniture_position(entity, dt);
    });
}

void SystemManager::render_normal(
    const std::vector<std::shared_ptr<Entity>>& entity_list, float dt) const {
    for_each(entity_list, dt, [](std::shared_ptr<Entity> entity_ptr, float dt) {
        const Entity& entity = *entity_ptr;
        // TODO extract render normal into system facign functions
        system_manager::render_manager::render_normal(entity, dt);
        system_manager::render_manager::render_floating_name(entity, dt);
        system_manager::render_manager::render_progress_bar(entity, dt);
    });
}

void SystemManager::render_debug(
    const std::vector<std::shared_ptr<Entity>>& entity_list, float dt) const {
    for_each(entity_list, dt, [](std::shared_ptr<Entity> entity_ptr, float dt) {
        const Entity& entity = *entity_ptr;
        system_manager::render_manager::render_debug(entity, dt);
    });
}

void SystemManager::render_entities(const Entities& entities, float dt) const {
    const auto debug_mode_on =
        GLOBALS.get_or_default<bool>("debug_ui_enabled", false);
    if (debug_mode_on) {
        render_debug(entities, dt);
    }
    render_normal(entities, dt);
}

void SystemManager::render_ui(const Entities& entities, float dt) const {
    const auto debug_mode_on =
        GLOBALS.get_or_default<bool>("debug_ui_enabled", false);
    if (debug_mode_on) {
        system_manager::ui::render_debug_ui(entities, dt);
    }
    system_manager::ui::render_normal(entities, dt);
}
