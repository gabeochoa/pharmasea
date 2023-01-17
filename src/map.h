

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
                                BagBox, MedicineCabinet> {};

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

//
// World represents all the pieces of the world that are serializable
// anything non serializable can go into Map
//
struct World {
    std::array<LevelInfo, 2> levels = {
        LevelInfo(),
        LevelInfo(),
    };
    int active_level = 0;

    void update_client_static() {
        client_entities_DO_NOT_USE = levels[active_level].entities;
        client_items_DO_NOT_USE = levels[active_level].items;
    }

    void init(const std::string& seed) {
        levels[0].type = LevelInfo::Type::Lobby;
        levels[1].type = LevelInfo::Type::Game;

        update_seed(seed);
    }

    void update_seed(const std::string& s) {
        levels[0].update_seed(s);
        levels[1].update_seed(s);
    }

    void onUpdate(float dt) {
        TRACY_ZONE_SCOPED;
        levels[active_level].onUpdate(dt);
    }

    void onDraw(float dt) const {
        TRACY_ZONE_SCOPED;
        levels[active_level].onDraw(dt);
    }

    void onDrawUI(float dt) {
        TRACY_ZONE_SCOPED;
        levels[active_level].onDrawUI(dt);
    }

    [[nodiscard]] bool in_lobby_state() const {
        return GameState::get().is(game::State::Lobby) ||
               GameState::get().is_paused_in(game::State::Lobby);
    }

    // These are called before every "send_map_state" when server
    // sends everything over to clients
    void ensure_generated_map() { levels[active_level].ensure_generated_map(); }
    void grab_things() { levels[active_level].grab_things(); }

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.container(levels);
        s.value4b(active_level);
    }
};

struct Map {
    World world;
    std::vector<std::shared_ptr<RemotePlayer>> remote_players_NOT_SERIALIZED;

    Map() {}

    void init(const std::string& seed) { world.init(seed); }

    void onUpdate(float dt) {
        TRACY_ZONE_SCOPED;
        for (auto rp : remote_players_NOT_SERIALIZED) {
            rp->update(dt);
        }
        world.onUpdate(dt);
    }

    void onDraw(float dt) const {
        TRACY_ZONE_SCOPED;
        for (auto rp : remote_players_NOT_SERIALIZED) {
            if (rp) rp->render();
            if (!rp) log_warn("we have invalid remote players");
        }
        world.onDraw(dt);
    }

    void grab_things() { world.grab_things(); }
    void ensure_generated_map() { world.ensure_generated_map(); }

    void onDrawUI(float dt) {
        TRACY_ZONE_SCOPED;
        world.onDrawUI(dt);
    }
};
