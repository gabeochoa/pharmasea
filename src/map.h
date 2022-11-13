

#pragma once

#include <bitsery/ext/inheritance.h>
#include <bitsery/ext/pointer.h>
#include <bitsery/ext/std_smart_ptr.h>

#include <memory>

#include "external_include.h"
//
#include "entity.h"
#include "furnitures.h"
#include "remote_player.h"
#include "ui_color.h"

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
struct PolymorphicBaseClass<Entity> : PolymorphicDerivedClasses<Furniture> {};

template<>
struct PolymorphicBaseClass<Furniture>
    : PolymorphicDerivedClasses<Wall, Table> {};

template<>
struct PolymorphicBaseClass<Item> : PolymorphicDerivedClasses<Bag> {};

}  // namespace ext
}  // namespace bitsery

const int MAX_MAP_WIDTH = 20;
const int MAX_MAP_HEIGHT = 20;
const int MAX_SEED_LENGTH = 20;

using MyPolymorphicClasses = bitsery::ext::PolymorphicClassesList<Entity, Item>;

struct Map {
    bool was_generated = false;

    std::string seed;
    Entities entities;
    Entities::size_type num_entities;

    Items items;
    Items::size_type num_items;

    std::vector<std::shared_ptr<RemotePlayer>> remote_players_NOT_SERIALIZED;

    Map(const std::string& _seed = "default") : seed(_seed) {}

    void merge(const Map& other) { seed = other.seed; }

    void onUpdate(float dt) {
        for (auto e : EntityHelper::get_entities()) {
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

        for (auto i : items) {
            i->render();
        }
        for (auto rp : remote_players_NOT_SERIALIZED) {
            rp->render();
        }
    }

    void ensure_generated_map() {
        if (was_generated) return;
        was_generated = true;
        generate_map();
    }

    void grab_things() {
        {
            entities.clear();
            auto es = EntityHelper::get_entities();
            this->entities = es;
            num_entities = this->entities.size();
        }

        {
            items.clear();
            auto is = ItemHelper::get_items();
            this->items = is;
            num_items = this->items.size();
        }
    }

   private:
    void generate_map() {
        generate_walls();

        {
            std::shared_ptr<Table> table;
            auto location = vec2{10 * TILESIZE, 10 * TILESIZE};
            table.reset(new Table(location));
            EntityHelper::addEntity(table);

            std::shared_ptr<Item> item;
            item.reset(new Bag(location, (Color){255, 15, 240, 255}));
            ItemHelper::addItem(item);

            table->held_item = item;
        }
    }
    void generate_walls() {
        auto d_color = (Color){155, 75, 0, 255};
        for (int i = 0; i < MAX_MAP_WIDTH; i++) {
            for (int j = 0; j < MAX_MAP_HEIGHT; j++) {
                if ((i == 0 && j == 0) || (i == 0 && j == 1)) continue;
                if (i == 0 || j == 0 || i == MAX_MAP_WIDTH - 1 ||
                    j == MAX_MAP_HEIGHT - 1) {
                    vec2 location = vec2{i * TILESIZE, j * TILESIZE};
                    std::shared_ptr<Wall> wall;
                    wall.reset(new Wall(location, d_color));
                    EntityHelper::addEntity(wall);
                }
            }
        }
    }

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.text1b(seed, MAX_SEED_LENGTH);
        s.value8b(num_entities);
        s.container(entities, num_entities,
                    [](S& s2, std::shared_ptr<Entity>& entity) {
                        s2.ext(entity, bitsery::ext::StdSmartPtr{});
                    });

        s.value8b(num_items);
        s.container(items, num_items, [](S& s2, std::shared_ptr<Item>& item) {
            s2.ext(item, bitsery::ext::StdSmartPtr{});
        });
    }
};
