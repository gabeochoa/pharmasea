

#pragma once

#include <bitsery/ext/inheritance.h>
#include <bitsery/ext/pointer.h>
#include <bitsery/ext/std_smart_ptr.h>

#include <memory>
#include <random>

#include "engine.h"
#include "external_include.h"
//
#include "engine/log.h"
#include "engine/random.h"
#include "entity.h"
#include "furniture/character_switcher.h"
#include "furniture/medicine_cabinet.h"
#include "furnitures.h"
#include "level_info.h"
#include "remote_player.h"
#include "statemanager.h"

const std::string TINY = R"(
.........
.........
..A.0..T
.........
..@......
........#)";

const std::string EXAMPLE_MAP = R"(
####################################################
#......TTTTTTTTTTTT................................#
#..................................................#
#........@R..............C..CCCC...................#
#..................................................#
#.............0....................................#
#..................................................#
#..................................................#
#..................................................#
#..................................................#
#..................................................#
#########################.##########.###############
#.............#....................................#
#.............#....................................#
#.............#....................................#
#.............#....................................#
#.............#....................................#
#.............#....................................#
#.............#....................................#
#.............#..................................S.#
####################################################)";

const std::string WALL_TEST = R"(
..................
......#...........
.....###..........
......#...........
..................
##...###...##.....
#.....#.....#.....
....@.............
#...........#.....
#...........#.....
..................
#...........#.....
#...........#.....
#...........#.....
..................
#.....#.....#.....
##...###...##.....
..................
#######...####....
#.....#.....#.....
#.....#.....#.....
#######...###.....
............#.....
............#.....
..#.#.............
..#.#......#......
###.###..#####....
..#.#.............
..#.#....#####....
...........#......
..................)";

namespace bitsery {
template<typename S>
void serialize(S& s, vec3& data) {
    s.value4b(data.x);
    s.value4b(data.y);
    s.value4b(data.z);
}

template<typename S>
void serialize(S& s, Color& data) {
    s.value1b(data.r);
    s.value1b(data.g);
    s.value1b(data.b);
    s.value1b(data.a);
}

template<typename S>
void serialize(S& s, std::shared_ptr<Item>& item) {
    s.ext(item, bitsery::ext::StdSmartPtr{});
}

template<typename S>
void serialize(S& s, std::shared_ptr<Entity>& entity) {
    s.ext(entity, bitsery::ext::StdSmartPtr{});
}

namespace ext {
template<>
struct PolymorphicBaseClass<Entity>
    : PolymorphicDerivedClasses<Furniture, Person> {};

template<>
struct PolymorphicBaseClass<Furniture>
    : PolymorphicDerivedClasses<Wall, Table, Register, Conveyer, Grabber,
                                BagBox, MedicineCabinet, CharacterSwitcher> {};

template<>
struct PolymorphicBaseClass<Person> : PolymorphicDerivedClasses<AIPerson> {};

template<>
struct PolymorphicBaseClass<AIPerson> : PolymorphicDerivedClasses<Customer> {};

template<>
struct PolymorphicBaseClass<Item>
    : PolymorphicDerivedClasses<Bag, PillBottle, Pill> {};

template<>
struct PolymorphicBaseClass<LevelInfo>
    : PolymorphicDerivedClasses<LobbyMapInfo, GameMapInfo> {};

}  // namespace ext
}  // namespace bitsery

using MyPolymorphicClasses = bitsery::ext::PolymorphicClassesList<Entity, Item>;

struct Map {
    LobbyMapInfo lobby_info;
    GameMapInfo game_info;

    std::vector<std::shared_ptr<RemotePlayer>> remote_players_NOT_SERIALIZED;

    Map(const std::string& _seed = "default_seed") { update_seed(_seed); }

    void update_seed(const std::string& s) { game_info.update_seed(s); }

    Items items() const {
        return in_lobby_state() ? lobby_info.items : game_info.items;
    }

    Entities entities() const {
        return in_lobby_state() ? lobby_info.entities : game_info.entities;
    }

    void onUpdate(float dt) {
        TRACY_ZONE_SCOPED;
        for (auto rp : remote_players_NOT_SERIALIZED) {
            rp->update(dt);
        }
        if (in_lobby_state()) {
            lobby_info.onUpdate(dt);
        } else {
            game_info.onUpdate(dt);
        }
    }

    void onDraw(float dt) const {
        TRACY_ZONE_SCOPED;
        for (auto rp : remote_players_NOT_SERIALIZED) {
            if (rp) rp->render();
            if (!rp) log_warn("we have invalid remote players");
        }
        if (in_lobby_state()) {
            lobby_info.onDraw(dt);
        } else {
            game_info.onDraw(dt);
        }
    }

    void onDrawUI(float dt) {
        TRACY_ZONE_SCOPED;
        if (in_lobby_state()) {
            lobby_info.onDrawUI(dt);
        } else {
            game_info.onDrawUI(dt);
        }
    }

    [[nodiscard]] bool in_lobby_state() const {
        return GameState::get().is(game::State::Lobby) ||
               GameState::get().is_paused_in(game::State::Lobby);
    }

    // These are called before every "send_map_state" when server
    // sends everything over to clients
    void ensure_generated_map() { game_info.ensure_generated_map(); }
    void grab_things() {
        if (in_lobby_state()) {
            lobby_info.grab_things();
        } else {
            game_info.grab_things();
        }
    }

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.object(lobby_info);
        s.object(game_info);
    }
};
