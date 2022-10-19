
#pragma once

#include "globals.h"
#include "keymap.h"
#include "person.h"
#include "raylib.h"
#include "sound_library.h"
//
#include "furniture.h"

struct Player : public Person {
    float player_reach = 1.25f;
    std::shared_ptr<Furniture> held_furniture;

    Player(vec3 p, Color face_color_in, Color base_color_in)
        : Person(p, face_color_in, base_color_in) {}
    Player(vec2 p, Color face_color_in, Color base_color_in)
        : Person(p, face_color_in, base_color_in) {}
    Player() : Person({0, 0, 0}, {0, 255, 0, 255}, {255, 0, 0, 255}) {}
    Player(vec2 location)
        : Person({location.x, 0, location.y}, {0, 255, 0, 255},
                 {255, 0, 0, 255}) {}

    virtual float base_speed() override { return 7.5f; }

    virtual vec3 update_xaxis_position(float dt) override {
        float speed = this->base_speed() * dt;
        auto new_pos_x = this->raw_position;
        float left = KeyMap::is_event(Menu::State::Game, "Player Left");
        float right = KeyMap::is_event(Menu::State::Game, "Player Right");
        new_pos_x.x -= left * speed;
        new_pos_x.x += right * speed;
        return new_pos_x;
    }

    virtual vec3 update_zaxis_position(float dt) override {
        float speed = this->base_speed() * dt;
        auto new_pos_z = this->raw_position;
        float up = KeyMap::is_event(Menu::State::Game, "Player Forward");
        float down = KeyMap::is_event(Menu::State::Game, "Player Back");
        new_pos_z.z -= up * speed;
        new_pos_z.z += down * speed;
        return new_pos_z;
    }

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
                PlaySound(SoundLibrary::get().get("roblox"));
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
            std::shared_ptr<Item> closest_item =
                EntityHelper::getClosestMatchingItem(vec::to2(this->position),
                                                     TILESIZE * player_reach);
            this->held_item = closest_item;
            return;
        }
    }
};
