
#pragma once

#include "../base_player.h"
#include "../components/can_be_ghost_player.h"
#include "../components/can_highlight_others.h"
#include "../components/can_hold_furniture.h"
#include "../components/can_perform_job.h"
#include "../components/collects_user_input.h"
#include "../components/custom_item_position.h"
#include "../components/is_snappable.h"
#include "../components/responds_to_user_input.h"
#include "../components/transform.h"
#include "../customer.h"
#include "../entity.h"
#include "../entityhelper.h"
#include "../furniture.h"
#include "../furniture/conveyer.h"
#include "../furniture/register.h"
#include "job_system.h"
#include "rendering_system.h"

namespace system_manager {

inline void transform_snapper(std::shared_ptr<Entity> entity, float) {
    if (!entity->has<Transform>()) return;
    Transform& transform = entity->get<Transform>();

    if (entity->has<IsSnappable>()) {
        transform.position = transform.snap_position();
    } else {
        transform.position = transform.raw_position;
    }
}

inline void update_held_furniture_position(std::shared_ptr<Entity> entity,
                                           float) {
    if (!entity->has<CanHoldFurniture>()) return;
    CanHoldFurniture& can_hold_furniture = entity->get<CanHoldFurniture>();

    if (!entity->has<Transform>()) return;
    const Transform& transform = entity->get<Transform>();

    // TODO if cannot be placed in this spot make it obvious to the user

    if (can_hold_furniture.empty()) return;

    auto new_pos = transform.position;
    if (transform.face_direction & Transform::FrontFaceDirection::FORWARD) {
        new_pos.z += TILESIZE;
    }
    if (transform.face_direction & Transform::FrontFaceDirection::RIGHT) {
        new_pos.x += TILESIZE;
    }
    if (transform.face_direction & Transform::FrontFaceDirection::BACK) {
        new_pos.z -= TILESIZE;
    }
    if (transform.face_direction & Transform::FrontFaceDirection::LEFT) {
        new_pos.x -= TILESIZE;
    }

    Transform& furn_transform =
        can_hold_furniture.furniture()->get<Transform>();
    furn_transform.update(new_pos);
}

inline void update_held_item_position(std::shared_ptr<Entity> entity, float) {
    if (!entity->has<CanHoldItem>()) return;
    const CanHoldItem& can_hold_item = entity->get<CanHoldItem>();
    if (can_hold_item.empty()) return;

    if (!entity->has<Transform>()) return;
    const Transform& transform = entity->get<Transform>();

    vec3 new_pos = transform.position;

    // TODO disabling custom positions for now until i can figure out the seg
    // fault
    if (false && entity->has<CustomHeldItemPosition>()) {
        CustomHeldItemPosition& custom_item_position =
            entity->get<CustomHeldItemPosition>();

        switch (custom_item_position.positioner) {
            case CustomHeldItemPosition::Positioner::Default:
                new_pos.y += TILESIZE / 4;
                break;
            case CustomHeldItemPosition::Positioner::Table:
                new_pos.y += TILESIZE / 2;
                break;
            case CustomHeldItemPosition::Positioner::Conveyer:
                auto conveyer = dynamic_pointer_cast<Conveyer>(entity);
                if (!conveyer) {
                    log_warn("Using custom held conveyer but not a conveyer");
                    break;
                }
                if (transform.face_direction &
                    Transform::FrontFaceDirection::FORWARD) {
                    new_pos.z += TILESIZE * conveyer->relative_item_pos;
                }
                if (transform.face_direction &
                    Transform::FrontFaceDirection::RIGHT) {
                    new_pos.x += TILESIZE * conveyer->relative_item_pos;
                }
                if (transform.face_direction &
                    Transform::FrontFaceDirection::BACK) {
                    new_pos.z -= TILESIZE * conveyer->relative_item_pos;
                }
                if (transform.face_direction &
                    Transform::FrontFaceDirection::LEFT) {
                    new_pos.x -= TILESIZE * conveyer->relative_item_pos;
                }
                new_pos.y += TILESIZE / 4;
                break;
        }
        can_hold_item.item()->update_position(new_pos);
        return;
    }

    // Default
    if (transform.face_direction & Transform::FrontFaceDirection::FORWARD) {
        new_pos.z += TILESIZE;
    }
    if (transform.face_direction & Transform::FrontFaceDirection::RIGHT) {
        new_pos.x += TILESIZE;
    }
    if (transform.face_direction & Transform::FrontFaceDirection::BACK) {
        new_pos.z -= TILESIZE;
    }
    if (transform.face_direction & Transform::FrontFaceDirection::LEFT) {
        new_pos.x -= TILESIZE;
    }
    can_hold_item.item()->update_position(new_pos);
}

inline void reset_highlighted(std::shared_ptr<Entity> entity, float) {
    if (!entity->has<CanBeHighlighted>()) return;
    CanBeHighlighted& cbh = entity->get<CanBeHighlighted>();
    cbh.is_highlighted = false;
}

inline void highlight_facing_furniture(std::shared_ptr<Entity> entity, float) {
    if (!entity->has<CanHighlightOthers>()) return;
    CanHighlightOthers& cho = entity->get<CanHighlightOthers>();

    // TODO explicity commenting this out so that we get an error
    // if (!entity->has<Transform>()) return;
    Transform& transform = entity->get<Transform>();

    // TODO this is impossible to read, what can we do to fix this while
    // keeping it configurable
    auto match = EntityHelper::getMatchingEntityInFront<Furniture>(
        // TODO add a player reach component
        transform.as2(), cho.reach(), transform.face_direction,
        [](std::shared_ptr<Furniture>) { return true; });
    if (!match) return;
    if (!match->has<CanBeHighlighted>()) return;
    match->get<CanBeHighlighted>().is_highlighted = true;
}

// TODO We need like a temporary storage for this
inline void move_entity_based_on_push_force(std::shared_ptr<Entity> entity,
                                            float, vec3& new_pos_x,
                                            vec3& new_pos_z) {
    CanBePushed& cbp = entity->get<CanBePushed>();

    new_pos_x.x += cbp.pushed_force.x;
    cbp.pushed_force.x = 0.0f;

    new_pos_z.z += cbp.pushed_force.z;
    cbp.pushed_force.z = 0.0f;
}

inline void person_update_given_new_pos(int id, Transform& transform,
                                        std::shared_ptr<Person> person,
                                        float dt, vec3 new_pos_x,
                                        vec3 new_pos_z) {
    int facedir_x = -1;
    int facedir_z = -1;

    vec3 delta_distance_x = new_pos_x - transform.raw_position;
    if (delta_distance_x.x > 0) {
        facedir_x = Transform::FrontFaceDirection::RIGHT;
    } else if (delta_distance_x.x < 0) {
        facedir_x = Transform::FrontFaceDirection::LEFT;
    }

    vec3 delta_distance_z = new_pos_z - transform.raw_position;
    if (delta_distance_z.z > 0) {
        facedir_z = Transform::FrontFaceDirection::FORWARD;
    } else if (delta_distance_z.z < 0) {
        facedir_z = Transform::FrontFaceDirection::BACK;
    }

    if (facedir_x == -1 && facedir_z == -1) {
        // do nothing
    } else if (facedir_x == -1) {
        transform.face_direction =
            static_cast<Transform::FrontFaceDirection>(facedir_z);
    } else if (facedir_z == -1) {
        transform.face_direction =
            static_cast<Transform::FrontFaceDirection>(facedir_x);
    } else {
        transform.face_direction =
            static_cast<Transform::FrontFaceDirection>(facedir_x | facedir_z);
    }

    // TODO what is this for
    // this->get<Transform>().face_direction =
    // Transform::FrontFaceDirection::BACK &
    // Transform::FrontFaceDirection::LEFT;

    // TODO this should be a component
    {
        // horizontal check
        auto new_bounds_x = get_bounds(new_pos_x, transform.size);
        // vertical check
        auto new_bounds_y = get_bounds(new_pos_z, transform.size);

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
            if (CheckCollisionBoxes(new_bounds_x, entity->bounds())) {
                would_collide_x = true;
                collided_entity_x = entity;
            }
            if (CheckCollisionBoxes(new_bounds_y, entity->bounds())) {
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

        if (!would_collide_x) {
            transform.raw_position.x = new_pos_x.x;
        }
        if (!would_collide_z) {
            transform.raw_position.z = new_pos_z.z;
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
                if (auto person_ptr_x = dynamic_cast<Person*>(entity_x.get())) {
                    CanBePushed& cbp = entity_x->get<CanBePushed>();
                    const float random_jitter = randSign() * TILESIZE / 2.0f;
                    if (facedir_x & Transform::FrontFaceDirection::LEFT) {
                        cbp.pushed_force.x += tile_div_push_mod;
                        cbp.pushed_force.z += random_jitter;
                    }
                    if (facedir_x & Transform::FrontFaceDirection::RIGHT) {
                        cbp.pushed_force.x -= tile_div_push_mod;
                        cbp.pushed_force.z += random_jitter;
                    }
                }
            }
            if (auto entity_z = collided_entity_z.lock()) {
                // TODO remove this check since we can just put CanBePushed
                // on the person entity and replace with a has<> check
                if (auto person_ptr_z = dynamic_cast<Person*>(entity_z.get())) {
                    CanBePushed& cbp = entity_z->get<CanBePushed>();
                    const float random_jitter = randSign() * TILESIZE / 2.0f;
                    if (facedir_z & Transform::FrontFaceDirection::FORWARD) {
                        cbp.pushed_force.x += random_jitter;
                        cbp.pushed_force.z += tile_div_push_mod;
                    }
                    if (facedir_z & Transform::FrontFaceDirection::BACK) {
                        cbp.pushed_force.x += random_jitter;
                        cbp.pushed_force.z -= tile_div_push_mod;
                    }
                }
            }
        }
    }
}

inline void collect_user_input(std::shared_ptr<Entity> entity, float dt) {
    if (!entity->has<CollectsUserInput>()) return;
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
    if (!entity->has<Transform>()) return;
    Transform& transform = entity->get<Transform>();
    std::shared_ptr<Player> player = dynamic_pointer_cast<Player>(entity);

    if (!entity->has<HasBaseSpeed>()) return;
    HasBaseSpeed& hasBaseSpeed = entity->get<HasBaseSpeed>();

    const float speed = hasBaseSpeed.speed() * dt;
    auto new_position = transform.position;

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
    transform.position = transform.raw_position;
};

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

