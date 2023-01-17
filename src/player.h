
#pragma once

#include "base_player.h"
#include "engine/keymap.h"
#include "globals.h"
#include "raylib.h"
#include "statemanager.h"
//
#include "furniture.h"

struct Player : public BasePlayer {
    // Theres no players not in game menu state,
    const menu::State state = menu::State::Game;

    std::string username;
    std::vector<UserInput> inputs;
    bool is_ghost_player = false;

    // NOTE: this is kept public because we use it in the network when prepping
    // server players
    Player() : BasePlayer({0, 0, 0}, WHITE, WHITE) {}

    Player(vec3 p, Color face_color_in, Color base_color_in)
        : BasePlayer(p, face_color_in, base_color_in) {}
    Player(vec2 p, Color face_color_in, Color base_color_in)
        : BasePlayer(p, face_color_in, base_color_in) {}
    Player(vec2 location)
        : BasePlayer({location.x, 0, location.y}, {0, 255, 0, 255},
                     {255, 0, 0, 255}) {}

    virtual void render_normal() const override {
        if (!is_ghost_player) {
            BasePlayer::render_normal();
        }
    }

    virtual void render_debug_mode() const override {
        if (is_ghost_player) {
            BasePlayer::render_normal();
        }
    }

    virtual bool is_collidable() override { return !is_ghost_player; }

    virtual vec3 update_xaxis_position(float dt) override {
        float left = KeyMap::is_event(state, InputName::PlayerLeft);
        float right = KeyMap::is_event(state, InputName::PlayerRight);
        if (left > 0)
            inputs.push_back({state, InputName::PlayerLeft, left, dt});
        if (right > 0)
            inputs.push_back({state, InputName::PlayerRight, right, dt});
        return this->raw_position;
    }

    virtual vec3 update_zaxis_position(float dt) override {
        float up = KeyMap::is_event(state, InputName::PlayerForward);
        float down = KeyMap::is_event(state, InputName::PlayerBack);
        if (up > 0) inputs.push_back({state, InputName::PlayerForward, up, dt});
        if (down > 0)
            inputs.push_back({state, InputName::PlayerBack, down, dt});
        return this->raw_position;
    }

    virtual void always_update(float dt) override {
        TRACY_ZONE_SCOPED;
        BasePlayer::always_update(dt);

        bool pickup =
            KeyMap::is_event_once_DO_NOT_USE(state, InputName::PlayerPickup);
        if (pickup) {
            inputs.push_back({state, InputName::PlayerPickup, 1.f, dt});
        }

        bool rotate = KeyMap::is_event_once_DO_NOT_USE(
            state, InputName::PlayerRotateFurniture);
        if (rotate) {
            inputs.push_back(
                {state, InputName::PlayerRotateFurniture, 1.f, dt});
        }

        float do_work = KeyMap::is_event(state, InputName::PlayerDoWork);
        if (do_work > 0) {
            inputs.push_back({state, InputName::PlayerDoWork, 1.f, dt});
        }
    }

    // TODO interpolate our old position and new position so its smoother
    virtual vec3 get_position_after_input(UserInputs inpts) {
        TRACY_ZONE_SCOPED;
        // log_trace("get_position_after_input {}", inpts.size());
        for (const UserInput& ui : inpts) {
            const auto menu_state = std::get<0>(ui);
            if (menu_state != state) continue;

            const InputName input_name = std::get<1>(ui);
            const float input_amount = std::get<2>(ui);
            const float frame_dt = std::get<3>(ui);

            if (input_name == InputName::PlayerPickup && input_amount > 0.5f) {
                grab_or_drop();
                continue;
            }

            if (input_name == InputName::PlayerRotateFurniture &&
                input_amount > 0.5f) {
                rotate_furniture();
                continue;
            }

            // TODO replace with correctly named input
            if (input_name == InputName::PlayerDoWork && input_amount > 0.5f) {
                work_furniture(frame_dt);
                continue;
            }

            // Movement down here...

            const float speed = this->base_speed() * frame_dt;
            auto new_position = this->position;

            if (input_name == InputName::PlayerLeft) {
                new_position.x -= input_amount * speed;
            } else if (input_name == InputName::PlayerRight) {
                new_position.x += input_amount * speed;
            } else if (input_name == InputName::PlayerForward) {
                new_position.z -= input_amount * speed;
            } else if (input_name == InputName::PlayerBack) {
                new_position.z += input_amount * speed;
            }

            const auto fd = get_face_direction(new_position, new_position);
            int fd_x = std::get<0>(fd);
            int fd_z = std::get<1>(fd);
            update_facing_direction(fd_x, fd_z);
            handle_collision(fd_x, new_position, fd_z, new_position);
            this->position = this->raw_position;
        }
        return this->position;
    }

    void rotate_furniture() {
        TRACY_ZONE_SCOPED;
        // Cant rotate outside planning mode
        if (GameState::get().is_not(game::State::Planning)) return;

        std::shared_ptr<Furniture> match =
            EntityHelper::getClosestMatchingEntity<Furniture>(
                vec::to2(this->position), player_reach,
                [](auto&& furniture) { return furniture->can_rotate(); });

        if (!match) return;

        match->rotate_facing_clockwise();
    }

