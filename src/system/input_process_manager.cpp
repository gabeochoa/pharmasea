
#include "input_process_manager.h"

#include "../building_locations.h"
#include "../camera.h"
#include "../components/can_be_ghost_player.h"
#include "../components/can_be_pushed.h"
#include "../components/can_grab_from_other_furniture.h"
#include "../components/can_highlight_others.h"
#include "../components/can_hold_furniture.h"
#include "../components/can_perform_job.h"
#include "../components/collects_user_input.h"
#include "../components/conveys_held_item.h"
#include "../components/custom_item_position.h"
#include "../components/has_base_speed.h"
#include "../components/has_work.h"
#include "../components/is_item_container.h"
#include "../components/is_rotatable.h"
#include "../components/is_snappable.h"
#include "../components/responds_to_user_input.h"
#include "../components/transform.h"
#include "../entity.h"
#include "../entity_helper.h"
#include "../network/server.h"
#include "expected.hpp"

namespace system_manager {

void person_update_given_new_pos(
    int id, Transform& transform,
    // this could be const but is_collidable has no way to convert
    // between const entity& and OptEntity
    Entity& person, float, vec3 new_pos_x, vec3 new_pos_z) {
    // TODO this should be a component?
    {
        // horizontal check
        auto new_bounds_x = get_bounds(new_pos_x, transform.size());
        // vertical check
        auto new_bounds_y = get_bounds(new_pos_z, transform.size());

        OptEntity collided_entity_x;
        OptEntity collided_entity_z;
        EntityHelper::forEachEntity([&](Entity& entity) {
            if (id == entity.id) {
                return EntityHelper::ForEachFlow::Continue;
            }
            if (!system_manager::input_process_manager::is_collidable(entity,
                                                                      person)) {
                return EntityHelper::ForEachFlow::Continue;
            }
            if (!system_manager::input_process_manager::is_collidable(person)) {
                return EntityHelper::ForEachFlow::Continue;
            }
            if (CheckCollisionBoxes(
                    new_bounds_x, entity.template get<Transform>().bounds())) {
                collided_entity_x = entity;
            }
            if (CheckCollisionBoxes(
                    new_bounds_y, entity.template get<Transform>().bounds())) {
                collided_entity_z = entity;
            }
            // Note: if these are both true, then we definitely dont need to
            // keep going and can break early, otherwise we should check the
            // rest to make sure
            if (collided_entity_x && collided_entity_z) {
                return EntityHelper::ForEachFlow::Break;
            }
            return EntityHelper::ForEachFlow::NormalFlow;
        });

        const auto debug_mode_on =
            GLOBALS.get_or_default<bool>("no_clip_enabled", false);
        if (debug_mode_on) {
            collided_entity_x = {};
            collided_entity_z = {};
        }
        //
        if (!collided_entity_x) {
            transform.update_x(new_pos_x.x);
        }
        if (!collided_entity_z) {
            transform.update_z(new_pos_z.z);
        }
    }
}

namespace input_process_manager {

// return true if the item has collision and is currently collidable
bool is_collidable(const Entity& entity, OptEntity other) {
    // by default we disable collisions when you are holding something
    // since its generally inside your bounding box
    if (entity.has<CanBeHeld>() && entity.get<CanBeHeld>().is_held()) {
        return false;
    }

    if (entity.has<CanBeHeld_HT>() && entity.get<CanBeHeld_HT>().is_held()) {
        return false;
    }

    if (check_type(entity, EntityType::MopBuddy)) {
        if (other && check_type(other.asE(), EntityType::MopBuddyHolder)) {
            return false;
        }
    }

    if (
        // checking for person update
        other &&
        // Entity is item and held by player
        entity.has<IsItem>() &&
        entity.get<IsItem>().is_held_by(EntityType::Player) &&
        // Entity is rope
        check_type(entity, EntityType::SodaSpout) &&
        // we are a player that is holding rope
        other->has<CanHoldItem>() &&
        other->get<CanHoldItem>().is_holding_item() &&
        check_type(other->get<CanHoldItem>().item(), EntityType::SodaSpout)) {
        return false;
    }

    if (entity.has<IsSolid>()) {
        return true;
    }

    // TODO :BE: rename this since it no longer makes sense
    // if you are a ghost player
    // then you are collidable
    if (entity.has<CanBeGhostPlayer>()) {
        return true;
    }
    return false;
}

void collect_user_input(Entity& entity, float dt) {
    if (entity.is_missing<CollectsUserInput>()) return;
    CollectsUserInput& cui = entity.get<CollectsUserInput>();

    // Theres no players not in game menu state,
    const menu::State state = menu::State::Game;

    // TODO right now when you press two at the same time you move faster
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

    left = key_left;
    right = key_right;
    down = key_down;
    up = key_up;

    if (left > 0) cui.write(InputName::PlayerLeft, left);
    if (right > 0) cui.write(InputName::PlayerRight, right);
    if (up > 0) cui.write(InputName::PlayerForward, up);
    if (down > 0) cui.write(InputName::PlayerBack, down);

    bool pickup =
        KeyMap::is_event_once_DO_NOT_USE(state, InputName::PlayerPickup);
    if (pickup) cui.write(InputName::PlayerPickup, 1.f);

    bool handtruck_interact = KeyMap::is_event_once_DO_NOT_USE(
        state, InputName::PlayerHandTruckInteract);
    if (handtruck_interact) cui.write(InputName::PlayerHandTruckInteract, 1.f);

    bool rotate = KeyMap::is_event_once_DO_NOT_USE(
        state, InputName::PlayerRotateFurniture);
    if (rotate) cui.write(InputName::PlayerRotateFurniture, 1.f);

    float do_work = KeyMap::is_event(state, InputName::PlayerDoWork);
    if (do_work > 0) cui.write(InputName::PlayerDoWork, 1.f);

    // run the input on the local client
    system_manager::input_process_manager::process_input(
        entity, {cui.read(), dt, camAngle});

    // Actually save the inputs if there were any
    cui.publish(dt, camAngle);
}

void process_player_movement_input(Entity& entity, float dt,
                                   float cam_angle_deg, InputName input_name,
                                   float input_amount) {
    if (entity.is_missing<Transform>()) return;
    Transform& transform = entity.get<Transform>();

    if (entity.is_missing<HasBaseSpeed>()) return;
    const HasBaseSpeed& hasBaseSpeed = entity.get<HasBaseSpeed>();

    // Convert camera angle from degrees to radians
    float cam_angle_rad = util::deg2rad(cam_angle_deg + 90.f);

    // Calculate the movement direction based on the camera angle
    float cos_angle = -1.f * std::cos(cam_angle_rad);
    float sin_angle = std::sin(cam_angle_rad);

    auto new_position_x = transform.pos();
    auto new_position_z = transform.pos();

    // TODO :DESIGN: this should be separate functions or something because
    // at the moment the order really matters, but probably we want these
    // to be additive? (like handtruck full & vomit is 25% speed??)
    const auto getSpeedMultiplier = [&]() {
        if (entity.has<CanHoldHandTruck>()) {
            const CanHoldHandTruck& chht = entity.get<CanHoldHandTruck>();
            if (chht.is_holding()) {
                OptEntity hand_truck =
                    EntityHelper::getEntityForID(chht.hand_truck_id());
                if (hand_truck) {
                    const CanHoldFurniture& ht_chf =
                        hand_truck->get<CanHoldFurniture>();
                    if (ht_chf.is_holding_furniture()) {
                        return 0.5f;
                    }
                }
            }
        }

        OptEntity overlap =
            EntityHelper::getOverlappingEntityIfExists(entity, 0.75f);
        if (!overlap.has_value()) return 1.f;
        if (check_type(overlap.asE(), EntityType::Vomit)) return 0.5f;
        return 1.f;
    };

    const float amount =
        input_amount * hasBaseSpeed.speed() * dt * getSpeedMultiplier();

    // Implement the logic based on the input_name
    switch (input_name) {
        case InputName::PlayerForward:
            new_position_x.x += cos_angle * amount;
            new_position_z.z += sin_angle * amount;
            transform.update_face_direction(cam_angle_deg);
            break;
        case InputName::PlayerBack:
            new_position_x.x -= cos_angle * amount;
            new_position_z.z -= sin_angle * amount;
            transform.update_face_direction(cam_angle_deg + 180.f);
            break;
        case InputName::PlayerLeft:
            new_position_x.x += sin_angle * amount;
            new_position_z.z -= cos_angle * amount;
            transform.update_face_direction(cam_angle_deg + 90.f);
            break;
        case InputName::PlayerRight:
            new_position_x.x -= sin_angle * amount;
            new_position_z.z += cos_angle * amount;
            transform.update_face_direction(cam_angle_deg - 90.f);
            break;
        default:
            break;
    }

    person_update_given_new_pos(entity.id, transform, entity, dt,
                                new_position_x, new_position_z);
};

void work_furniture(Entity& player, float frame_dt) {
    const CanHighlightOthers& cho = player.get<CanHighlightOthers>();

    OptEntity match = EntityHelper::getClosestMatchingFurniture(
        player.get<Transform>(), cho.reach(), [](const Entity& furniture) {
            if (furniture.template is_missing<HasWork>()) return false;
            const HasWork& hasWork = furniture.template get<HasWork>();
            return hasWork.has_work();
        });

    if (!match) return;

    match->get<HasWork>().call(match.asE(), player, frame_dt);
}

void fishing_game(Entity& player, float frame_dt) {
    if (!GameState::get().is_game_like()) return;
    CanHoldItem& chi = player.get<CanHoldItem>();
    if (!chi.is_holding_item()) return;
    Item& item = chi.item();
    if (item.is_missing<HasFishingGame>()) return;
    if (item.get<HasFishingGame>().has_score()) return;
    item.get<HasFishingGame>().go(frame_dt);
}

namespace planning {
void rotate_furniture(const Entity& player) {
    // Cant rotate outside planning mode
    if (GameState::get().is_not(game::State::Planning)) return;

    const CanHighlightOthers& cho = player.get<CanHighlightOthers>();

    OptEntity match = EntityHelper::getClosestMatchingFurniture(
        player.get<Transform>(), cho.reach(), [](const Entity& furniture) {
            return furniture.template has<IsRotatable>();
        });

    if (!match) return;
    match->get<Transform>().rotate_facing_clockwise();
}

void drop_held_furniture(Entity& player) {
    CanHoldFurniture& ourCHF = player.get<CanHoldFurniture>();
    EntityID furn_id = ourCHF.furniture_id();
    OptEntity hf = EntityHelper::getEntityForID(furn_id);
    if (!hf) {
        log_info(" id:{} we'd like to drop but our hands are empty", player.id);
        return;
    }

    vec3 drop_location = player.get<Transform>().drop_location();

    // TODO :DUPE: this logic is also in rendering system
    // they should match so we dont have weirdness with ui not matching
    // the actual logic

    // We have to use the non cached version because the pickup location
    // is in the cache
    bool can_place =
        EntityHelper::isWalkableRawEntities(vec::to2(drop_location));

    // need to make sure it doesnt place ontop of another one
    // log_info("you cant place that here...");
    if (!can_place) {
        return;
    }

    // For items in the store, we should make sure you dont take them outside
    // somehow
    if (hf->has<IsStoreSpawned>()) {
        // TODO add a message or something to show you cant drop it
        if (!STORE_BUILDING.is_inside({drop_location.x, drop_location.z}))
            return;
    }

    hf->get<CanBeHeld>().set_is_being_held(false);
    Transform& hftrans = hf->get<Transform>();
    hftrans.update(drop_location);

    ourCHF.update(-1, vec3{});
    log_info("we {} dropped the furniture {} we were holding", player.id,
             hf->id);

    EntityHelper::invalidatePathCache();

    // TODO :PICKUP: i dont like that these are spread everywhere,
    network::Server::play_sound(player.get<Transform>().as2(),
                                strings::sounds::PLACE);

    {
        Transform& transform = player.get<Transform>();
        auto my_bounds = transform.bounds();
        auto their_bounds = hftrans.bounds();

        if (raylib::CheckCollisionBoxes(my_bounds, their_bounds)) {
            // player is inside dropped object
            transform.update(vec::to3(transform.tile_behind(0.15f)));
        }
    }
}

void handle_grab_or_drop(Entity& player) {
    log_info("Handle grab or drop, player is {}", player.type);
    const CanHighlightOthers& cho = player.get<CanHighlightOthers>();
    CanHoldFurniture& ourCHF = player.get<CanHoldFurniture>();

    if (ourCHF.is_holding_furniture()) {
        drop_held_furniture(player);
        return;
    } else {
        // TODO support finding things in the direction the player is
        // facing, instead of in a box around him

        OptEntity closest_furniture = EntityHelper::getClosestMatchingFurniture(
            player.get<Transform>(), cho.reach(), [](const Entity& f) {
                // right now walls inherit this from furniture
                // but eventually that should not be the case
                if (f.is_missing<CanBeHeld>()) return false;
                return f.get<CanBeHeld>().is_not_held();
            });
        // no match
        if (!closest_furniture) return;

        ourCHF.update(closest_furniture->id,
                      closest_furniture->get<Transform>().pos());
        OptEntity furniture =
            EntityHelper::getEntityForID(ourCHF.furniture_id());
        furniture->get<CanBeHeld>().set_is_being_held(true);

        // Note: we expect thatr since ^ set is held is true,
        // the previous position this furniture was at before you picked it up
        // should now be walkable but for some reason the preview doesnt turn
        // red
        EntityHelper::invalidatePathCache();

        // TODO :PICKUP: i dont like that these are spread everywhere,
        network::Server::play_sound(player.get<Transform>().as2(),
                                    strings::sounds::PICKUP);

        return;
    }
}

}  // namespace planning

namespace inround {
void handle_drop(Entity& player) {
    const CanHighlightOthers& cho = player.get<CanHighlightOthers>();

    const auto _place_special_item_onto_ground =
        [&]() -> tl::expected<bool, std::string> {
        Item& item = player.get<CanHoldItem>().item();

        // This is only allowed for special boys
        if (!check_type(item, EntityType::MopBuddy))
            return tl::unexpected("boy was not special");

        // Just drop him wherever we are
        item.get<IsItem>().set_held_by(EntityType::Unknown, -1);
        player.get<CanHoldItem>().update(nullptr, -1);
        return true;
    };

    /*
    // This is for example putting a pill into a bag you are holding
    // taking an item and placing it into the container in your hand
    const auto _merge_item_from_furniture_into_hand_item =
        [&player]() -> tl::expected<bool, std::string> {
        const CanHighlightOthers& cho = player.get<CanHighlightOthers>();

        std::shared_ptr<Item> item = player.get<CanHoldItem>().item();

        if (item->is_missing<CanHoldItem>()) {
            return tl::unexpected(
                "trying to merge from furniture, but item can not hold "
                "things");
        }

        CanHoldItem& item_chi = item->get<CanHoldItem>();

        // T.ODO replace !empty() with full()
        // our item is already full
        if (!item_chi.empty()) {
            return tl::unexpected(
                "trying to merge from furniture, but item was not "
                "empty");
        }

        OptEntity closest_furniture = EntityHelper::getClosestMatchingFurniture(
            player.get<Transform>(), cho.reach(), [](const Entity& f) {
                if (f.is_missing<CanHoldItem>()) return false;
                return f.get<CanHoldItem>().is_holding_item();
            });

        if (!closest_furniture) {
            return tl::unexpected(
                "trying to merge from furniture, but didnt find "
                "anything "
                "holding something");
        }

        std::shared_ptr<Item> item_to_merge =
            closest_furniture->get<CanHoldItem>().item();

        // T.ODO this check should be probably be int he furniture check
        if (!item_chi.can_hold(*item_to_merge, RespectFilter::All)) {
            return tl::unexpected(
                "trying to merge from furniture, but we cant hold"
                "that kind of item ");
        }

        // T.ODO probably need to have heldby updated here

        // Our item takes ownership of the item
        item_chi.update(item_to_merge, item->id);
        // furniture lets go
        closest_furniture->get<CanHoldItem>().update(nullptr, -1);
        return true;
    };
    */

    // This is like placing an item onto a plate and immediately picking
    // it up it should only apply when the merged item isnt valid in
    // that spot anymore ie only empty bags go in bagbox so a bag
    // containg something shouldnt go back
    //
    /*
     * T.ODO since this exists as a way to handle containers
     * maybe short circuit earlier by doing?
        static bool is_an_item_container(Entity* e) {
            return e->has_any<                //
                IsItemContainer<Pill>,        //
                IsItemContainer<PillBottle>,  //
                IsItemContainer<Bag>          //
                >();
        }
     * */
    /*
    const auto _merge_item_from_furniture_around_hand_item =
        [&player]() -> tl::expected<bool, std::string> {
        const CanHighlightOthers& cho = player.get<CanHighlightOthers>();
        CanHoldItem& playerCHI = player.get<CanHoldItem>();

        if (!playerCHI.is_holding_item()) {
            return tl::unexpected(
                "trying to merge from furniture around hand, but "
                "player wasnt "
                "holding anything");
        }

        OptEntity closest_furniture = EntityHelper::getClosestMatchingFurniture(
            player.get<Transform>(), cho.reach(),
            [&playerCHI](const Entity& f) {
                if (f.is_missing<CanHoldItem>()) return false;
                const auto& chi = f.get<CanHoldItem>();
                if (chi.empty()) return false;
                // T.ODO we are using const_item() but this doesnt
                // enforce const and is just for us to understand
                const std::shared_ptr<Entity> item = chi.const_item();

                // Does the item this furniture holds have the
                // ability to hold things
                if (item->is_missing<CanHoldItem>()) return false;

                // Can it hold the thing we are holding?
                const CanHoldItem& item_chi = item->get<CanHoldItem>();
                if (!item_chi.can_hold(*playerCHI.item(), RespectFilter::All))
                    return false;

                return true;
            });

        if (!closest_furniture) {
            return tl::unexpected(
                "trying to merge from furniture around hand, but didnt "
                "find "
                "anything than can hold what we are holding");
        }

        std::shared_ptr<Item> item_to_merge =
            closest_furniture->get<CanHoldItem>().item();

        CanHoldItem& merge_chi = item_to_merge->get<CanHoldItem>();

        // T.ODO probably need to have heldby updated here
        // Their item takes ownership of the item
        merge_chi.update(playerCHI.item(), player.id);

        // remove the link to the one we were already holding
        player.get<CanHoldItem>().update(nullptr, -1);
        // add the link to the new one
        player.get<CanHoldItem>().update(item_to_merge, player.id);
        // furniture can let go
        closest_furniture->get<CanHoldItem>().update(nullptr, -1);
        return true;
    };
    */

    const auto _place_item_onto_furniture =
        [&]() -> tl::expected<bool, std::string> {
        OptEntity closest_furniture = EntityHelper::getClosestMatchingFurniture(
            player.get<Transform>(), cho.reach(),
            [&player](const Entity& f) -> bool {
                // This cant hold anything
                if (f.is_missing<CanHoldItem>()) return false;

                // we dont currently need to be able to hand directly to
                // customers so for now just disable for them
                if (check_type(f, EntityType::Customer)) {
                    return false;
                }

                const CanHoldItem& furnCanHold = f.get<CanHoldItem>();
                const CanHoldItem& playerCanHold = player.get<CanHoldItem>();
                if (!playerCanHold.is_holding_item()) return false;
                const Item& item = playerCanHold.item();

                // Handle item containers
                if (f.has<IsItemContainer>()) {
                    const IsItemContainer& itemContainer =
                        f.get<IsItemContainer>();
                    // note: right now item container only validates
                    // EntityType
                    bool matches_item_type =
                        itemContainer.is_matching_item(item);
                    if (!matches_item_type) return false;
                }
                // if you are not an item container
                // then you have to be empty for us to place into
                else {
                    // This check has to go after the item containers,
                    // for putting back into supply to work
                    if (!furnCanHold.empty()) return false;
                }

                // Can it hold the item we are trying to drop
                return furnCanHold.can_hold(
                    item,
                    // dont worry about suggested filters
                    // because we are a player and force drop
                    RespectFilter::ReqOnly);
            });

        // no matching furniture
        if (!closest_furniture)
            return tl::unexpected("place_onto: no matching furniture");

        const Transform& furnT = closest_furniture->get<Transform>();
        CanHoldItem& furnCHI = closest_furniture->get<CanHoldItem>();

        Item& item = player.get<CanHoldItem>().item();
        item.get<Transform>().update(furnT.snap_position());

        if (closest_furniture->has<IsItemContainer>() &&
            closest_furniture->get<IsItemContainer>().is_matching_item(item)) {
            // So in this case,
            // if theres already an item there, then we just have to
            // delete the one we are holding.
            //
            // if theres nothing there, then we do the normal drop logic
            if (furnCHI.is_holding_item()) {
                player.get<CanHoldItem>().item().cleanup = true;
                player.get<CanHoldItem>().update(nullptr, -1);
                return true;
            }
        }

        furnCHI.update(EntityHelper::getEntityAsSharedPtr(item),
                       closest_furniture->id);
        player.get<CanHoldItem>().update(nullptr, -1);
        return true;
    };

    typedef std::function<tl::expected<bool, std::string>()> MergeFunc;
    // NOTE: ORDER MATTERS HERE
    std::vector<MergeFunc> fns{
        _place_special_item_onto_ground,
        // _merge_item_from_furniture_into_hand_item,
        // _merge_item_from_furniture_around_hand_item,
        // _merge_item_in_hand_into_furniture_item,
        _place_item_onto_furniture,
    };

    bool item_merged = false;
    for (const auto& fn : fns) {
        auto was_merged = fn();
        item_merged |= was_merged.value_or(false);
        if (was_merged) break;
        log_info("{}", was_merged.error());
    }

    if (item_merged) {
        // TODO :PICKUP: i dont like that these are spread everywhere,
        network::Server::play_sound(player.get<Transform>().as2(),
                                    strings::sounds::PLACE);
    }
}

void handle_grab(Entity& player) {
    const auto _try_to_pickup_item_from_furniture = [&player]() {
        const CanHighlightOthers& cho = player.get<CanHighlightOthers>();
        const Transform& playerT = player.get<Transform>();

        OptEntity closest_furniture = EntityHelper::getClosestMatchingFurniture(
            playerT, cho.reach(), [&player](const Entity& furn) -> bool {
                // You should not be able to take from
                // other player / customer
                if (check_type(furn, EntityType::Customer)) return false;
                if (check_type(furn, EntityType::Player)) return false;
                if (check_type(furn, EntityType::RemotePlayer)) return false;

                // This furniture can never hold anything so no
                // match
                if (furn.is_missing<CanHoldItem>()) return false;
                // its not holding something
                if (furn.get<CanHoldItem>().empty()) return false;

                const Item& item = furn.get<CanHoldItem>().const_item();
                // Can we hold the item it has?
                return player.get<CanHoldItem>().can_hold(item,
                                                          RespectFilter::All);
            });

        // No matching furniture that also can hold item
        if (!closest_furniture) return false;

        // we found a match, grab the item from it
        CanHoldItem& furnCanHold = closest_furniture->get<CanHoldItem>();
        Item& item = furnCanHold.item();

        // log_info("Found {} to pick up from {}",
        // item->name(), closest_furniture->name());

        CanHoldItem& playerCHI = player.get<CanHoldItem>();
        playerCHI.update(EntityHelper::getEntityAsSharedPtr(item), player.id);
        item.get<Transform>().update(player.get<Transform>().snap_position());
        furnCanHold.update(nullptr, -1);

        // In certain cases, we need to reset the progress when you pick up an
        // item. im not sure exactly when this needs to be, but im sure over
        // time we will see a pattern.
        //
        // Reset work progress bar if you remove the drink from it
        if (closest_furniture->has<HasWork>() && item.has<IsDrink>()) {
            closest_furniture->get<HasWork>().reset_pct();
        }

        return true;
    };

    bool picked_up_item = _try_to_pickup_item_from_furniture();
    if (picked_up_item) {
        // TODO :PICKUP: i dont like that these are spread everywhere,
        network::Server::play_sound(player.get<Transform>().as2(),
                                    strings::sounds::PICKUP);
        return;
    }

    // Handles the non-furniture grabbing case
    const CanHighlightOthers& cho = player.get<CanHighlightOthers>();

    auto pos = player.get<Transform>().as2();

    OptEntity closest_item =
        EntityQuery()
            .whereHasComponentAndLambda<IsItem>(
                [](const IsItem& isitem) { return !isitem.is_held(); })
            .whereInRange(pos, TILESIZE * cho.reach())
            .orderByDist(pos)
            .gen_first();

    // nothing found
    if (!closest_item) return;

    player.get<CanHoldItem>().update(
        EntityHelper::getEntityAsSharedPtr(closest_item), player.id);

    // TODO :PICKUP: i dont like that these are spread everywhere,
    network::Server::play_sound(player.get<Transform>().as2(),
                                strings::sounds::PICKUP);
}

bool handle_drop_hand_truck(Entity& player) {
    Transform& transform = player.get<Transform>();
    CanHoldHandTruck& chht = player.get<CanHoldHandTruck>();
    OptEntity hand_truck = EntityHelper::getEntityForID(chht.hand_truck_id());
    if (!hand_truck) {
        log_error(
            "We are supposed to be holding a handtruck but the id is bad "
            "{}",
            chht.hand_truck_id());
        return true;
    }

    CanHoldFurniture& ht_chf = hand_truck->get<CanHoldFurniture>();

    if (!ht_chf.is_holding_furniture()) {
        log_info("chf is not holding anything, Handle drop hand truck");
        ///////////////////
        // Drop the hand truck
        ///////////////////

        vec3 drop_location = player.get<Transform>().drop_location();

        // We have to use the non cached version because the pickup location
        // is in the cache
        bool can_place =
            EntityHelper::isWalkableRawEntities(vec::to2(drop_location));

        // need to make sure it doesnt place ontop of another one
        // log_info("you cant place that here...");
        if (!can_place) {
            return true;
        }

        // hand_truck->get<CanBeHeld>().set_is_being_held(false);
        Transform& hftrans = hand_truck->get<Transform>();
        hftrans.update(drop_location);

        chht.update(-1, vec3{});
        log_info("we {} dropped the handtruck {} we were holding", player.id,
                 hand_truck->id);

        EntityHelper::invalidatePathCache();

        // TODO :PICKUP: i dont like that these are spread everywhere,
        network::Server::play_sound(player.get<Transform>().as2(),
                                    strings::sounds::PLACE);

        {
            auto my_bounds = transform.bounds();
            auto their_bounds = hftrans.bounds();

            if (raylib::CheckCollisionBoxes(my_bounds, their_bounds)) {
                // player is inside dropped object
                transform.update(vec::to3(transform.tile_behind(0.15f)));
            }
        }
        return false;
    }

    log_info("pickup/drop the furniture");
    planning::handle_grab_or_drop(hand_truck.asE());
    //
    return false;
}

bool handle_hand_truck(Entity& player) {
    log_info("Handle hand truck");
    const CanHighlightOthers& cho = player.get<CanHighlightOthers>();
    const Transform& transform = player.get<Transform>();
    CanHoldHandTruck& chht = player.get<CanHoldHandTruck>();

    const auto handle_grab_hand_truck = [&]() -> bool {
        log_info("Handle grab hand truck");

        // TODO need a way to ignore ones that are held by someone else
        OptEntity closest_handtruck =
            EntityQuery()
                .whereInRange(transform.as2(), cho.reach())
                .whereType(EntityType::HandTruck)
                .gen_first();
        // no match
        if (!closest_handtruck) return false;

        chht.update(closest_handtruck->id,
                    closest_handtruck->get<Transform>().pos());

        OptEntity hand_truck =
            EntityHelper::getEntityForID(chht.hand_truck_id());
        hand_truck->get<CanBeHeld_HT>().set_is_being_held(true);

        // Note: we expect thatr since ^ set is held is true,
        // the previous position this furniture was at before you picked it up
        // should now be walkable but for some reason the preview doesnt turn
        // red
        EntityHelper::invalidatePathCache();

        // TODO :PICKUP: i dont like that these are spread everywhere,
        network::Server::play_sound(player.get<Transform>().as2(),
                                    strings::sounds::PICKUP);
        return true;
    };

    // We dont have to check CanHoldFurniture because you cant hold anything
    // without a handtruck during inround mode
    return (chht.empty() ? handle_grab_hand_truck()
                         : handle_drop_hand_truck(player));
}

void handle_grab_or_drop(Entity& player) {
    // If you are holding the handtruck,
    // then dont allow picking up anything else
    CanHoldHandTruck& chht = player.get<CanHoldHandTruck>();
    if (chht.is_holding()) {
        OptEntity hand_truck =
            EntityHelper::getEntityForID(chht.hand_truck_id());
        planning::handle_grab_or_drop(hand_truck.asE());
        return;
    }

    // Do we already have something in our hands?
    // We must be trying to drop it
    player.get<CanHoldItem>().empty() ? handle_grab(player)
                                      : handle_drop(player);
}

}  // namespace inround

void process_input(Entity& entity, const UserInput& input) {
    const auto _proc_single_input_name =
        [](Entity& entity, const InputName& input_name, float input_amount,
           float frame_dt, float cam_angle) {
            switch (input_name) {
                case InputName::PlayerLeft:
                case InputName::PlayerRight:
                case InputName::PlayerForward:
                case InputName::PlayerBack:
                    return process_player_movement_input(
                        entity, frame_dt, cam_angle, input_name, input_amount);
                default:
                    break;
            }

            // Because of predictive input, we run this _proc_single as the
            // client and as the server
            //
            // For the client we only care about player movement, so if we are
            // not the server then just skip the rest

            // TODO we would like to disable this so placing preview works
            // however it breaks all pickup/drop on non host client...
            // if (!is_server()) return;
            //
            // ^^^ This breaks clientside held furniture, which means the
            // preview wont work with this

            switch (input_name) {
                case InputName::PlayerRotateFurniture:
                    planning::rotate_furniture(entity);
                    break;
                case InputName::PlayerHandTruckInteract:
                    if (GameState::get().is_game_like()) {
                        inround::handle_hand_truck(entity);
                    }
                    break;
                case InputName::PlayerPickup:
                    // grab_or_drop(entity);
                    {
                        if (GameState::get().is_game_like()) {
                            inround::handle_grab_or_drop(entity);
                        } else if (GameState::get().is(game::State::Planning)) {
                            // TODO the is_game_like check above kills this
                            // entire case
                            planning::handle_grab_or_drop(entity);
                        } else {
                            // probably want to handle messing around in the
                            // lobby?
                        }
                    }
                    break;
                case InputName::PlayerDoWork: {
                    work_furniture(entity, frame_dt);
                    fishing_game(entity, frame_dt);
                } break;
                default:
                    break;
            }
        };

    const InputSet input_set = std::get<0>(input);
    const float frame_dt = std::get<1>(input);
    const float cam_angle = std::get<2>(input);

    size_t i = 0;
    while (i < magic_enum::enum_count<InputName>()) {
        auto input_name = magic_enum::enum_value<InputName>(i);
        float input_amount = input_set[i];
        if (input_amount > 0.f) {
            _proc_single_input_name(entity, input_name, input_amount, frame_dt,
                                    cam_angle);
        }
        i++;
    }
}
}  // namespace input_process_manager
}  // namespace system_manager
