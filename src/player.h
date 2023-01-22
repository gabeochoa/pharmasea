
#pragma once

#include "base_player.h"
#include "components/can_be_ghost_player.h"
#include "components/can_hold_furniture.h"
#include "components/collects_user_input.h"
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

    void add_static_components() {
        addComponent<CanBeGhostPlayer>();
        addComponent<CollectsUserInput>();
        addComponent<RespondsToUserInput>();
    }

    // NOTE: this is kept public because we use it in the network when prepping
    // server players
    Player() : BasePlayer({0, 0, 0}, WHITE, WHITE) { add_static_components(); }

    Player(vec3 p, Color face_color_in, Color base_color_in)
        : BasePlayer(p, face_color_in, base_color_in) {
        add_static_components();
    }
    Player(vec2 p, Color face_color_in, Color base_color_in)
        : BasePlayer(p, face_color_in, base_color_in) {
        add_static_components();
    }
    Player(vec2 location)
        : BasePlayer({location.x, 0, location.y}, {0, 255, 0, 255},
                     {255, 0, 0, 255}) {
        add_static_components();
    }

    virtual bool is_collidable() override {
        return get<CanBeGhostPlayer>().is_not_ghost();
    }

    void handle_in_game_grab_or_drop() {
        TRACY_ZONE_SCOPED;
        // TODO Need to auto drop any held furniture

        // TODO need to figure out if this should be separate from highlighting
        CanHighlightOthers& cho = this->get<CanHighlightOthers>();

        // Do we already have something in our hands?
        // We must be trying to drop it
        // TODO fix
        if (this->get<CanHoldItem>().item()) {
            const auto _merge_item_from_furniture_into_hand = [&]() {
                TRACY_ZONE(tracy_merge_item_from_furniture);
                // our item cant hold anything or is already full
                if (!this->get<CanHoldItem>().item()->empty()) {
                    return false;
                }

                std::shared_ptr<Furniture> closest_furniture =
                    EntityHelper::getMatchingEntityInFront<Furniture>(
                        this->get<Transform>().as2(), cho.reach(),
                        this->get<Transform>().face_direction,
                        [](std::shared_ptr<Furniture> f) {
                            return f->get<CanHoldItem>().is_holding_item();
                        });

                if (!closest_furniture) {
                    return false;
                }

                auto item_to_merge =
                    closest_furniture->get<CanHoldItem>().item();
                bool eat_was_successful =
                    this->get<CanHoldItem>().item()->eat(item_to_merge);
                if (eat_was_successful)
                    closest_furniture->get<CanHoldItem>().item() = nullptr;
                return eat_was_successful;
            };

            const auto _merge_item_in_hand_into_furniture_item = [&]() {
                TRACY_ZONE(tracy_merge_item_in_hand_into_furniture);
                std::shared_ptr<Furniture> closest_furniture =
                    EntityHelper::getMatchingEntityInFront<Furniture>(
                        this->get<Transform>().as2(), cho.reach(),
                        this->get<Transform>().face_direction,
                        [&](std::shared_ptr<Furniture> f) {
                            return
                                // is there something there to merge into?
                                f->get<CanHoldItem>().is_holding_item() &&
                                // can that thing hold the item we are holding?
                                f->get<CanHoldItem>().item()->can_eat(
                                    this->get<CanHoldItem>().item());
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
                        this->get<CanHoldItem>().item());
                if (!eat_was_successful) return false;
                // TODO we need a let_go_of_item() to handle this kind of
                // transfer because it might get complicated and we might end up
                // with two things owning this could maybe be solved by
                // enforcing uniqueptr
                this->get<CanHoldItem>().item() = nullptr;
                return true;
            };

            const auto _place_item_onto_furniture = [&]() {
                TRACY_ZONE(tracy_place_item_onto_furniture);
                std::shared_ptr<Furniture> closest_furniture =
                    EntityHelper::getMatchingEntityInFront<Furniture>(
                        this->get<Transform>().as2(), cho.reach(),
                        this->get<Transform>().face_direction,
                        [this](std::shared_ptr<Furniture> f) {
                            return f->can_place_item_into(
                                this->get<CanHoldItem>().item());
                        });
                if (!closest_furniture) {
                    return false;
                }

                auto item = this->get<CanHoldItem>().item();
                item->update_position(
                    closest_furniture->get<Transform>().snap_position());

                closest_furniture->get<CanHoldItem>().item() = item;
                closest_furniture->get<CanHoldItem>().item()->held_by =
                    Item::HeldBy::FURNITURE;

                this->get<CanHoldItem>().item() = nullptr;
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
                        this->get<Transform>().as2(), cho.reach(),
                        this->get<Transform>().face_direction,
                        [](std::shared_ptr<Furniture> furn) {
                            // TODO fix
                            return (furn->get<CanHoldItem>().item() != nullptr);
                        });
                if (!closest_furniture) {
                    return;
                }
                CanHoldItem& furnCanHold =
                    closest_furniture->get<CanHoldItem>();

                this->get<CanHoldItem>().item() = furnCanHold.item();
                this->get<CanHoldItem>().item()->held_by = Item::HeldBy::PLAYER;

                furnCanHold.item() = nullptr;
            };

            _pickup_item_from_furniture();

            if (this->get<CanHoldItem>().is_holding_item()) return;

            // Handles the non-furniture grabbing case
            std::shared_ptr<Item> closest_item =
                ItemHelper::getClosestMatchingItem<Item>(
                    this->get<Transform>().as2(), TILESIZE * cho.reach());
            this->get<CanHoldItem>().item() = closest_item;
            // TODO fix
            if (this->get<CanHoldItem>().item() != nullptr) {
                this->get<CanHoldItem>().item()->held_by = Item::HeldBy::PLAYER;
            }
            return;
        }
    }

    void handle_in_planning_grab_or_drop() {
        TRACY_ZONE_SCOPED;
        // TODO: Need to delete any held items when switching from game ->
        // planning

        // TODO need to auto drop when "in_planning" changes

        // TODO need to figure out if this should be separate from highlighting
        CanHighlightOthers& cho = this->get<CanHighlightOthers>();

        CanHoldFurniture& ourCHF = get<CanHoldFurniture>();

        if (ourCHF.is_holding_furniture()) {
            const auto _drop_furniture = [&]() {
                // TODO need to make sure it doesnt place ontop of another
                // one
                auto hf = ourCHF.furniture();
                hf->on_drop(vec::to3(this->tile_infront(1)));
                ourCHF.update(nullptr);
            };
            _drop_furniture();
            return;
        } else {
            // TODO support finding things in the direction the player is
            // facing, instead of in a box around him
            std::shared_ptr<Furniture> closest_furniture =
                EntityHelper::getMatchingEntityInFront<Furniture>(
                    this->get<Transform>().as2(), cho.reach(),
                    this->get<Transform>().face_direction,
                    [](std::shared_ptr<Furniture> f) {
                        return f->can_be_picked_up();
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
    }

    void grab_or_drop() {
        if (GameState::s_in_round()) {
            handle_in_game_grab_or_drop();
        } else if (GameState::get().is(game::State::Planning)) {
            handle_in_planning_grab_or_drop();
        } else {
            // TODO we probably want to handle messing around in the lobby
        }
    }
};
