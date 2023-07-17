

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
void transform_snapper(std::shared_ptr<Entity> entity, float) {
    if (entity->is_missing<Transform>()) return;
    Transform& transform = entity->get<Transform>();
    transform.update(entity->has<IsSnappable>() ? transform.snap_position()
                                                : transform.raw());
}

// TODO if cannot be placed in this spot make it obvious to the user
void update_held_furniture_position(std::shared_ptr<Entity> entity, float) {
    if (entity->is_missing_any<Transform, CanHoldFurniture>()) return;

    const Transform& transform = entity->get<Transform>();

    CanHoldFurniture& can_hold_furniture = entity->get<CanHoldFurniture>();
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
void update_held_item_position(std::shared_ptr<Entity> entity, float) {
    if (entity->is_missing<CanHoldItem>()) return;

    CanHoldItem& can_hold_item = entity->get<CanHoldItem>();
    if (can_hold_item.empty()) return;

    const Transform& transform = entity->get<Transform>();

    vec3 new_pos = transform.pos();

    if (entity->has<CustomHeldItemPosition>()) {
        CustomHeldItemPosition& custom_item_position =
            entity->get<CustomHeldItemPosition>();

        switch (custom_item_position.positioner) {
            case CustomHeldItemPosition::Positioner::Default:
                new_pos.y += TILESIZE / 4;
                break;
            case CustomHeldItemPosition::Positioner::Table:
                new_pos.y += TILESIZE / 2;
                break;
            case CustomHeldItemPosition::Positioner::ItemHoldingItem:
                new_pos.x += 0;
                new_pos.y += 0;
                break;
            case CustomHeldItemPosition::Positioner::Conveyer:
                if (entity->is_missing<ConveysHeldItem>()) {
                    log_warn(
                        "A conveyer positioned item needs ConveysHeldItem");
                    break;
                }
                ConveysHeldItem& conveysHeldItem =
                    entity->get<ConveysHeldItem>();
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
                break;
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

void reset_highlighted(std::shared_ptr<Entity> entity, float) {
    if (entity->is_missing<CanBeHighlighted>()) return;
    CanBeHighlighted& cbh = entity->get<CanBeHighlighted>();
    cbh.update(false);
}

void highlight_facing_furniture(std::shared_ptr<Entity> entity, float) {
    Transform& transform = entity->get<Transform>();
    if (entity->is_missing<CanHighlightOthers>()) return;
    // TODO add a player reach component
    CanHighlightOthers& cho = entity->get<CanHighlightOthers>();

    auto match = EntityHelper::getClosestMatchingFurniture(
        transform, cho.reach(),
        [](auto e) { return e->template has<CanBeHighlighted>(); });
    if (!match) return;

    match->get<CanBeHighlighted>().update(true);
}

// TODO We need like a temporary storage for this
void move_entity_based_on_push_force(std::shared_ptr<Entity> entity, float,
                                     vec3& new_pos_x, vec3& new_pos_z) {
    CanBePushed& cbp = entity->get<CanBePushed>();

    new_pos_x.x += cbp.pushed_force().x;
    cbp.update_x(0.0f);

    new_pos_z.z += cbp.pushed_force().z;
    cbp.update_z(0.0f);
}

void process_conveyer_items(std::shared_ptr<Entity> entity, float dt) {
    Transform& transform = entity->get<Transform>();
    if (entity->is_missing_any<CanHoldItem, ConveysHeldItem, CanBeTakenFrom>())
        return;

    CanHoldItem& canHold = entity->get<CanHoldItem>();
    CanBeTakenFrom& canBeTakenFrom = entity->get<CanBeTakenFrom>();
    ConveysHeldItem& conveysHeldItem = entity->get<ConveysHeldItem>();

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

    auto match = EntityHelper::getClosestMatchingFurniture(
        transform, 1.f, [entity](std::shared_ptr<Furniture> furn) {
            // cant be us
            if (entity->id == furn->id) return false;
            // needs to be able to hold something
            if (furn->is_missing<CanHoldItem>()) return false;
            // has to be empty
            return furn->get<CanHoldItem>().empty();
        });

    // no match means we can't continue, stay in the middle
    if (!match) {
        conveysHeldItem.relative_item_pos = 0.f;
        canBeTakenFrom.update(true);
        return;
    }

    // we got something that will take from us,
    // but only once we get close enough

    // so keep moving forward
    if (conveysHeldItem.relative_item_pos <= ConveysHeldItem::ITEM_END) {
        conveysHeldItem.relative_item_pos += conveysHeldItem.SPEED * dt;
        return;
    }

    // we reached the end, pass ownership

    CanHoldItem& ourCHI = entity->get<CanHoldItem>();

    CanHoldItem& matchCHI = match->get<CanHoldItem>();
    matchCHI.update(ourCHI.item());

    ourCHI.update(nullptr);

    canBeTakenFrom.update(true);  // we are ready to have someone grab from us
    // reset so that the next item we get starts from beginning
    conveysHeldItem.relative_item_pos = ConveysHeldItem::ITEM_START;

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
void process_grabber_items(std::shared_ptr<Entity> entity, float) {
    Transform& transform = entity->get<Transform>();

    if (entity->is_missing<CanHoldItem>()) return;
    CanHoldItem& canHold = entity->get<CanHoldItem>();
    // we are already holding something so
    if (canHold.is_holding_item()) return;

    // Should only run this for conveyers
    if (entity->is_missing<ConveysHeldItem>()) return;
    // Should only run for grabbers
    if (entity->is_missing<CanGrabFromOtherFurniture>()) return;

    ConveysHeldItem& conveysHeldItem = entity->get<ConveysHeldItem>();

    auto behind =
        transform.offsetFaceDirection(transform.face_direction(), 180);
    auto match = EntityHelper::getMatchingEntityInFront<Furniture>(
        transform.as2(), 1.f, behind,
        [entity](std::shared_ptr<Furniture> furn) {
            // cant be us
            if (entity->id == furn->id) return false;
            // needs to be able to hold something
            if (furn->is_missing<CanHoldItem>()) return false;
            // doesnt have anything
            if (furn->get<CanHoldItem>().empty()) return false;
            // We only check CanBe when it exists because everyone else can
            // always be taken from with a grabber
            if (furn->is_missing<CanBeTakenFrom>()) return true;
            return furn->get<CanBeTakenFrom>().can_take_from();
        });

    // No furniture behind us
    if (!match) return;

    // Grab from the furniture match
    CanHoldItem& matchCHI = match->get<CanHoldItem>();
    CanHoldItem& ourCHI = entity->get<CanHoldItem>();

    ourCHI.update(matchCHI.item());
    matchCHI.update(nullptr);

    conveysHeldItem.relative_item_pos = ConveysHeldItem::ITEM_START;
}

template<typename... TArgs>
void backfill_empty_container(const std::string& match_type,
                              std::shared_ptr<Entity>& entity,
                              TArgs&&... args) {
    if (entity->is_missing<IsItemContainer>()) return;
    IsItemContainer& iic = entity->get<IsItemContainer>();
    if (iic.type() != match_type) return;
    CanHoldItem& canHold = entity->get<CanHoldItem>();
    if (canHold.is_holding_item()) return;

    if (iic.hit_max()) return;
    iic.increment();

    // create item
    std::shared_ptr<Item> item =
        EntityHelper::createItem(iic.type(), std::forward<TArgs>(args)...);

    canHold.update(item);
}

void process_is_container_and_should_backfill_item(
    std::shared_ptr<Entity> entity, float) {
    if (entity->is_missing<CanHoldItem>()) return;
    CanHoldItem& canHold = entity->get<CanHoldItem>();
    if (canHold.is_holding_item()) return;

    // TODO speed have each <> return true/false if it worked
    //      then can skip the rest, are there any that can hold mixed ?
    // TODO can we dynamically figure out what to run?

    auto pos = entity->get<Transform>().as2();

    backfill_empty_container(strings::item::BAG, entity, pos);
    backfill_empty_container(strings::item::PILL_BOTTLE, entity, pos);
    backfill_empty_container(strings::item::SODA_SPOUT, entity, pos);

    if (entity->is_missing<Indexer>()) return;
    backfill_empty_container(strings::item::PILL, entity, pos,
                             entity->get<Indexer>().value());
    backfill_empty_container(strings::item::ALCOHOL, entity, pos,
                             entity->get<Indexer>().value());
    entity->get<Indexer>().mark_change_completed();
}

void process_is_container_and_should_update_item(std::shared_ptr<Entity> entity,
                                                 float) {
    if (entity->is_missing<Indexer>()) return;
    Indexer& indexer = entity->get<Indexer>();
    // user didnt change the index so we are good to wait
    if (indexer.value_same_as_last_render()) return;

    if (entity->is_missing<CanHoldItem>()) return;
    CanHoldItem& canHold = entity->get<CanHoldItem>();

    // Delete the currently held item
    if (canHold.is_holding_item()) {
        canHold.item()->cleanup = true;
        canHold.update(nullptr);
    }

    auto pos = entity->get<Transform>().as2();

    backfill_empty_container(strings::item::PILL, entity, pos, indexer.value());
    backfill_empty_container(strings::item::ALCOHOL, entity, pos,
                             indexer.value());
    indexer.mark_change_completed();
}

void handle_autodrop_furniture_when_exiting_planning(
    const std::shared_ptr<Entity>& entity) {
    if (entity->is_missing<CanHoldFurniture>()) return;

    CanHoldFurniture& ourCHF = entity->get<CanHoldFurniture>();
    if (ourCHF.empty()) return;

    // TODO need to find a spot it can go in using EntityHelper::isWalkable
    input_process_manager::planning::drop_held_furniture(entity);
}

void delete_held_items_when_leaving_inround(
    const std::shared_ptr<Entity>& entity) {
    // TODO this doesnt seem to work
    // you keep holding it even after the transition

    if (entity->is_missing<CanHoldItem>()) return;

    CanHoldItem& canHold = entity->get<CanHoldItem>();
    if (canHold.empty()) return;

    // Mark it as deletable
    std::shared_ptr<Item>& item = canHold.item();

    // let go of the item
    item->cleanup = true;
    canHold.update(nullptr);
}

void refetch_dynamic_model_names(const std::shared_ptr<Entity>& entity, float) {
    if (entity->is_missing<ModelRenderer>()) return;
    if (entity->is_missing<HasDynamicModelName>()) return;

    HasDynamicModelName& hDMN = entity->get<HasDynamicModelName>();
    ModelRenderer& renderer = entity->get<ModelRenderer>();
    renderer.update_model_name(hDMN.fetch(*entity));
}

void count_max_trigger_area_entrants(const std::shared_ptr<Entity>& entity,
                                     float) {
    if (entity->is_missing<IsTriggerArea>()) return;

    int count = 0;
    for (auto& e : SystemManager::get().oldAll) {
        if (!check_name(*e, strings::entity::PLAYER)) continue;
        count++;
    }
    entity->get<IsTriggerArea>().update_max_entrants(count);
}

void count_trigger_area_entrants(const std::shared_ptr<Entity>& entity, float) {
    if (entity->is_missing<IsTriggerArea>()) return;

    int count = 0;
    for (auto& e : SystemManager::get().oldAll) {
        if (!check_name(*e, strings::entity::PLAYER)) continue;
        if (CheckCollisionBoxes(
                e->get<Transform>().bounds(),
                entity->get<Transform>().expanded_bounds({0, TILESIZE, 0}))) {
            count++;
        }
    }
    entity->get<IsTriggerArea>().update_entrants(count);
}

void update_trigger_area_percent(const std::shared_ptr<Entity>& entity,
                                 float dt) {
    if (entity->is_missing<IsTriggerArea>()) return;
    IsTriggerArea& ita = entity->get<IsTriggerArea>();
    if (ita.should_wave()) {
        ita.increase_progress(dt);
    } else {
        ita.decrease_progress(dt);
    }
}

void trigger_cb_on_full_progress(const std::shared_ptr<Entity>& entity, float) {
    if (entity->is_missing<IsTriggerArea>()) return;
    IsTriggerArea& ita = entity->get<IsTriggerArea>();
    if (ita.progress() < 1.f) return;
    auto cb = ita.get_complete_fn();
    if (cb) cb();
}

void process_trigger_area(const std::shared_ptr<Entity>& entity, float dt) {
    count_max_trigger_area_entrants(entity, dt);
    count_trigger_area_entrants(entity, dt);
    update_trigger_area_percent(entity, dt);
    trigger_cb_on_full_progress(entity, dt);
}

void process_spawner(const std::shared_ptr<Entity>& entity, float dt) {
    if (entity->is_missing<IsSpawner>()) return;
    auto pos = entity->get<Transform>().as2();
    entity->get<IsSpawner>().pass_time(pos, dt);
}

void run_timer(const std::shared_ptr<Entity>& entity, float dt) {
    if (entity->is_missing<HasTimer>()) return;
    entity->get<HasTimer>().pass_time(dt).reset_if_complete(dt);
}

void sophie(const std::shared_ptr<Entity>& entity, float) {
    if (entity->is_missing<HasTimer>()) return;

    // Handle customers finally leaving the store
    auto _customers_in_store = [entity]() {
        // TODO with the others siwtch to something else... customer
        // spawner?
        const auto endpos = vec2{GATHER_SPOT, GATHER_SPOT};

        bool all_gone = true;
        std::vector<std::shared_ptr<Entity>> customers =
            EntityHelper::getAllWithName(strings::entity::CUSTOMER);
        for (std::shared_ptr<Entity> e : customers) {
            if (vec::distance(e->get<Transform>().as2(), endpos) >
                TILESIZE * 2.f) {
                all_gone = false;
                break;
            }
        }
        entity->get<HasTimer>().write_reason(
            HasTimer::WaitingReason::CustomersInStore, !all_gone);
        return;
    };

    // Handle some player is holding furniture
    auto _player_holding_furniture = [entity]() {
        bool all_empty = true;
        // TODO i want to to do it this way: but players are not in
        // entities, so its not possible
        //
        // auto players =
        // EntityHelper::getAllWithComponent<CanHoldFurniture>();

        for (std::shared_ptr<Entity> e : SystemManager::get().oldAll) {
            if (e->is_missing<CanHoldFurniture>()) continue;
            if (e->get<CanHoldFurniture>().is_holding_furniture()) {
                all_empty = false;
                break;
            }
        }
        entity->get<HasTimer>().write_reason(
            HasTimer::WaitingReason::HoldingFurniture, !all_empty);
        return;
    };

    // TODO merge with map generation validation?
    // Run lightweight map validation
    auto _lightweight_map_validation = [entity]() {
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

        entity->get<HasTimer>().write_reason(
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

void reset_empty_work_furniture(const std::shared_ptr<Entity>& entity,
                                float dt) {
    if (entity->is_missing<HasWork>()) return;
    if (entity->is_missing<CanHoldItem>()) return;

    HasWork& hasWork = entity->get<HasWork>();
    if (!hasWork.should_reset_on_empty()) return;

    const CanHoldItem& chi = entity->get<CanHoldItem>();
    if (chi.empty()) {
        hasWork.reset_pct();
        return;
    }

    // TODO if its not empty, we have to see if its an item that can be worked
    return;
}

void process_has_rope(const std::shared_ptr<Entity>& entity, float) {
    if (entity->is_missing<CanHoldItem>()) return;
    if (entity->is_missing<HasRopeToItem>()) return;

    HasRopeToItem& hrti = entity->get<HasRopeToItem>();

    // No need to have rope if spout is put away
    const CanHoldItem& chi = entity->get<CanHoldItem>();
    if (chi.is_holding_item()) {
        hrti.clear();
        return;
    }

    OptEntity opt_player;
    for (std::shared_ptr<Entity> e : SystemManager::get().oldAll) {
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

    auto new_path = astar::find_path(entity->get<Transform>().as2(), pos,
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

}  // namespace system_manager
