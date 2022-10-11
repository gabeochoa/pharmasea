#pragma once

#include "entity.h"
#include "external_include.h"
#include "furniture.h"
#include "globals.h"
#include "people.h"
#include "util.h"
#include "vec_util.h"

const std::string EXAMPLE_MAP = R"(
####################################################
#..................................................#
#..................................................#
#........@....I....................................#
#............0.....................................#
#........#.........................................#
#..................................................#
#..................................................#
#..................................................#
#..................................................#
#..................................................#
#########.###############.##########.###############
#..................................................#
#..................................................#
#.......T..............................A...........#
#..................................................#
#..................................................#
#..................................................#
#..................................................#
#..................................................#
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

//const std::string ACTIVE_MAP = WALL_TEST;
const std::string ACTIVE_MAP = EXAMPLE_MAP;

struct World {
    std::vector<std::string> lines;
    World() {
        this->lines = util::split_string(ACTIVE_MAP, "\n");
        this->init_map();
    }

   private:
    vec2 find_origin() {
        vec2 origin{0.0, 0.0};
        for (int i = 0; i < (int) lines.size(); i++) {
            auto line = lines[i];
            for (int j = 0; j < (int) line.size(); j++) {
                auto ch = line[j];
                if (ch == '0') {
                    origin = vec2{j * TILESIZE, i * TILESIZE};
                    break;
                }
            }
        }
        return origin;
    }

    char get_char(int i, int j) {
        if (i < 0) return '.';
        if (j < 0) return '.';
        if (i >= (int) lines.size()) return '.';
        if (j >= (int) lines[i].size()) return '.';
        return lines[i][j];
    }

