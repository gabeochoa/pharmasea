
#pragma once

#include "../base_player.h"
#include "../components/can_be_ghost_player.h"
#include "../components/can_highlight_others.h"
#include "../components/can_hold_furniture.h"
#include "../components/can_perform_job.h"
#include "../components/collects_user_input.h"
#include "../components/custom_item_position.h"
#include "../components/responds_to_user_input.h"
#include "../components/transform.h"
#include "../customer.h"
#include "../entity.h"
#include "../entityhelper.h"
#include "../furniture.h"
#include "../furniture/register.h"
#include "../player.h"
#include "job_system.h"
#include "rendering_system.h"

namespace system_manager {

inline void transform_snapper(std::shared_ptr<Entity> entity, float) {
    if (!entity->has<Transform>()) return;

    Transform& transform = entity->get<Transform>();
    if (entity->is_snappable()) {
        transform.position = transform.snap_position();
    } else {
        transform.position = transform.raw_position;
    }
}

inline void update_held_furniture_position(std::shared_ptr<Entity> entity,
                                           float) {
    if (!entity->has<CanHoldFurniture>()) return;
    const CanHoldFurniture& can_hold_furniture =
        entity->get<CanHoldFurniture>();

    // TODO explicity commenting this out so that we get an error
    // if (!entity->has<Transform>()) return;
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

    can_hold_furniture.furniture()->update_position(new_pos);
}

inline void update_held_item_position(std::shared_ptr<Entity> entity, float) {
    if (!entity->has<CanHoldItem>()) return;
    CanHoldItem& can_hold_item = entity->get<CanHoldItem>();
    if (can_hold_item.empty()) return;

    if (!entity->has<Transform>()) return;
    Transform& transform = entity->get<Transform>();

    if (entity->has<CustomHeldItemPosition>()) {
        CustomHeldItemPosition& custom_item_position =
            entity->get<CustomHeldItemPosition>();

        if (custom_item_position.mutator) {
            can_hold_item.item()->update_position(
                custom_item_position.mutator(transform));
        } else {
            log_warn(
                "Entity has custom held item position but didnt initalize the "
                "component");
        }
        return;
    }

    // Default

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
            if (!entity->is_collidable()) {
                return EntityHelper::ForEachFlow::Continue;
            }
            if (!person->is_collidable()) {
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

    const float speed = player->base_speed() * dt;
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
                [](auto&& furniture) { return furniture->can_rotate(); });

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
                [](auto&& furniture) { return furniture->has_work(); });

        if (!match) return;

        match->do_work(frame_dt, player);
    };

    switch (input_name) {
        case InputName::PlayerRotateFurniture:
            rotate_furniture();
            break;
        case InputName::PlayerPickup:
            player->grab_or_drop();
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
        }
    }

    void render_debug(const std::vector<std::shared_ptr<Entity>>& entity_list,
                      float dt) const {
        for (auto& entity : entity_list) {
            system_manager::render_manager::render_debug(entity, dt);
        }
    }
};
