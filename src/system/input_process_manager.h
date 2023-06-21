#pragma once

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

    // TODO what is this for
    // this->get<Transform>().face_direction =
    // Transform::FrontFaceDirection::BACK &
    // Transform::FrontFaceDirection::LEFT;

    // TODO this should be a component
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
            if (!is_collidable(entity)) {
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
            return EntityHelper::ForEachFlow::None;
        });

        // TODO add debug mode that turns on noclip

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

inline void collect_user_input(std::shared_ptr<Entity> entity, float dt) {
    if (entity->is_missing<CollectsUserInput>()) return;
    CollectsUserInput& cui = entity->get<CollectsUserInput>();

    // Theres no players not in game menu state,
    const menu::State state = menu::State::Game;

    float left = KeyMap::is_event(state, InputName::PlayerLeft);
    float right = KeyMap::is_event(state, InputName::PlayerRight);
    if (left > 0)
        cui.inputs.push_back({state, InputName::PlayerLeft, left, dt});
    if (right > 0)
        cui.inputs.push_back({state, InputName::PlayerRight, right, dt});

    float up = KeyMap::is_event(state, InputName::PlayerForward);
    float down = KeyMap::is_event(state, InputName::PlayerBack);
    if (up > 0) cui.inputs.push_back({state, InputName::PlayerForward, up, dt});
    if (down > 0)
        cui.inputs.push_back({state, InputName::PlayerBack, down, dt});

    bool pickup =
        KeyMap::is_event_once_DO_NOT_USE(state, InputName::PlayerPickup);
    if (pickup) {
        cui.inputs.push_back({state, InputName::PlayerPickup, 1.f, dt});
    }

    bool rotate = KeyMap::is_event_once_DO_NOT_USE(
        state, InputName::PlayerRotateFurniture);
    if (rotate) {
        cui.inputs.push_back(
            {state, InputName::PlayerRotateFurniture, 1.f, dt});
    }

    float do_work = KeyMap::is_event(state, InputName::PlayerDoWork);
    if (do_work > 0) {
        cui.inputs.push_back({state, InputName::PlayerDoWork, 1.f, dt});
    }
}

inline void process_player_movement_input(std::shared_ptr<Entity> entity,
                                          float dt, InputName input_name,
                                          float input_amount) {
    if (entity->is_missing<Transform>()) return;
    Transform& transform = entity->get<Transform>();
    std::shared_ptr<Entity> player = dynamic_pointer_cast<Entity>(entity);

    if (entity->is_missing<HasBaseSpeed>()) return;
    HasBaseSpeed& hasBaseSpeed = entity->get<HasBaseSpeed>();

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
    transform.update(transform.raw());
};

void rotate_furniture(const std::shared_ptr<Entity> player) {
    TRACY_ZONE_SCOPED;
    // Cant rotate outside planning mode
    if (GameState::get().is_not(game::State::Planning)) return;

    // TODO need to figure out if this should be separate from highlighting
    CanHighlightOthers& cho = player->get<CanHighlightOthers>();

    std::shared_ptr<Furniture> match =
        // TODO have this just take a transform
        EntityHelper::getClosestMatchingEntity<Furniture>(
            player->get<Transform>().as2(), cho.reach(), [](auto&& furniture) {
                return furniture->template has<IsRotatable>();
            });

    if (!match) return;
    match->get<Transform>().rotate_facing_clockwise();
}

void work_furniture(const std::shared_ptr<Entity> player, float frame_dt) {
    TRACY_ZONE_SCOPED;
    // Cant do work during planning
    if (GameState::get().is(game::State::Planning)) return;

    // TODO need to figure out if this should be separate from highlighting
    CanHighlightOthers& cho = player->get<CanHighlightOthers>();

    std::shared_ptr<Furniture> match =
        EntityHelper::getClosestMatchingEntity<Furniture>(
            player->get<Transform>().as2(), cho.reach(),
            [](std::shared_ptr<Furniture> furniture) {
                if (furniture->is_missing<HasWork>()) return false;
                HasWork& hasWork = furniture->get<HasWork>();
                return hasWork.has_work();
            });

    if (!match) return;

    HasWork& hasWork = match->get<HasWork>();
    if (hasWork.do_work) hasWork.do_work(hasWork, player, frame_dt);
}