    std::vector<char> get_neighbors(int i, int j) {
        /*
            0 1 2
            3 . 4
            5 6 7
        */
        std::vector<char> output;
        const int x[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
        const int y[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
        for (int a = 0; a < 8; a++) {
            char neighbor = get_char(i + x[a], j + y[a]);
            output.push_back(neighbor);
        }
        return output;
    }

    void init_wall(int i, int j, vec2 loc) {
        auto d_color = (Color){155, 75, 0, 255};
        const auto create_wall = [](const vec2& loc, Color c,
                                    const Wall::Type& type = Wall::Type::FULL,
                                    const Entity::FrontFaceDirection direction =
                                        Entity::FrontFaceDirection::FORWARD) {
            std::shared_ptr<Wall> wall;
            wall.reset(new Wall(loc, c));
            wall->type = type;
            wall->face_direction = direction;
            EntityHelper::addEntity(wall);
        };

        auto nbs = get_neighbors(i, j);
        auto nb = [&](int i, char c) {
            if (c == '*') return true;
            return nbs[i] == c;
        };
        /*
            0 1 2
            3 x 4
            5 6 7
        */

        if (                                            //
            (nb(0, '.') && nb(1, '.') && nb(2, '.') &&  //
             nb(3, '#') /*         */ && nb(4, '#') &&  //
             nb(5, '.') && nb(6, '#') && nb(7, '.')     //
             ) ||
            (nb(0, '.') && nb(1, '#') && nb(2, '.') &&  //
             nb(3, '#') /*         */ && nb(4, '#') &&  //
             nb(5, '.') && nb(6, '.') && nb(7, '.')     //
             ) ||
            (nb(0, '.') && nb(1, '#') && nb(2, '.') &&  //
             nb(3, '.') /*         */ && nb(4, '#') &&  //
             nb(5, '.') && nb(6, '#') && nb(7, '.')     //
             ) ||
            (nb(0, '.') && nb(1, '#') && nb(2, '.') &&  //
             nb(3, '#') /*         */ && nb(4, '.') &&  //
             nb(5, '.') && nb(6, '#') && nb(7, '.')     //
             ) ||
            false) {
            create_wall(loc, d_color, WallType::TEE, EntityDir::RIGHT);
            return;
        }
        if (                                            //
            (nb(0, '.') && nb(1, '#') && nb(2, '.') &&  //
             nb(3, '#') /*         */ && nb(4, '#') &&  //
             nb(5, '.') && nb(6, '#') && nb(7, '.')     //
             )                                          //
        ) {
            create_wall(loc, d_color, WallType::DOUBLE_TEE, EntityDir::FORWARD);
            return;
        }
        if (                                            //
            (nb(0, '*') && nb(1, '#') && nb(2, '*') &&  //
             nb(3, '#') /*         */ && nb(4, '#') &&  //
             nb(5, '*') && nb(6, '#') && nb(7, '*')     //
             )                                          //
        ) {
            create_wall(loc, d_color, WallType::FULL, EntityDir::FORWARD);
            return;
        }

        if (                                            //
            (nb(0, '#') && nb(1, '.') && nb(2, '.') &&  //
             nb(3, '#') /*         */ && nb(4, '.') &&  //
             nb(5, '*') && nb(6, '.') && nb(7, '.')     //
             ) ||
            (nb(0, '*') && nb(1, '.') && nb(2, '.') &&  //
             nb(3, '#') /*         */ && nb(4, '.') &&  //
             nb(5, '#') && nb(6, '.') && nb(7, '.')     //
             ) ||
            (nb(0, '.') && nb(1, '.') && nb(2, '#') &&  //
             nb(3, '.') /*         */ && nb(4, '#') &&  //
             nb(5, '.') && nb(6, '.') && nb(7, '*')     //
             ) ||
            (nb(0, '.') && nb(1, '.') && nb(2, '*') &&  //
             nb(3, '.') /*         */ && nb(4, '#') &&  //
             nb(5, '.') && nb(6, '.') && nb(7, '#')     //
             ) ||
            false) {
            create_wall(loc, d_color, WallType::HALF, EntityDir::RIGHT);
            return;
        }

        if (                                            //
            (nb(0, '.') && nb(1, '.') && nb(2, '.') &&  //
             nb(3, '.') /*         */ && nb(4, '.') &&  //
             nb(5, '*') && nb(6, '#') && nb(7, '#')     //
             ) ||
            (nb(0, '.') && nb(1, '.') && nb(2, '.') &&  //
             nb(3, '.') /*         */ && nb(4, '.') &&  //
             nb(5, '#') && nb(6, '#') && nb(7, '*')     //
             ) ||
            (nb(0, '*') && nb(1, '#') && nb(2, '#') &&  //
             nb(3, '.') /*         */ && nb(4, '.') &&  //
             nb(5, '.') && nb(6, '.') && nb(7, '.')     //
             ) ||
            (nb(0, '#') && nb(1, '#') && nb(2, '*') &&  //
             nb(3, '.') /*         */ && nb(4, '.') &&  //
             nb(5, '.') && nb(6, '.') && nb(7, '.')     //
             ) ||
            false) {
            create_wall(loc, d_color, WallType::HALF, EntityDir::FORWARD);
            return;
        }

        if ((nb(0, '.') && nb(1, '.') && nb(2, '.') &&  //
             nb(3, '.') /*         */ && nb(4, '#') &&  //
             nb(5, '.') && nb(6, '.') && nb(7, '.')     //
             ) ||
            (nb(0, '.') && nb(1, '.') && nb(2, '.') &&  //
             nb(3, '#') /*         */ && nb(4, '.') &&  //
             nb(5, '.') && nb(6, '.') && nb(7, '.')     //
             ) ||                                       //
            (nb(0, '.') && nb(1, '.') && nb(2, '.') &&  //
             nb(3, '#') /*         */ && nb(4, '#') &&  //
             nb(5, '.') && nb(6, '.') && nb(7, '.')     //
             ) ||                                       //
            (nb(0, '.') && nb(1, '.') && nb(2, '.') &&  //
             nb(3, '#') /*         */ && nb(4, '#') &&  //
             nb(5, '.') && nb(6, '.') && nb(7, '#')     //
             ) ||                                       //
            (nb(0, '.') && nb(1, '.') && nb(2, '#') &&  //
             nb(3, '#') /*         */ && nb(4, '#') &&  //
             nb(5, '.') && nb(6, '.') && nb(7, '.')     //
             ) ||                                       //
            (nb(0, '#') && nb(1, '.') && nb(2, '.') &&  //
             nb(3, '#') /*         */ && nb(4, '#') &&  //
             nb(5, '.') && nb(6, '.') && nb(7, '.')     //
             ) ||                                       //
            (nb(0, '.') && nb(1, '.') && nb(2, '.') &&  //
             nb(3, '#') /*         */ && nb(4, '#') &&  //
             nb(5, '#') && nb(6, '.') && nb(7, '.')     //
             ) ||                                       //
            (nb(0, '.') && nb(1, '.') && nb(2, '#') &&  //
             nb(3, '#') /*         */ && nb(4, '#') &&  //
             nb(5, '.') && nb(6, '.') && nb(7, '#')     //
             ) ||                                       //
            (nb(0, '#') && nb(1, '.') && nb(2, '.') &&  //
             nb(3, '#') /*         */ && nb(4, '#') &&  //
             nb(5, '#') && nb(6, '.') && nb(7, '.')     //
             ) ||                                       //
            false                                       //
        ) {
            create_wall(loc, d_color, WallType::HALF, EntityDir::RIGHT);
            return;
        }


        if ((nb(0, '.') && nb(1, '#') && nb(2, '.') &&  //
             nb(3, '.') /*         */ && nb(4, '.') &&  //
             nb(5, '.') && nb(6, '.') && nb(7, '.')     //
             ) ||

            (nb(0, '.') && nb(1, '.') && nb(2, '.') &&  //
             nb(3, '.') /*         */ && nb(4, '.') &&  //
             nb(5, '.') && nb(6, '#') && nb(7, '.')     //
             ) ||                                       //
            (nb(0, '.') && nb(1, '#') && nb(2, '.') &&  //
             nb(3, '.') /*         */ && nb(4, '.') &&  //
             nb(5, '.') && nb(6, '#') && nb(7, '.')     //
             ) ||                                       //
            (nb(0, '.') && nb(1, '#') && nb(2, '.') &&  //
             nb(3, '.') /*         */ && nb(4, '.') &&  //
             nb(5, '.') && nb(6, '#') && nb(7, '#')     //
             ) ||                                       //
            (nb(0, '.') && nb(1, '#') && nb(2, '.') &&  //
             nb(3, '.') /*         */ && nb(4, '.') &&  //
             nb(5, '#') && nb(6, '#') && nb(7, '.')     //
             ) ||                                       //
            (nb(0, '#') && nb(1, '#') && nb(2, '*') &&  //
             nb(3, '.') /*         */ && nb(4, '.') &&  //
             nb(5, '.') && nb(6, '#') && nb(7, '.')     //
             ) ||                                       //
            (nb(0, '*') && nb(1, '#') && nb(2, '#') &&  //
             nb(3, '.') /*         */ && nb(4, '.') &&  //
             nb(5, '.') && nb(6, '#') && nb(7, '.')     //
             ) ||                                       //
            false                                       //
        ) {
            create_wall(loc, d_color, WallType::HALF, EntityDir::FORWARD);
            return;
        }


        // Corners
        if ((nb(0, '.') && nb(1, '#') && nb(2, '.') &&  //
             nb(3, '#') /*         */ && nb(4, '.') &&  //
             nb(5, '.') && nb(6, '.') && nb(7, '.')     //
             ) ||

            (nb(0, '.') && nb(1, '.') && nb(2, '.') &&  //
             nb(3, '.') /*         */ && nb(4, '#') &&  //
             nb(5, '.') && nb(6, '#') && nb(7, '.')     //
             ) ||
            (nb(0, '.') && nb(1, '#') && nb(2, '.') &&  //
             nb(3, '.') /*         */ && nb(4, '#') &&  //
             nb(5, '.') && nb(6, '.') && nb(7, '.')     //
             ) ||

            (nb(0, '.') && nb(1, '.') && nb(2, '.') &&  //
             nb(3, '#') /*         */ && nb(4, '.') &&  //
             nb(5, '.') && nb(6, '#') && nb(7, '.')     //
             )) {
            create_wall(loc, d_color, WallType::CORNER, EntityDir::BACK);
            return;
        }

        // Surrounded by nothing
        if (nb(0, '.') && nb(1, '.') && nb(2, '.') &&  //
            nb(3, '.') /*         */ && nb(4, '.') &&  //
            nb(5, '.') && nb(6, '.') && nb(7, '.')     //
        ) {
            create_wall(loc, d_color);
            return;
        }

        // Not setup yet
        create_wall(loc, BLUE);

        /*
           ...
           .ax  => put two half walls | and --
           .x

           ...
           xax => put three half walls -- and | and --
           .x.

           ...
           .a. => put a full wall
           ...

          */
    }

    void init_map() {
        vec2 origin = find_origin();
        for (int i = 0; i < (int) this->lines.size(); i++) {
            auto line = this->lines[i];
            for (int j = 0; j < (int) line.size(); j++) {
                vec2 raw_location = vec2{i * TILESIZE, j * TILESIZE};
                vec2 location = raw_location - origin;
                auto ch = get_char(i, j);
                switch (ch) {
                    case '#': {
                        init_wall(i, j, location);
                        break;
                    }
                    case '@': {
                        if (GLOBALS.contains("player")) {
                            std::cout << "WorldGen: "
                                      << "cant have two players (yet) "
                                      << std::endl;
                            break;
                        }
                        std::shared_ptr<Player> player;
                        player.reset(new Player(location));
                        GLOBALS.set("player", player.get());
                        EntityHelper::addEntity(player);
                        break;
                    }
                    case 'A': {
                        std::shared_ptr<AIPerson> aiperson;
                        aiperson.reset(new AIPerson(location,
                                                    (Color){255, 0, 0, 255},
                                                    {0, 255, 0, 255}));
                        EntityHelper::addEntity(aiperson);
                        break;
                    }
                    case 'T': {
                        if (GLOBALS.contains("targetcube")) {
                            std::cout << "WorldGen: "
                                      << "cant have two targetcubes (yet) "
                                      << std::endl;
                            break;
                        }
                        std::shared_ptr<TargetCube> targetcube;
                        targetcube.reset(
                            new TargetCube(location, (Color){255, 16, 240, 255},
                                           (Color){16, 255, 240, 255}));
                        EntityHelper::addEntity(targetcube);
                        GLOBALS.set("targetcube", targetcube.get());
                        break;
                    }
                    case 'I': {
                        std::shared_ptr<Item> item;
                        item.reset(
                            new Item(location, (Color){255, 16, 240, 255}));
                        EntityHelper::addItem(item);
                        break;
                    }
                    case '.':
                    default:
                        break;
                }
            }
        }
    }
};
