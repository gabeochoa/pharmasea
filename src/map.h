

#pragma once

#include <bitsery/ext/inheritance.h>
#include <bitsery/ext/pointer.h>
#include <bitsery/ext/std_smart_ptr.h>

#include <memory>

#include "external_include.h"
//
#include "entity.h"
#include "furniture/wall.h"
#include "remote_player.h"

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
}

namespace ext {
template<>
struct PolymorphicBaseClass<Entity> : PolymorphicDerivedClasses<Furniture> {};

template<>
struct PolymorphicBaseClass<Furniture> : PolymorphicDerivedClasses<Wall> {};

}  // namespace ext
}  // namespace bitsery

const int MAX_MAP_WIDTH = 20;
const int MAX_MAP_HEIGHT = 20;
const int MAX_SEED_LENGTH = 20;

using PolymorphicEntityClasses = bitsery::ext::PolymorphicClassesList<Entity>;

struct Map {
    std::string seed;
    std::vector<std::shared_ptr<Entity>> entities;

    std::vector<std::shared_ptr<RemotePlayer>> remote_players_NOT_SERIALIZED;

    Map(const std::string& _seed = "default") : seed(_seed) {
        generate_walls();
    }

    void merge(const Map& other) {
        seed = other.seed;
        this->entities = other.entities;
    }

    void onUpdate(float dt) {
        for (auto e : entities) {
            e->update(dt);
        }
        for (auto rp : remote_players_NOT_SERIALIZED) {
            rp->update(dt);
        }
    }

    void onDraw(float) {
        for (auto e : entities) {
            e->render();
        }
        for (auto rp : remote_players_NOT_SERIALIZED) {
            rp->render();
        }
    }

   private:
    void generate_walls() {
        auto d_color = (Color){155, 75, 0, 255};
        for (int i = 1; i < MAX_MAP_WIDTH; i++) {
            for (int j = 1; j < MAX_MAP_HEIGHT; j++) {
                if (i == 1 || j == 1 || i == MAX_MAP_WIDTH - 1 ||
                    i == MAX_MAP_HEIGHT - 1) {
                    vec2 location = vec2{i * TILESIZE, j * TILESIZE};
                    std::shared_ptr<Wall> wall;
                    wall.reset(new Wall(location, d_color));
                    entities.push_back(wall);
                }
            }
        }
    }

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.text1b(seed, MAX_SEED_LENGTH);
        s.container(entities, entities.size(),
                    [](S& s2, std::shared_ptr<Entity>& entity) {
                        s2.ext(entity, bitsery::ext::StdSmartPtr{});
                    });
    }
};
