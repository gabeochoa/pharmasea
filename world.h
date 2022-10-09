#pragma once

#include "entity.h"
#include "external_include.h"
#include "furniture.h"
#include "globals.h"
#include "people.h"
#include "util.h"
#include "vec_util.h"

const std::string EXAMPLE_MAP = R"(
    #####################################################################
    #...................................................................#
    #...................................................................#
    #........@..........................................................#
    #...................................................................#
    #...................................................................#
    #...................................................................#
    #...................................................................#
    #...............................0...................................#
    #...................................................................#
    #...................................................................#
    #########.###############.###############.###########################
    #...................................................................#
    #...................................................................#
    #...........................................A.......................#
    #...................................................................#
    #...................................................................#
    #.............A.............................A.......................#
    #...................................................................#
    #...................................................................#
    #####################################################################)";

struct World {
    World() { this->init_map(); }

   private:
    vec2 find_origin(const std::vector<std::string>& lines) {
        vec2 origin;
        for (int i = 0; i < (int)lines.size(); i++) {
            auto line = lines[i];
            for (int j = 0; j < (int)line.size(); j++) {
                auto ch = line[j];
                if (ch == '0') {
                    origin = vec2{i * TILESIZE, j * TILESIZE};
                    break;
                }
            }
        }
        return origin;
    }

    void init_map() {
        auto lines = util::split_string(EXAMPLE_MAP, "\n");

        vec2 origin = find_origin(lines);

        bool placed_player = false;
        for (int i = 0; i < (int)lines.size(); i++) {
            auto line = lines[i];
            for (int j = 0; j < (int)line.size(); j++) {
                vec2 raw_location = vec2{i * TILESIZE, j * TILESIZE};
                vec2 location = raw_location - origin;
                auto ch = line[j];
                switch (ch) {
                    case '#': {
                        std::shared_ptr<Wall> wall;
                        wall.reset(
                            new Wall(location, (Color){155, 75, 0, 255}));
                        EntityHelper::addEntity(wall);
                        break;
                    }
                    case '@': {
                        if (placed_player) {
                            std::cout << "WorldGen: "
                                      << "cant have two players (yet) "
                                      << std::endl;
                            break;
                        }
                        std::shared_ptr<Player> player;
                        player.reset(new Player(location));
                        GLOBALS.set("player", player.get());
                        EntityHelper::addEntity(player);
                        placed_player = true;
                        break;
                    }
                    case 'A': {
                        std::shared_ptr<AIPerson> aiperson;
                        aiperson.reset(
                            new AIPerson(location, (Color){255, 0, 0, 255}));

                        EntityHelper::addEntity(aiperson);
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
