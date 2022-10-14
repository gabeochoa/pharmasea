
#pragma once

#include "globals.h"
#include "keymap.h"
#include "person.h"
#include "raylib.h"
//
#include "furniture.h"

struct Player : public Person {
    float player_reach = 4.f;

    Player(vec3 p, Color face_color_in, Color base_color_in)
        : Person(p, face_color_in, base_color_in) {}
    Player(vec2 p, Color face_color_in, Color base_color_in)
        : Person(p, face_color_in, base_color_in) {}
    Player() : Person({0, 0, 0}, {0, 255, 0, 255}, {255, 0, 0, 255}) {}
    Player(vec2 location)
        : Person({location.x, 0, location.y}, {0, 255, 0, 255},
                 {255, 0, 0, 255}) {}

    virtual vec3 update_xaxis_position(float dt) override {
        float speed = 10.0f * dt;
        auto new_pos_x = this->raw_position;
        float left = KeyMap::is_event(Menu::State::Game, "Player Left");
        float right = KeyMap::is_event(Menu::State::Game, "Player Right");
        new_pos_x.x -= left * speed;
        new_pos_x.x += right * speed;
        return new_pos_x;
    }

    virtual vec3 update_zaxis_position(float dt) override {
        float speed = 10.0f * dt;
        auto new_pos_z = this->raw_position;
        float up = KeyMap::is_event(Menu::State::Game, "Player Forward");
        float down = KeyMap::is_event(Menu::State::Game, "Player Back");
        new_pos_z.z -= up * speed;
        new_pos_z.z += down * speed;
        return new_pos_z;
    }

    virtual void update(float dt) override {
        Person::update(dt);

        grab_or_drop_item();
        rotate_furniture();
    }

    virtual void rotate_furniture(){
        std::shared_ptr<Furniture> match= EntityHelper::getClosestMatchingEntity<Furniture>(vec::to2(this->position), TILESIZE * player_reach);
        if(match){
            float rotate = KeyMap::is_event_once_DO_NOT_USE(Menu::State::Game, "Player Rotate Furniture");
            if(rotate > 0.5f) match->rotate_facing_clockwise();
        }
    }

    virtual void grab_or_drop_item() {
        bool pickup = KeyMap::is_event_once_DO_NOT_USE(Menu::State::Game,
                                                       "Player Pickup");
        if (pickup) {
            if (held_item != nullptr) {
                held_item = nullptr;
            } else {
                std::shared_ptr<Item> closest_item = nullptr;
                float best_distance = std::numeric_limits<float>::max();
                for (auto item : items_DO_NOT_USE) {
                    if (item->collides(
                            get_bounds(this->position, this->size() * player_reach))) {
                        auto current_distance = vec::distance(
                            vec::to2(item->position), vec::to2(this->position));
                        if (current_distance < best_distance) {
                            best_distance = current_distance;
                            closest_item = item;
                        }
                    }
                }
                // std::cout << "Grabbing item:" << closest_item << std::endl;
                if (closest_item != nullptr) {
                    this->held_item = closest_item;
                }
            }
        }
    }
};
