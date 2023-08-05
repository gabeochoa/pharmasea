
#include "input_process_manager.h"

#include "../camera.h"
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
#include "../entity.h"
#include "../entityhelper.h"

namespace system_manager {

void person_update_given_new_pos(int id, Transform& transform,
                                 std::shared_ptr<Entity> person, float,
                                 vec3 new_pos_x, vec3 new_pos_z) {
    int facedir_x = -1;
    int facedir_z = -1;

    vec3 delta_distance_x = new_pos_x - transform.raw();
    if (delta_distance_x.x > 0) {
        facedir_x = Transform::FrontFaceDirection::RIGHT;
    } else if (delta_distance_x.x < 0) {
        facedir_x = Transform::FrontFaceDirection::LEFT;
    }

    vec3 delta_distance_z = new_pos_z - transform.raw();
    if (delta_distance_z.z > 0) {
        facedir_z = Transform::FrontFaceDirection::FORWARD;
    } else if (delta_distance_z.z < 0) {
        facedir_z = Transform::FrontFaceDirection::BACK;
    }

    if (facedir_x == -1 && facedir_z == -1) {
        // do nothing
    } else if (facedir_x == -1) {
        transform.update_face_direction(
            static_cast<Transform::FrontFaceDirection>(facedir_z));
    } else if (facedir_z == -1) {
        transform.update_face_direction(
            static_cast<Transform::FrontFaceDirection>(facedir_x));
    } else {
        transform.update_face_direction(
            static_cast<Transform::FrontFaceDirection>(facedir_x | facedir_z));
    }
    // TODO if anything spawns in anything else then it cant move,
    // we need some way to handle popping people back to spawn or something if
    // they get stuck

    // TODO this should be a component?
    {
        // horizontal check
        auto new_bounds_x = get_bounds(new_pos_x, transform.size());
        // vertical check
        auto new_bounds_y = get_bounds(new_pos_z, transform.size());

        bool would_collide_x = false;
        bool would_collide_z = false;
        std::weak_ptr<Entity> collided_entity_x;
        std::weak_ptr<Entity> collided_entity_z;
        EntityHelper::forEachEntity([&](auto entity) {
            if (id == entity->id) {
                return EntityHelper::ForEachFlow::Continue;
            }
            if (!is_collidable(entity, person)) {
                return EntityHelper::ForEachFlow::Continue;
            }
            if (!is_collidable(person)) {
                return EntityHelper::ForEachFlow::Continue;
            }
            if (CheckCollisionBoxes(
                    new_bounds_x, entity->template get<Transform>().bounds())) {
                would_collide_x = true;
                collided_entity_x = entity;
            }
            if (CheckCollisionBoxes(
                    new_bounds_y, entity->template get<Transform>().bounds())) {
                would_collide_z = true;
                collided_entity_z = entity;
            }
            // Note: if these are both true, then we definitely dont need to
            // keep going and can break early, otherwise we should check the
            // rest to make sure
            if (would_collide_x && would_collide_z) {
                return EntityHelper::ForEachFlow::Break;
            }
            return EntityHelper::ForEachFlow::NormalFlow;
        });

        const auto debug_mode_on =
            GLOBALS.get_or_default<bool>("debug_ui_enabled", false);
        if (debug_mode_on) {
            would_collide_x = false;
            would_collide_z = false;
        }

        if (!would_collide_x) {
            transform.update_x(new_pos_x.x);
        }
        if (!would_collide_z) {
            transform.update_z(new_pos_z.z);
        }

        // This value determines how "far" to impart a push force on the
        // collided entity
        const float directional_push_modifier = 1.0f;

        // Figure out if there's a more graceful way to "jitter" things
        // around each other
        const float tile_div_push_mod = TILESIZE / directional_push_modifier;

        if (would_collide_x || would_collide_z) {
            if (auto entity_x = collided_entity_x.lock()) {
                // TODO remove this check since we can just put CanBePushed
                // on the person entity and replace with a has<> check
                if (auto person_ptr_x = dynamic_cast<Entity*>(entity_x.get())) {
                    if (entity_x->has<CanBePushed>()) {
                        CanBePushed& cbp = entity_x->get<CanBePushed>();
                        const float random_jitter =
                            randSign() * TILESIZE / 2.0f;
                        if (facedir_x & Transform::FrontFaceDirection::LEFT) {
                            cbp.update({
                                cbp.pushed_force().x + tile_div_push_mod,
                                cbp.pushed_force().y,
                                cbp.pushed_force().z + random_jitter,
                            });
                        }
                        if (facedir_x & Transform::FrontFaceDirection::RIGHT) {
                            cbp.update({
                                cbp.pushed_force().x - tile_div_push_mod,
                                cbp.pushed_force().y,
                                cbp.pushed_force().z + random_jitter,
                            });
                        }
                    }
                }
            }
            if (auto entity_z = collided_entity_z.lock()) {
                // TODO remove this check since we can just put CanBePushed
                // on the person entity and replace with a has<> check
                if (auto person_ptr_z = dynamic_cast<Entity*>(entity_z.get())) {
                    if (entity_z->has<CanBePushed>()) {
                        CanBePushed& cbp = entity_z->get<CanBePushed>();
                        const float random_jitter =
                            randSign() * TILESIZE / 2.0f;
                        if (facedir_z &
                            Transform::FrontFaceDirection::FORWARD) {
                            cbp.update({
                                cbp.pushed_force().x + random_jitter,
                                cbp.pushed_force().y,
                                cbp.pushed_force().z + tile_div_push_mod,
                            });
                        }
                        if (facedir_z & Transform::FrontFaceDirection::BACK) {
                            cbp.update({
                                cbp.pushed_force().x + random_jitter,
                                cbp.pushed_force().y,
                                cbp.pushed_force().z - tile_div_push_mod,
                            });
                        }
                    }
                }
            }
        }
    }
}

namespace input_process_manager {

void collect_user_input(std::shared_ptr<Entity> entity, float dt) {
    if (entity->is_missing<CollectsUserInput>()) return;
    CollectsUserInput& cui = entity->get<CollectsUserInput>();

    // Theres no players not in game menu state,
    const menu::State state = menu::State::Game;

    float left, right, up, down;

    float key_left = KeyMap::is_event(state, InputName::PlayerLeft);
    float key_right = KeyMap::is_event(state, InputName::PlayerRight);
    float key_up = KeyMap::is_event(state, InputName::PlayerForward);
    float key_down = KeyMap::is_event(state, InputName::PlayerBack);

    // we need to rotate these controls based on the camera
    auto cam = GLOBALS.get_ptr<GameCam>("game_cam");
    auto camAngle = util::rad2deg(cam->angle.x);
    camAngle = std::fmod(camAngle, 360.0f);
    if (camAngle < 0.0f) {
        camAngle += 360.0f;
    }
    int xang = static_cast<int>(camAngle);
    // log_warn(" angles {}", xang);

    if (xang >= 135 && xang < 225) {
        // Default controls
        left = key_left;
        right = key_right;
        down = key_down;
        up = key_up;
    } else if (xang >= 45 && xang < 135) {
        up = key_left;
        down = key_right;
        left = key_down;
        right = key_up;
    } else if (xang >= 315 || xang <= 45) {
        left = key_right;
        right = key_left;
        up = key_down;
        down = key_up;
    } else if (xang >= 225 && xang < 315) {
        left = key_up;
        right = key_down;
        up = key_right;
        down = key_left;
    } else {
        log_warn("reached a camera angle that has no controls enabled: {}",
                 xang);
        // Default controls
        left = 0;
        right = 0;
        down = 0;
        up = 0;
    }

    if (left > 0) cui.write(InputName::PlayerLeft);
    if (right > 0) cui.write(InputName::PlayerRight);
    if (up > 0) cui.write(InputName::PlayerForward);
    if (down > 0) cui.write(InputName::PlayerBack);

    bool pickup =
        KeyMap::is_event_once_DO_NOT_USE(state, InputName::PlayerPickup);
    if (pickup) cui.write(InputName::PlayerPickup);

    bool rotate = KeyMap::is_event_once_DO_NOT_USE(
        state, InputName::PlayerRotateFurniture);
    if (rotate) cui.write(InputName::PlayerRotateFurniture);

    float do_work = KeyMap::is_event(state, InputName::PlayerDoWork);
    if (do_work > 0) cui.write(InputName::PlayerDoWork);

    // Actually save the inputs if there were any
    cui.publish(dt);
}

void process_player_movement_input(std::shared_ptr<Entity> entity, float dt,
                                   InputName input_name, float input_amount) {
    if (entity->is_missing<Transform>()) return;
    Transform& transform = entity->get<Transform>();
    std::shared_ptr<Entity> player = dynamic_pointer_cast<Entity>(entity);

    if (entity->is_missing<HasBaseSpeed>()) return;
    const HasBaseSpeed& hasBaseSpeed = entity->get<HasBaseSpeed>();

    const float speed = hasBaseSpeed.speed() * dt;
    auto new_position = transform.pos();

    if (input_name == InputName::PlayerLeft) {
        new_position.x -= input_amount * speed;
    } else if (input_name == InputName::PlayerRight) {
        new_position.x += input_amount * speed;
    } else if (input_name == InputName::PlayerForward) {
        new_position.z -= input_amount * speed;
    } else if (input_name == InputName::PlayerBack) {
        new_position.z += input_amount * speed;
    }

    person_update_given_new_pos(entity->id, transform, player, dt, new_position,
                                new_position);
    transform.trunc(2);
};

namespace planning {

void rotate_furniture(const std::shared_ptr<Entity> player) {
    // Cant rotate outside planning mode
    if (GameState::get().is_not(game::State::Planning)) return;

    // TODO need to figure out if reach should be separate from highlighting
    const CanHighlightOthers& cho = player->get<CanHighlightOthers>();

    std::shared_ptr<Furniture> match =
        EntityHelper::getClosestMatchingFurniture(
            player->get<Transform>(), cho.reach(), [](auto&& furniture) {
                return furniture->template has<IsRotatable>();
            });

    if (!match) return;
    match->get<Transform>().rotate_facing_clockwise();
}

void drop_held_furniture(Entity& player) {
    CanHoldFurniture& ourCHF = player.get<CanHoldFurniture>();
    std::shared_ptr<Furniture> hf = ourCHF.furniture();
    if (!hf) {
        log_info(" id:{} we'd like to drop but our hands are empty", player.id);
        return;
    }

    vec3 drop_location =
        vec::snap(vec::to3(player.get<Transform>().tile_infront(1)));

    bool can_place = EntityHelper::isWalkable(vec::to2(drop_location));

    if (can_place) {
        hf->get<CanBeHeld>().set_is_being_held(false);
        hf->get<Transform>().update(drop_location);

        ourCHF.update(nullptr);
        log_info("we {} dropped the furniture {} we were holding", player.id,
                 hf->id);

        EntityHelper::invalidatePathCache();
    }

    // TODO need to make sure it doesnt place ontop of another
    // one
    log_info("you cant place that here...");
}

// TODO grabbing reach needs to be better, you should be able to grab in the 8
// spots around you and just prioritize the one you are facing
//

void handle_grab_or_drop(const std::shared_ptr<Entity>& player) {
    // TODO need to figure out if this should be separate from highlighting
    const CanHighlightOthers& cho = player->get<CanHighlightOthers>();
    CanHoldFurniture& ourCHF = player->get<CanHoldFurniture>();

    if (ourCHF.is_holding_furniture()) {
        drop_held_furniture(*player);
        return;
    } else {
        // TODO support finding things in the direction the player is
        // facing, instead of in a box around him

        std::shared_ptr<Furniture> closest_furniture =
            EntityHelper::getClosestMatchingFurniture(
                player->get<Transform>(), cho.reach(),
                [](std::shared_ptr<Furniture> f) {
                    // TODO right now walls inherit this from furniture but
                    // eventually that should not be the case
                    if (f->is_missing<CanBeHeld>()) return false;
                    return f->get<CanBeHeld>().is_not_held();
                });
        // no match
        if (!closest_furniture) return;

        ourCHF.update(closest_furniture);
        ourCHF.furniture()->get<CanBeHeld>().set_is_being_held(true);

        // TODO
        // NOTE: we want to remove the furniture ONLY from the nav mesh
        //       when picked up because then AI can walk through,
        //       this also means we have to add it back when we place it
        // if (this->held_furniture) {
        // auto nv = GLOBALS.get_ptr<NavMesh>("navmesh");
        // nv->removeEntity(this->held_furniture->id);
        // }
        return;
    }
}

}  // namespace planning

namespace inround {

void work_furniture(const std::shared_ptr<Entity> player, float frame_dt) {
    // Cant do work during planning
    if (GameState::get().is(game::State::Planning)) return;

    // TODO need to figure out if this should be separate from highlighting
    const CanHighlightOthers& cho = player->get<CanHighlightOthers>();

    std::shared_ptr<Furniture> match =
        EntityHelper::getClosestMatchingFurniture(
            player->get<Transform>(), cho.reach(), [](auto&& furniture) {
                if (furniture->template is_missing<HasWork>()) return false;
                const HasWork& hasWork = furniture->template get<HasWork>();
                return hasWork.has_work();
            });

    if (!match) return;

    match->get<HasWork>().call(*match, *player, frame_dt);
}

void handle_drop(const std::shared_ptr<Entity>& player) {
    const CanHighlightOthers& cho = player->get<CanHighlightOthers>();

    // This is for example putting a pill into a bag you are holding
    // taking an item and placing it into the container in your hand
    const auto _merge_item_from_furniture_into_hand_item =
        [player]() -> tl::expected<bool, std::string> {
        const CanHighlightOthers& cho = player->get<CanHighlightOthers>();

        std::shared_ptr<Item> item = player->get<CanHoldItem>().item();

        if (item->is_missing<CanHoldItem>()) {
            return tl::unexpected(
                "trying to merge from furniture, but item can not hold things");
        }

        CanHoldItem& item_chi = item->get<CanHoldItem>();

        // TODO replace !empty() with full()
        // our item is already full
        if (!item_chi.empty()) {
            return tl::unexpected(
                "trying to merge from furniture, but item was not empty");
        }

        std::shared_ptr<Furniture> closest_furniture =
            EntityHelper::getClosestMatchingFurniture(
                player->get<Transform>(), cho.reach(),
                [](std::shared_ptr<Furniture> f) {
                    if (f->is_missing<CanHoldItem>()) return false;
                    return f->get<CanHoldItem>().is_holding_item();
                });

        if (!closest_furniture) {
            return tl::unexpected(
                "trying to merge from furniture, but didnt find anything "
                "holding something");
        }

        std::shared_ptr<Item> item_to_merge =
            closest_furniture->get<CanHoldItem>().item();

        // TODO this check should be probably be int he furniture check
        if (!item_chi.can_hold(*item_to_merge, RespectFilter::All)) {
            return tl::unexpected(
                "trying to merge from furniture, but we cant hold"
                "that kind of item ");
        }

        // TODO probably need to have heldby updated here

        // Our item takes ownership of the item
        item_chi.update(item_to_merge);
        // furniture lets go
        closest_furniture->get<CanHoldItem>().update(nullptr);
        return true;
    };

    // This is like placing an item onto a plate and immediately picking it up
    // it should only apply when the merged item isnt valid in that spot anymore
    // ie only empty bags go in bagbox so a bag containg something shouldnt go
    // back
    //
    /*
     * TODO since this exists as a way to handle containers
     * maybe short circuit earlier by doing?
        static bool is_an_item_container(Entity* e) {
            return e->has_any<                //
                IsItemContainer<Pill>,        //
                IsItemContainer<PillBottle>,  //
                IsItemContainer<Bag>          //
                >();
        }
     * */
    const auto _merge_item_from_furniture_around_hand_item =
        [player]() -> tl::expected<bool, std::string> {
        const CanHighlightOthers& cho = player->get<CanHighlightOthers>();
        CanHoldItem& playerCHI = player->get<CanHoldItem>();

        if (!playerCHI.is_holding_item()) {
            return tl::unexpected(
                "trying to merge from furniture around hand, but player wasnt "
                "holding anything");
        }

        std::shared_ptr<Furniture> closest_furniture =
            EntityHelper::getClosestMatchingFurniture(
                player->get<Transform>(), cho.reach(),
                [&playerCHI](std::shared_ptr<Furniture> f) {
                    if (f->is_missing<CanHoldItem>()) return false;
                    const auto& chi = f->get<CanHoldItem>();
                    if (chi.empty()) return false;
                    // TODO we are using const_item() but this doesnt
                    // enforce const and is just for us to understand
                    const std::shared_ptr<Entity> item = chi.const_item();

                    // Does the item this furniture holds have the ability to
                    // hold things
                    if (item->is_missing<CanHoldItem>()) return false;

                    // Can it hold the thing we are holding?
                    const CanHoldItem& item_chi = item->get<CanHoldItem>();
                    if (!item_chi.can_hold(*playerCHI.item(),
                                           RespectFilter::All))
                        return false;

                    return true;
                });

        if (!closest_furniture) {
            return tl::unexpected(
                "trying to merge from furniture around hand, but didnt find "
                "anything than can hold what we are holding");
        }

        std::shared_ptr<Item> item_to_merge =
            closest_furniture->get<CanHoldItem>().item();

        CanHoldItem& merge_chi = item_to_merge->get<CanHoldItem>();

        // TODO probably need to have heldby updated here
        // Their item takes ownership of the item
        merge_chi.update(playerCHI.item());

        // remove the link to the one we were already holding
        player->get<CanHoldItem>().update(nullptr);
        // add the link to the new one
        player->get<CanHoldItem>().update(item_to_merge);
        // furniture can let go
        closest_furniture->get<CanHoldItem>().update(nullptr);
        return true;
    };

    // TODO is there a time when this function gets called?
    // i feel like the wrap-around function will handle this case
    const auto _merge_item_in_hand_into_furniture_item =
        [&]() -> tl::expected<bool, std::string> {
        std::shared_ptr<Furniture> closest_furniture =
            EntityHelper::getClosestMatchingFurniture(
                player->get<Transform>(), cho.reach(),
                [&](std::shared_ptr<Furniture> f) {
                    if (f->is_missing<CanHoldItem>()) return false;
                    CanHoldItem& fCHI = f->get<CanHoldItem>();
                    // is there something there to merge into?
                    if (!fCHI.is_holding_item()) return false;
                    // Grab furniture item
                    const auto furn_item = fCHI.item();
                    // Does the item this furniture holds have the ability to
                    // hold things
                    if (furn_item->is_missing<CanHoldItem>()) return false;
                    // Grab the item we are holding...
                    const auto player_item = player->get<CanHoldItem>().item();
                    // Can it hold the thing we are holding?
                    if (!fCHI.can_hold(*player_item, RespectFilter::All))
                        return false;
                    return true;
                });

        // No matching furniture
        if (!closest_furniture)
            return tl::unexpected("merge hand into: no matching furniture");

        // TODO need to handle the case where the merged item is not a
        // valid thing the furniture can hold.
        //
        // This happens for example when you merge into a supply cache.
        // Because the supply can only hold the container and not a
        // filled one... In this case we should either:
        // - block the merge
        // - place the merged item into the player's hand

        CanHoldItem& fCHI = closest_furniture->get<CanHoldItem>();

        std::shared_ptr<Item> f_item = fCHI.item();
        const auto player_item = player->get<CanHoldItem>().item();

        // Their item eats our item
        f_item->get<CanHoldItem>().update(player_item);
        player->get<CanHoldItem>().update(nullptr);

        return true;
    };

    const auto _place_item_onto_furniture =
        [&]() -> tl::expected<bool, std::string> {
        std::shared_ptr<Furniture> closest_furniture =
            EntityHelper::getClosestMatchingFurniture(
                player->get<Transform>(), cho.reach(),
                [player](std::shared_ptr<Furniture> f) {
                    // This cant hold anything
                    if (f->is_missing<CanHoldItem>()) return false;
                    const CanHoldItem& furnCanHold = f->get<CanHoldItem>();

                    std::shared_ptr<Item> item =
                        player->get<CanHoldItem>().item();

                    const auto item_container_is_matching_item =
                        [](std::shared_ptr<Entity> entity,
                           std::shared_ptr<Item> item = nullptr) {
                            if (!item) return false;
                            if (!entity) return false;
                            if (entity->is_missing<IsItemContainer>())
                                return false;
                            IsItemContainer& itemContainer =
                                entity->get<IsItemContainer>();
                            return itemContainer.is_matching_item(item);
                        };

                    // Handle item containers
                    bool matches_item =
                        item_container_is_matching_item(f, item);
                    if (matches_item) return true;

                    // This check has to go after the item containers, for
                    // putting back into supply to work
                    if (!furnCanHold.empty()) return false;

                    // Can it hold the item we are trying to drop
                    return furnCanHold.can_hold(
                        *item,
                        // dont worry about suggested filters
                        RespectFilter::ReqOnly);
                });

        // no matching furniture
        if (!closest_furniture)
            return tl::unexpected("place_onto: no matching furniture");

        const Transform& furnT = closest_furniture->get<Transform>();
        CanHoldItem& furnCHI = closest_furniture->get<CanHoldItem>();

        std::shared_ptr<Item> item = player->get<CanHoldItem>().item();
        item->get<Transform>().update(furnT.snap_position());

        if (closest_furniture->has<IsItemContainer>() &&
            closest_furniture->get<IsItemContainer>().is_matching_item(item)) {
            // So in this case,
            // if theres already an item there, then we just have to delete
            // the one we are holding.
            //
            // if theres nothing there, then we do the normal drop logic
            if (furnCHI.is_holding_item()) {
                player->get<CanHoldItem>().item()->cleanup = true;
                player->get<CanHoldItem>().update(nullptr);
                return true;
            }
        }

        furnCHI.update(item);
        player->get<CanHoldItem>().update(nullptr);
        return true;
    };

    typedef std::function<tl::expected<bool, std::string>()> MergeFunc;
    // NOTE: ORDER MATTERS HERE
    std::vector<MergeFunc> fns{
        // _merge_item_from_furniture_into_hand_item,
        // _merge_item_from_furniture_around_hand_item,
        // _merge_item_in_hand_into_furniture_item,
        _place_item_onto_furniture,
    };

    for (auto fn : fns) {
        auto item_merged = fn();
        if (item_merged) break;
        log_info("{}", item_merged.error());
    }

    return;
}

void handle_grab(const std::shared_ptr<Entity>& player) {
    const auto _try_to_pickup_item_from_furniture = [player]() {
        const CanHighlightOthers& cho = player->get<CanHighlightOthers>();
        const Transform& playerT = player->get<Transform>();

        std::shared_ptr<Furniture> closest_furniture =
            EntityHelper::getClosestMatchingFurniture(
                playerT, cho.reach(),
                [player](std::shared_ptr<Furniture> furn) -> bool {
                    // This furniture can never hold anything so no match
                    if (furn->is_missing<CanHoldItem>()) return false;
                    // its not holding something
                    if (furn->get<CanHoldItem>().empty()) return false;

                    auto item = furn->get<CanHoldItem>().const_item();
                    // Can we hold the item it has?
                    return player->get<CanHoldItem>().can_hold(
                        *item, RespectFilter::All);
                });

        // No matching furniture that also can hold item
        if (!closest_furniture) return false;

        // we found a match, grab the item from it
        CanHoldItem& furnCanHold = closest_furniture->get<CanHoldItem>();
        std::shared_ptr<Item> item = furnCanHold.item();

        // log_info("Found {} to pick up from {}", item->get<DebugName>(),
        // closest_furniture->get<DebugName>());

        CanHoldItem& playerCHI = player->get<CanHoldItem>();
        playerCHI.update(item);
        item->get<Transform>().update(player->get<Transform>().snap_position());
        furnCanHold.update(nullptr);
        return true;
    };

    bool picked_up_item = _try_to_pickup_item_from_furniture();
    if (picked_up_item) return;

    // Handles the non-furniture grabbing case
    const CanHighlightOthers& cho = player->get<CanHighlightOthers>();

    std::shared_ptr<Item> closest_item =
        EntityHelper::getClosestMatchingEntity<Item>(
            player->get<Transform>().as2(), TILESIZE * cho.reach(),
            [](const std::shared_ptr<Item> entity) {
                if (entity->is_missing<IsItem>()) return false;
                if (entity->get<IsItem>().is_held()) return false;
                return true;
            });

    // nothing found
    if (closest_item == nullptr) return;

    player->get<CanHoldItem>().update(closest_item);
    return;
}

void handle_grab_or_drop(const std::shared_ptr<Entity>& player) {
    // TODO Need to auto drop any held furniture

    // Do we already have something in our hands?
    // We must be trying to drop it
    player->get<CanHoldItem>().empty() ? handle_grab(player)
                                       : handle_drop(player);
}

}  // namespace inround

void process_input(const std::shared_ptr<Entity> entity,
                   const UserInput& input) {
    const auto _proc_single_input_name =
        [](const std::shared_ptr<Entity> entity, const InputName& input_name,
           float frame_dt) {
            switch (input_name) {
                case InputName::PlayerLeft:
                case InputName::PlayerRight:
                case InputName::PlayerForward:
                case InputName::PlayerBack:
                    return process_player_movement_input(entity, frame_dt,
                                                         input_name, 1.f);
                default:
                    break;
            }

            switch (input_name) {
                case InputName::PlayerRotateFurniture:
                    planning::rotate_furniture(entity);
                    break;
                case InputName::PlayerPickup:
                    // grab_or_drop(entity);
                    {
                        if (GameState::s_in_round()) {
                            inround::handle_grab_or_drop(entity);
                        } else if (GameState::get().is(game::State::Planning)) {
                            planning::handle_grab_or_drop(entity);
                        } else {
                            // TODO we probably want to handle messing around in
                            // the lobby
                        }
                    }
                    break;
                case InputName::PlayerDoWork:
                    inround::work_furniture(entity, frame_dt);
                default:
                    break;
            }
        };

    const InputSet input_set = std::get<0>(input);
    const float frame_dt = std::get<1>(input);

    int i = 0;
    while (i < InputName::Last) {
        auto input_name = magic_enum::enum_value<InputName>(i);
        bool was_pressed = input_set.test(i);
        if (was_pressed) {
            _proc_single_input_name(entity, input_name, frame_dt);
        }
        i++;
    }
}
}  // namespace input_process_manager
}  // namespace system_manager