void handle_in_planning_grab_or_drop(const std::shared_ptr<Entity>& player) {
    TRACY_ZONE_SCOPED;
    // TODO: Need to delete any held items when switching from game ->
    // planning

    // TODO need to auto drop when "in_planning" changes

    // TODO add transform request / has checks

    // TODO add has<> checks here
    // TODO need to figure out if this should be separate from highlighting
    CanHighlightOthers& cho = player->get<CanHighlightOthers>();
    CanHoldFurniture& ourCHF = player->get<CanHoldFurniture>();

    if (ourCHF.is_holding_furniture()) {
        const auto _drop_furniture = [&]() {
            // TODO need to make sure it doesnt place ontop of another
            // one
            auto hf = ourCHF.furniture();

            hf->get<CanBeHeld>().update(false);
            hf->get<Transform>().update(vec::snap(vec::to3(
                player->get<Transform>().tile_infront_given_player(1))));

            ourCHF.update(nullptr);
        };
        _drop_furniture();
        return;
    } else {
        // TODO support finding things in the direction the player is
        // facing, instead of in a box around him
        std::shared_ptr<Furniture> closest_furniture =
            EntityHelper::getMatchingEntityInFront<Furniture>(
                player->get<Transform>().as2(), cho.reach(),
                player->get<Transform>().face_direction(),
                [](std::shared_ptr<Furniture> f) {
                    // TODO right now walls inherit this from furniture but
                    // eventually that should not be the case
                    return f->get<CanBeHeld>().is_not_held();
                });
        ourCHF.update(closest_furniture);
        if (ourCHF.is_holding_furniture()) {
            ourCHF.furniture()->get<CanBeHeld>().update(true);
        }
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

void handle_in_game_grab_or_drop(const std::shared_ptr<Entity>& player) {
    TRACY_ZONE_SCOPED;
    // TODO Need to auto drop any held furniture

    // TODO add has<> checks
    // TODO need to figure out if this should be separate from highlighting
    CanHighlightOthers& cho = player->get<CanHighlightOthers>();

    if (player->is_missing<CanHoldItem>()) {
        log_warn("Player trying to grab/drop but missing CanHoldItem");
        return;
    }

    CanHoldItem& playerCHI = player->get<CanHoldItem>();

    // Do we already have something in our hands?
    // We must be trying to drop it
    // TODO fix
    if (playerCHI.item()) {
        const auto _merge_item_from_furniture_into_hand = [&]() {
            TRACY_ZONE(tracy_merge_item_from_furniture);
            // our item cant hold anything or is already full
            if (!player->get<CanHoldItem>().item()->empty()) {
                return false;
            }

            std::shared_ptr<Furniture> closest_furniture =
                EntityHelper::getMatchingEntityInFront<Furniture>(
                    player->get<Transform>().as2(), cho.reach(),
                    player->get<Transform>().face_direction(),
                    [](std::shared_ptr<Furniture> f) {
                        return f->get<CanHoldItem>().is_holding_item();
                    });

            if (!closest_furniture) {
                return false;
            }

            auto item_to_merge = closest_furniture->get<CanHoldItem>().item();
            bool eat_was_successful =
                player->get<CanHoldItem>().item()->eat(item_to_merge);
            if (eat_was_successful)
                closest_furniture->get<CanHoldItem>().item() = nullptr;
            return eat_was_successful;
        };

        const auto _merge_item_in_hand_into_furniture_item = [&]() {
            TRACY_ZONE(tracy_merge_item_in_hand_into_furniture);
            std::shared_ptr<Furniture> closest_furniture =
                EntityHelper::getMatchingEntityInFront<Furniture>(
                    player->get<Transform>().as2(), cho.reach(),
                    player->get<Transform>().face_direction(),
                    [&](std::shared_ptr<Furniture> f) {
                        return
                            // is there something there to merge into?
                            f->get<CanHoldItem>().is_holding_item() &&
                            // can that thing hold the item we are holding?
                            f->get<CanHoldItem>().item()->can_eat(
                                player->get<CanHoldItem>().item());
                    });
            if (!closest_furniture) {
                return false;
            }

            // TODO need to handle the case where the merged item is not a
            // valid thing the furniture can hold.
            //
            // This happens for example when you merge into a supply cache.
            // Because the supply can only hold the container and not a
            // filled one... In this case we should either:
            // - block the merge
            // - place the merged item into the player's hand

            bool eat_was_successful =
                closest_furniture->get<CanHoldItem>().item()->eat(
                    player->get<CanHoldItem>().item());
            if (!eat_was_successful) return false;
            // TODO we need a let_go_of_item() to handle this kind of
            // transfer because it might get complicated and we might end up
            // with two things owning this could maybe be solved by
            // enforcing uniqueptr
            player->get<CanHoldItem>().item() = nullptr;
            return true;
        };

        const auto can_place_item_into = [](std::shared_ptr<Entity> entity,
                                            std::shared_ptr<Item> item) {
            if (entity->is_missing<CanHoldItem>()) return false;
            CanHoldItem& furnCanHold = entity->get<CanHoldItem>();

            const auto item_container_is_matching_item =
                []<typename I>(std::shared_ptr<Entity> entity,
                               std::shared_ptr<I> item = nullptr) {
                    if (!item) return false;
                    if (entity->has<IsItemContainer<I>>()) return false;
                    IsItemContainer<I>& itemContainer =
                        entity->get<IsItemContainer<I>>();
                    return itemContainer.is_matching_item(item);
                };

            // Handle item containers
            bool matches_bag = item_container_is_matching_item(
                entity, dynamic_pointer_cast<Bag>(item));
            if (matches_bag) return true;

            bool matches_pill_bottle = item_container_is_matching_item(
                entity, dynamic_pointer_cast<PillBottle>(item));
            if (matches_pill_bottle) return true;

            // If we are empty and can hold we good..
            return furnCanHold.empty();
        };

        const auto _place_item_onto_furniture = [&]() {
            TRACY_ZONE(tracy_place_item_onto_furniture);
            std::shared_ptr<Furniture> closest_furniture =
                EntityHelper::getMatchingEntityInFront<Furniture>(
                    player->get<Transform>().as2(), cho.reach(),
                    player->get<Transform>().face_direction(),
                    [player,
                     can_place_item_into](std::shared_ptr<Furniture> f) {
                        return can_place_item_into(
                            f, player->get<CanHoldItem>().item());
                    });
            if (!closest_furniture) {
                return false;
            }

            auto item = player->get<CanHoldItem>().item();
            item->update_position(
                closest_furniture->get<Transform>().snap_position());

            closest_furniture->get<CanHoldItem>().item() = item;
            closest_furniture->get<CanHoldItem>().item()->held_by =
                Item::HeldBy::FURNITURE;

            player->get<CanHoldItem>().item() = nullptr;
            return true;
        };

        // TODO could be solved with tl::expected i think
        bool item_merged = _merge_item_from_furniture_into_hand();
        if (item_merged) return;

        item_merged = _merge_item_in_hand_into_furniture_item();
        if (item_merged) return;

        [[maybe_unused]] bool item_placed = _place_item_onto_furniture();

        return;

    } else {
        const auto _try_to_pickup_item_from_furniture = [&]() {
            Transform& playerT = player->get<Transform>();

            std::shared_ptr<Furniture> closest_furniture =
                EntityHelper::getClosestMatchingFurniture(
                    playerT, cho.reach(), [](std::shared_ptr<Furniture> furn) {
                        if (furn->is_missing<CanHoldItem>()) return false;
                        return (furn->get<CanHoldItem>().item() != nullptr);
                    });

            if (!closest_furniture) {
                // No matching furniture that also can hold item
                return;
            }

            CanHoldItem& furnCanHold = closest_furniture->get<CanHoldItem>();
            std::shared_ptr<Item> item = furnCanHold.item();

            CanHoldItem& playerCHI = player->get<CanHoldItem>();
            playerCHI.item() = item;
            playerCHI.item()->held_by = Item::HeldBy::PLAYER;

            furnCanHold.item() = nullptr;
        };

        _try_to_pickup_item_from_furniture();

        if (player->get<CanHoldItem>().is_holding_item()) return;

        // Handles the non-furniture grabbing case
        std::shared_ptr<Item> closest_item =
            ItemHelper::getClosestMatchingItem<Item>(
                player->get<Transform>().as2(), TILESIZE * cho.reach());
        player->get<CanHoldItem>().item() = closest_item;
        // TODO fix
        if (player->get<CanHoldItem>().item() != nullptr) {
            player->get<CanHoldItem>().item()->held_by = Item::HeldBy::PLAYER;
        }
        return;
    }
}

inline void process_input(const std::shared_ptr<Entity> entity,
                          const UserInput& input) {
    const auto menu_state = std::get<0>(input);
    if (menu_state != menu::State::Game) return;

    const InputName input_name = std::get<1>(input);
    const float input_amount = std::get<2>(input);
    const float frame_dt = std::get<3>(input);

    switch (input_name) {
        case InputName::PlayerLeft:
        case InputName::PlayerRight:
        case InputName::PlayerForward:
        case InputName::PlayerBack:
            return process_player_movement_input(entity, frame_dt, input_name,
                                                 input_amount);
        default:
            break;
    }

    // The inputs below here only trigger when the input amount was enough
    bool was_pressed = input_amount >= 0.5;
    if (!was_pressed) {
        return;
    }

    // TODO eventually clean this up
    std::shared_ptr<Entity> player = dynamic_pointer_cast<Entity>(entity);
    if (!player) return;

    switch (input_name) {
        case InputName::PlayerRotateFurniture:
            rotate_furniture(player);
            break;
        case InputName::PlayerPickup:
            // grab_or_drop(player);
            {
                if (GameState::s_in_round()) {
                    handle_in_game_grab_or_drop(player);
                } else if (GameState::get().is(game::State::Planning)) {
                    handle_in_planning_grab_or_drop(player);
                } else {
                    // TODO we probably want to handle messing around in the
                    // lobby
                }
            }
            break;
        case InputName::PlayerDoWork:
            work_furniture(player, frame_dt);
        default:
            break;
    }
}
}  // namespace input_process_manager
}  // namespace system_manager