    std::shared_ptr<Player> player = dynamic_pointer_cast<Player>(entity);
    if (!player) return;

    const auto rotate_furniture = [player]() {
        TRACY_ZONE_SCOPED;
        // Cant rotate outside planning mode
        if (GameState::get().is_not(game::State::Planning)) return;

        // TODO need to figure out if this should be separate from highlighting
        CanHighlightOthers& cho = player->get<CanHighlightOthers>();

        std::shared_ptr<Furniture> match =
            // TODO have this just take a transform
            EntityHelper::getClosestMatchingEntity<Furniture>(
                player->get<Transform>().as2(), cho.reach(),
                [](auto&& furniture) {
                    return furniture->template has<IsRotatable>();
                });

        if (!match) return;
        match->rotate_facing_clockwise();
    };

    const auto work_furniture = [player, frame_dt]() {
        TRACY_ZONE_SCOPED;
        // Cant do work during planning
        if (GameState::get().is(game::State::Planning)) return;

        // TODO need to figure out if this should be separate from highlighting
        CanHighlightOthers& cho = player->get<CanHighlightOthers>();

        std::shared_ptr<Furniture> match =
            EntityHelper::getClosestMatchingEntity<Furniture>(
                player->get<Transform>().as2(), cho.reach(),
                [](std::shared_ptr<Furniture> furniture) {
                    if (!furniture->has<HasWork>()) return false;
                    HasWork& hasWork = furniture->get<HasWork>();
                    return hasWork.has_work();
                });

        if (!match) return;

        HasWork& hasWork = match->get<HasWork>();
        if (hasWork.do_work) hasWork.do_work(hasWork, player, frame_dt);
    };