    void work_furniture(float frame_dt) {
        TRACY_ZONE_SCOPED;
        // Cant do work during planning
        if (GameState::get().is(game::State::Planning)) return;

        std::shared_ptr<Furniture> match =
            EntityHelper::getClosestMatchingEntity<Furniture>(
                vec::to2(this->position), player_reach,
                [](auto&& furniture) { return furniture->has_work(); });

        if (!match) return;

        match->do_work(frame_dt, this);
    }

    void handle_in_game_grab_or_drop() {
        TRACY_ZONE_SCOPED;
        // TODO Need to auto drop any held furniture

        // Do we already have something in our hands?
        // We must be trying to drop it
        if (this->held_item) {
            const auto _merge_item_from_furniture_into_hand = [&]() {
                TRACY_ZONE(tracy_merge_item_from_furniture);
                // our item cant hold anything or is already full
                if (!this->held_item->empty()) {
                    return false;
                }

                std::shared_ptr<Furniture> closest_furniture =
                    EntityHelper::getMatchingEntityInFront<Furniture>(
                        vec::to2(this->position), player_reach,
                        this->face_direction, [](std::shared_ptr<Furniture> f) {
                            return f->has_held_item();
                        });

                if (!closest_furniture) {
                    return false;
                }

                auto item_to_merge = closest_furniture->held_item;
                bool eat_was_successful = this->held_item->eat(item_to_merge);
                if (eat_was_successful) closest_furniture->held_item = nullptr;
                return eat_was_successful;
            };

            const auto _merge_item_in_hand_into_furniture_item = [&]() {
                TRACY_ZONE(tracy_merge_item_in_hand_into_furniture);
                std::shared_ptr<Furniture> closest_furniture =
                    EntityHelper::getMatchingEntityInFront<Furniture>(
                        vec::to2(this->position), player_reach,
                        this->face_direction,
                        [&](std::shared_ptr<Furniture> f) {
                            return
                                // is there something there to merge into?
                                f->has_held_item() &&
                                // can that thing hold the item we are holding?
                                f->held_item->can_eat(this->held_item);
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
                    closest_furniture->held_item->eat(this->held_item);
                if (!eat_was_successful) return false;
                // TODO we need a let_go_of_item() to handle this kind of
                // transfer because it might get complicated and we might end up
                // with two things owning this could maybe be solved by
                // enforcing uniqueptr
                this->held_item = nullptr;
                return true;
            };

            const auto _place_item_onto_furniture = [&]() {
                TRACY_ZONE(tracy_place_item_onto_furniture);
                std::shared_ptr<Furniture> closest_furniture =
                    EntityHelper::getMatchingEntityInFront<Furniture>(
                        vec::to2(this->position), player_reach,
                        this->face_direction,
                        [this](std::shared_ptr<Furniture> f) {
                            return f->can_place_item_into(this->held_item);
                        });
                if (!closest_furniture) {
                    return false;
                }

                auto item = this->held_item;
                item->update_position(vec::snap(closest_furniture->position));

                closest_furniture->held_item = item;
                closest_furniture->held_item->held_by = Item::HeldBy::FURNITURE;

                this->held_item = nullptr;
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
                        vec::to2(this->position), player_reach,
                        this->face_direction,
                        [](std::shared_ptr<Furniture> furn) {
                            return (furn->held_item != nullptr);
                        });
                if (!closest_furniture) {
                    return;
                }
                auto item = closest_furniture->held_item;

                this->held_item = item;
                this->held_item->held_by = Item::HeldBy::PLAYER;

                closest_furniture->held_item = nullptr;
            };

            _pickup_item_from_furniture();

            if (this->held_item) return;

            // Handles the non-furniture grabbing case
            std::shared_ptr<Item> closest_item =
                ItemHelper::getClosestMatchingItem<Item>(
                    vec::to2(this->position), TILESIZE * player_reach);
            this->held_item = closest_item;
            if (this->held_item != nullptr) {
                this->held_item->held_by = Item::HeldBy::PLAYER;
            }
            return;
        }
    }

    void handle_in_planning_grab_or_drop() {
        TRACY_ZONE_SCOPED;
        // TODO: Need to delete any held items when switching from game ->
        // planning

        // TODO need to auto drop when "in_planning" changes
        if (this->held_furniture) {
            const auto _drop_furniture = [&]() {
                // TODO need to make sure it doesnt place ontop of another
                // one
                auto hf = this->held_furniture;
                hf->on_drop(vec::to3(this->tile_infront(1)));
                this->held_furniture = nullptr;
            };
            _drop_furniture();
            return;
        } else {
            // TODO support finding things in the direction the player is
            // facing, instead of in a box around him
            std::shared_ptr<Furniture> closest_furniture =
                EntityHelper::getMatchingEntityInFront<Furniture>(
                    vec::to2(this->position), player_reach,
                    this->face_direction, [](std::shared_ptr<Furniture> f) {
                        return f->can_be_picked_up();
                    });
            this->held_furniture = closest_furniture;
            if (this->held_furniture) this->held_furniture->on_pickup();
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

    virtual void grab_or_drop() {
        if (GameState::s_in_round()) {
            handle_in_game_grab_or_drop();
        } else if (GameState::get().is(game::State::Planning)) {
            handle_in_planning_grab_or_drop();
        } else {
            // TODO we probably want to handle messing around in the lobby
        }
    }
};
