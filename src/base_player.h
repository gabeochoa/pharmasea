

#pragma once

#include "globals.h"
#include "keymap.h"
#include "person.h"
#include "raylib.h"
//
#include "furniture.h"

struct BasePlayer : public Person {
    float player_reach = 1.25f;
    std::shared_ptr<Furniture> held_furniture;

    BasePlayer(vec3 p, Color face_color_in, Color base_color_in)
        : Person(p, face_color_in, base_color_in) {}
    BasePlayer(vec2 p, Color face_color_in, Color base_color_in)
        : Person(p, face_color_in, base_color_in) {}
    BasePlayer() : Person({0, 0, 0}, {0, 255, 0, 255}, {255, 0, 0, 255}) {}
    BasePlayer(vec2 location)
        : Person({location.x, 0, location.y}, {0, 255, 0, 255},
                 {255, 0, 0, 255}) {}

    virtual float base_speed() override { return 7.5f; }

    virtual vec3 update_xaxis_position(float dt) override = 0;
    virtual vec3 update_zaxis_position(float dt) override = 0;

    void highlight_facing_furniture() {
        auto match = EntityHelper::getMatchingEntityInFront<Furniture>(
            vec::to2(this->position), player_reach, this->face_direction,
            [](std::shared_ptr<Furniture>) { return true; });
        if (match) {
            match->is_highlighted = true;
        }
    }

    virtual void update(float dt) override {
        Person::update(dt);
        highlight_facing_furniture();
        grab_or_drop();
        rotate_furniture();

        // TODO if cannot be placed in this spot
        // make it obvious to the user
        if (held_furniture != nullptr) {
            auto new_pos = this->position;
            if (this->face_direction & FrontFaceDirection::FORWARD) {
                new_pos.z += TILESIZE;
            }
            if (this->face_direction & FrontFaceDirection::RIGHT) {
                new_pos.x += TILESIZE;
            }
            if (this->face_direction & FrontFaceDirection::BACK) {
                new_pos.z -= TILESIZE;
            }
            if (this->face_direction & FrontFaceDirection::LEFT) {
                new_pos.x -= TILESIZE;
            }

            held_furniture->update_position(new_pos);
        }
    }

    virtual void rotate_furniture() {
        if (GLOBALS.get_or_default("in_planning", false)) {
            std::shared_ptr<Furniture> match =
                EntityHelper::getClosestMatchingEntity<Furniture>(
                    vec::to2(this->position), player_reach,
                    [](auto&&) { return true; });
            if (match && match->can_rotate()) {
                float rotate = KeyMap::is_event_once_DO_NOT_USE(
                    Menu::State::Game, "Player Rotate Furniture");
                if (rotate > 0.5f) match->rotate_facing_clockwise();
            }
        }
    }

    // TODO how to handle when they are holding something?
    virtual void grab_or_drop() {
        bool pickup = KeyMap::is_event_once_DO_NOT_USE(Menu::State::Game,
                                                       "Player Pickup");
        if (!pickup) {
            return;
        }

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
            if (this->held_furniture) {
                auto nv = GLOBALS.get_ptr<NavMesh>("navmesh");
                nv->removeEntity(this->held_furniture->id);
            }
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
            closest_item = EntityHelper::getClosestMatchingItem(
                vec::to2(this->position), TILESIZE * player_reach);
            this->held_item = closest_item;
            if (this->held_item != nullptr) {
                this->held_item->held_by = Item::HeldBy::PLAYER;
            }
            return;
        }
    }
};