    const auto handle_in_game_grab_or_drop = [player]() {
        TRACY_ZONE_SCOPED;
        // TODO Need to auto drop any held furniture

        // TODO add has<> checks
        // TODO need to figure out if this should be separate from highlighting
        CanHighlightOthers& cho = player->get<CanHighlightOthers>();

        // Do we already have something in our hands?
        // We must be trying to drop it
        // TODO fix
        if (player->get<CanHoldItem>().item()) {
            const auto _merge_item_from_furniture_into_hand = [&]() {
                TRACY_ZONE(tracy_merge_item_from_furniture);
                // our item cant hold anything or is already full
                if (!player->get<CanHoldItem>().item()->empty()) {
                    return false;
                }

                std::shared_ptr<Furniture> closest_furniture =
                    EntityHelper::getMatchingEntityInFront<Furniture>(
                        player->get<Transform>().as2(), cho.reach(),
                        player->get<Transform>().face_direction,
                        [](std::shared_ptr<Furniture> f) {
                            return f->get<CanHoldItem>().is_holding_item();
                        });

                if (!closest_furniture) {
                    return false;
                }

                auto item_to_merge =
                    closest_furniture->get<CanHoldItem>().item();
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
                        player->get<Transform>().face_direction,
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
                if (!entity->has<CanHoldItem>()) return false;
                CanHoldItem& furnCanHold = entity->get<CanHoldItem>();

                // TODO add support for item containter..

                // If we are empty and can hold we good..
                return furnCanHold.empty();
            };

            const auto _place_item_onto_furniture = [&]() {
                TRACY_ZONE(tracy_place_item_onto_furniture);
                std::shared_ptr<Furniture> closest_furniture =
                    EntityHelper::getMatchingEntityInFront<Furniture>(
                        player->get<Transform>().as2(), cho.reach(),
                        player->get<Transform>().face_direction,
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
            const auto _pickup_item_from_furniture = [&]() {
                std::shared_ptr<Furniture> closest_furniture =
                    EntityHelper::getMatchingEntityInFront<Furniture>(
                        player->get<Transform>().as2(), cho.reach(),
                        player->get<Transform>().face_direction,
                        [](std::shared_ptr<Furniture> furn) {
                            // TODO fix
                            return (furn->get<CanHoldItem>().item() != nullptr);
                        });
                if (!closest_furniture) {
                    return;
                }
                CanHoldItem& furnCanHold =
                    closest_furniture->get<CanHoldItem>();

                player->get<CanHoldItem>().item() = furnCanHold.item();
                player->get<CanHoldItem>().item()->held_by =
                    Item::HeldBy::PLAYER;

                furnCanHold.item() = nullptr;
            };

            _pickup_item_from_furniture();

            if (player->get<CanHoldItem>().is_holding_item()) return;

            // Handles the non-furniture grabbing case
            std::shared_ptr<Item> closest_item =
                ItemHelper::getClosestMatchingItem<Item>(
                    player->get<Transform>().as2(), TILESIZE * cho.reach());
            player->get<CanHoldItem>().item() = closest_item;
            // TODO fix
            if (player->get<CanHoldItem>().item() != nullptr) {
                player->get<CanHoldItem>().item()->held_by =
                    Item::HeldBy::PLAYER;
            }
            return;
        }
    };

    const auto handle_in_planning_grab_or_drop = [player]() {
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
                hf->on_drop(vec::to3(player->tile_infront(1)));
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
                    player->get<Transform>().face_direction,
                    [](std::shared_ptr<Furniture> f) {
                        // TODO right now walls inherit this from furniture but
                        // eventually that should not be the case
                        return f->get<CanBeHeld>().is_not_held();
                    });
            ourCHF.update(closest_furniture);
            if (ourCHF.is_holding_furniture()) ourCHF.furniture()->on_pickup();
            // NOTE: we want to remove the furniture ONLY from the nav mesh
            //       when picked up because then AI can walk through,
            //       this also means we have to add it back when we place it
            // if (this->held_furniture) {
            // auto nv = GLOBALS.get_ptr<NavMesh>("navmesh");
            // nv->removeEntity(this->held_furniture->id);
            // }
            return;
        }
    };

