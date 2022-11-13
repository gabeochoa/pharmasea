
#pragma once

#include "base_player.h"
#include "globals.h"
#include "keymap.h"
#include "raylib.h"
//
#include "furniture.h"

struct Player : public BasePlayer {
    std::string username;
    std::vector<UserInput> inputs;
    bool is_ghost_player = false;

    Player(vec3 p, Color face_color_in, Color base_color_in)
        : BasePlayer(p, face_color_in, base_color_in) {}
    Player(vec2 p, Color face_color_in, Color base_color_in)
        : BasePlayer(p, face_color_in, base_color_in) {}
    Player() : BasePlayer({0, 0, 0}, {0, 255, 0, 255}, {255, 0, 0, 255}) {}
    Player(vec2 location)
        : BasePlayer({location.x, 0, location.y}, {0, 255, 0, 255},
                     {255, 0, 0, 255}) {}

    virtual bool draw_outside_debug_mode() const override {
        return !is_ghost_player;
    }
    virtual bool is_collidable() override { return !is_ghost_player; }

    virtual vec3 update_xaxis_position(float dt) override {
        float left = KeyMap::is_event(Menu::State::Game, "Player Left");
        float right = KeyMap::is_event(Menu::State::Game, "Player Right");
        // TODO do we have to worry about having branches?
        // I feel like sending 4 floats over network probably worse than 4
        // branches on checking float positive but idk
        if (left > 0)
            inputs.push_back({Menu::State::Game, "Player Left", left, dt});
        if (right > 0)
            inputs.push_back({Menu::State::Game, "Player Right", right, dt});
        return this->raw_position;
    }

    virtual vec3 update_zaxis_position(float dt) override {
        float up = KeyMap::is_event(Menu::State::Game, "Player Forward");
        float down = KeyMap::is_event(Menu::State::Game, "Player Back");
        if (up > 0)
            inputs.push_back({Menu::State::Game, "Player Forward", up, dt});
        if (down > 0)
            inputs.push_back({Menu::State::Game, "Player Back", down, dt});
        return this->raw_position;
    }

    virtual void update(float dt) override {
        BasePlayer::update(dt);

        bool pickup = KeyMap::is_event_once_DO_NOT_USE(Menu::State::Game,
                                                       "Player Pickup");
        if (pickup) {
            inputs.push_back({Menu::State::Game, "Player Pickup", 1.f, dt});
        }
    }

    virtual vec3 get_position_after_input(UserInputs inpts) {
        for (UserInput& ui : inpts) {
            auto menu_state = std::get<0>(ui);
            if (menu_state != Menu::State::Game) continue;

            std::string input_key_name = std::get<1>(ui);
            float input_amount = std::get<2>(ui);
            float frame_dt = std::get<3>(ui);

            if (input_key_name == "Player Pickup" && input_amount > 0.f) {
                grab_or_drop();
                continue;
            }

            // Movement down here...

            float speed = this->base_speed() * frame_dt;
            auto new_position = this->position;

            if (input_key_name == "Player Left") {
                new_position.x -= input_amount * speed;
            } else if (input_key_name == "Player Right") {
                new_position.x += input_amount * speed;
            } else if (input_key_name == "Player Forward") {
                new_position.z -= input_amount * speed;
            } else if (input_key_name == "Player Back") {
                new_position.z += input_amount * speed;
            }

            auto fd = get_face_direction(new_position, new_position);
            int fd_x = std::get<0>(fd);
            int fd_z = std::get<1>(fd);
            update_facing_direction(fd_x, fd_z);
            handle_collision(fd_x, new_position, fd_z, new_position);
            this->position = this->raw_position;
        }
        return this->position;
    }

    // TODO how to handle when they are holding something?
    virtual void grab_or_drop() {
        // Do we already have something in our hands?
        if (this->held_item) {
            const auto _drop_item = [&]() {
                std::shared_ptr<Furniture> closest_furniture =
                    EntityHelper::getMatchingEntityInFront<Furniture>(
                        vec::to2(this->position), player_reach,
                        this->face_direction, [](std::shared_ptr<Furniture> f) {
                            return f->can_place_item_into();
                        });
                if (closest_furniture) {
                    this->held_item->update_position(
                        vec::snap(closest_furniture->position));
                    closest_furniture->held_item = this->held_item;
                    closest_furniture->held_item->held_by =
                        Item::HeldBy::FURNITURE;
                    this->held_item = nullptr;
                }
            };
            _drop_item();
            return;
        }

        // TODO need to auto drop when "in_planning" changes
        if (this->held_furniture) {
            const auto _drop_furniture = [&]() {
                // TODO need to make sure it doesnt place ontop of another one
                this->held_furniture->update_position(
                    vec::snap(this->held_furniture->position));
                EntityHelper::addEntity(this->held_furniture);
                this->held_furniture = nullptr;
            };
            _drop_furniture();
            return;
        }

        // TODO support finding things in the direction the player is facing,
        // instead of in a box around him

        if (GLOBALS.get_or_default("in_planning", false)) {
            std::shared_ptr<Furniture> closest_furniture =
                EntityHelper::getMatchingEntityInFront<Furniture>(
                    vec::to2(this->position), player_reach,
                    this->face_direction, [](std::shared_ptr<Furniture> f) {
                        return f->can_be_picked_up();
                    });
            this->held_furniture = closest_furniture;
            // NOTE: we want to remove the furniture ONLY from the nav mesh
            //       when picked up because then AI can walk through,
            //       this also means we have to add it back when we place it
            // if (this->held_furniture) {
            // auto nv = GLOBALS.get_ptr<NavMesh>("navmesh");
            // nv->removeEntity(this->held_furniture->id);
            // }
            return;
        } else {
            // TODO logic for the closest furniture holding an item within reach
            // Return early if it is found
            std::shared_ptr<Item> closest_item = nullptr;
            const auto _pickup_item_from_furniture = [&]() {
                std::shared_ptr<Furniture> closest_furniture =
                    EntityHelper::getMatchingEntityInFront<Furniture>(
                        vec::to2(this->position), player_reach,
                        this->face_direction,
                        [](std::shared_ptr<Furniture>) { return true; });
                if (closest_furniture &&
                    closest_furniture->held_item != nullptr) {
                    this->held_item = closest_furniture->held_item;
                    this->held_item->held_by = Item::HeldBy::PLAYER;
                    closest_furniture->held_item = nullptr;
                }
            };
            _pickup_item_from_furniture();
            if (this->held_item) return;

            // Handles the non-furniture grabbing case
            closest_item = ItemHelper::getClosestMatchingItem<Item>(
                vec::to2(this->position), TILESIZE * player_reach);
            this->held_item = closest_item;
            if (this->held_item != nullptr) {
                this->held_item->held_by = Item::HeldBy::PLAYER;
            }
            return;
        }
    }
};
