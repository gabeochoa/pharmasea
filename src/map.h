

#pragma once

#include <bitsery/ext/inheritance.h>
#include <bitsery/ext/pointer.h>
#include <bitsery/ext/std_smart_ptr.h>

#include <memory>
#include <random>

#include "external_include.h"
//
#include "entity.h"
#include "furnitures.h"
#include "random.h"
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
    : PolymorphicDerivedClasses<Wall, Table, Register, Conveyer, Grabber> {};

template<>
struct PolymorphicBaseClass<Person> : PolymorphicDerivedClasses<AIPerson> {};

template<>
struct PolymorphicBaseClass<AIPerson> : PolymorphicDerivedClasses<Customer> {};

template<>
struct PolymorphicBaseClass<Item> : PolymorphicDerivedClasses<Bag> {};

}  // namespace ext
}  // namespace bitsery

constexpr int MAX_MAP_SIZE = 20;
constexpr int MAX_SEED_LENGTH = 20;

using MyPolymorphicClasses = bitsery::ext::PolymorphicClassesList<Entity, Item>;

struct Map {
    bool was_generated = false;

    //
    std::string seed;
    size_t hashed_seed;
    std::mt19937 generator;
    std::uniform_int_distribution<> dist;
    //

    Entities entities;
    Entities::size_type num_entities;

    Items items;
    Items::size_type num_items;

    std::vector<std::shared_ptr<RemotePlayer>> remote_players_NOT_SERIALIZED;

    Map(const std::string& _seed = "default") { update_seed(_seed); }

    void update_seed(const std::string& s) {
        seed = s;
        hashed_seed = hashString(seed);
        generator = make_engine(hashed_seed);
        // TODO leaving at 1 because we dont have a door to block the entrance
        dist = std::uniform_int_distribution<>(1, MAX_MAP_SIZE - 1);
    }

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
            if (e) e->render();
            if (!e) std::cout << "we have invalid entities" << std::endl;
        }

        for (auto i : items) {
            if (i) i->render();
            if (!i) std::cout << "we have invalid items" << std::endl;
        }
        for (auto rp : remote_players_NOT_SERIALIZED) {
            if (rp) rp->render();
            if (!rp) std::cout << "we have invalid remote players" << std::endl;
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
            EntityHelper::cleanup();
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
    auto get_rand_walkable() {
        vec2 location;
        do {
            location =
                vec2{dist(generator) * TILESIZE, dist(generator) * TILESIZE};
        } while (!EntityHelper::isWalkable(location));
        return location;
    }

    void generate_map() {
        auto generate_tables = [this]() {
            std::shared_ptr<Table> table;
            auto location = get_rand_walkable();
            table.reset(new Table(location));
            EntityHelper::addEntity(table);

            std::shared_ptr<Item> item;
            item.reset(new Bag(location, (Color){255, 15, 240, 255}));
            ItemHelper::addItem(item);

            table->held_item = item;
        };

        auto generate_register = [this]() {
            std::shared_ptr<Register> reg;
            auto location = get_rand_walkable();
            reg.reset(new Register(location));
            EntityHelper::addEntity(reg);
        };

        // TODO replace with a CustomerSpawner eventually
        auto generate_customer = []() {
            auto location = vec2{-10 * TILESIZE, -10 * TILESIZE};
            std::shared_ptr<Customer> customer;
            customer.reset(new Customer(location, RED));
            EntityHelper::addEntity(customer);
        };

        auto generate_test = [this]() {
            for (int i = 0; i < 5; i++) {
                auto location = get_rand_walkable();
                std::shared_ptr<Conveyer> conveyer;
                conveyer.reset(new Grabber(location));
                EntityHelper::addEntity(conveyer);
            }
        };

        generate_walls();
        generate_tables();
        generate_register();
        generate_customer();
        generate_test();
    }

    void generate_walls() {
        auto d_color = (Color){155, 75, 0, 255};
        for (int i = 0; i < MAX_MAP_SIZE; i++) {
            for (int j = 0; j < MAX_MAP_SIZE; j++) {
                if ((i == 0 && j == 0) || (i == 0 && j == 1)) continue;
                if (i == 0 || j == 0 || i == MAX_MAP_SIZE - 1 ||
                    j == MAX_MAP_SIZE - 1) {
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