    const auto grab_or_drop = [player, handle_in_planning_grab_or_drop,
                               handle_in_game_grab_or_drop]() {
        if (GameState::s_in_round()) {
            handle_in_game_grab_or_drop();
        } else if (GameState::get().is(game::State::Planning)) {
            handle_in_planning_grab_or_drop();
        } else {
            // TODO we probably want to handle messing around in the lobby
        }
    };

    switch (input_name) {
        case InputName::PlayerRotateFurniture:
            rotate_furniture();
            break;
        case InputName::PlayerPickup:
            grab_or_drop();
            break;
        case InputName::PlayerDoWork:
            work_furniture();
        default:
            break;
    }
}

}  // namespace system_manager

SINGLETON_FWD(SystemManager)
struct SystemManager {
    SINGLETON(SystemManager)

    void update(const Entities& entities, float dt) {
        always_update(entities, dt);
        // TODO do we run game updates during paused?
        if (GameState::get().is(game::State::InRound)) {
            in_round_update(entities, dt);
        } else {
            planning_update(entities, dt);
        }
    }

    void update(float dt) { update(EntityHelper::get_entities(), dt); }

    void process_inputs(const Entities& entities, const UserInputs& inputs) {
        for (auto& entity : entities) {
            if (!entity->has<RespondsToUserInput>()) continue;
            for (auto input : inputs) {
                system_manager::process_input(entity, input);
            }
        }
    }

    void render(const Entities& entities, float dt) const {
        const auto debug_mode_on =
            GLOBALS.get_or_default<bool>("debug_ui_enabled", false);
        if (debug_mode_on) {
            render_debug(entities, dt);
        } else {
            render_normal(entities, dt);
        }
    }

    void render(float dt) const { render(EntityHelper::get_entities(), dt); }

   private:
    void always_update(const std::vector<std::shared_ptr<Entity>>& entity_list,
                       float dt) {
        for (auto& entity : entity_list) {
            system_manager::reset_highlighted(entity, dt);
            system_manager::transform_snapper(entity, dt);
            system_manager::update_held_item_position(entity, dt);
            system_manager::collect_user_input(entity, dt);
        }
    }

    void in_round_update(
        const std::vector<std::shared_ptr<Entity>>& entity_list, float dt) {
        for (auto& entity : entity_list) {
            system_manager::job_system::handle_job_holder_pushed(entity, dt);
            system_manager::job_system::update_job_information(entity, dt);
        }
    }

    void planning_update(
        const std::vector<std::shared_ptr<Entity>>& entity_list, float dt) {
        for (auto& entity : entity_list) {
            system_manager::highlight_facing_furniture(entity, dt);
            system_manager::update_held_furniture_position(entity, dt);
        }
    }

    void render_normal(const std::vector<std::shared_ptr<Entity>>& entity_list,
                       float dt) const {
        for (auto& entity : entity_list) {
            system_manager::render_manager::render_normal(entity, dt);
            system_manager::render_manager::render_floating_name(entity, dt);
            system_manager::render_manager::render_progress_bar(entity, dt);
        }
    }

    void render_debug(const std::vector<std::shared_ptr<Entity>>& entity_list,
                      float dt) const {
        for (auto& entity : entity_list) {
            system_manager::render_manager::render_debug(entity, dt);
        }
    }
};
