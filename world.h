#pragma once

#include "entity.h"
#include "external_include.h"
#include "furniture.h"
#include "people.h"

struct World {
    World() {
        std::shared_ptr<AIPerson> aiperson;
        aiperson.reset(new AIPerson((vec3){-TILESIZE, 0.0f, -TILESIZE},
                                    (Color){255, 0, 0, 255}));

        std::shared_ptr<Player> player;
        player.reset(new Player());

        GLOBALS.set("player", player.get());

        std::shared_ptr<Wall> wall;
        wall.reset(new Wall((vec3){-TILESIZE * 2, 0.0f, -TILESIZE * 2},
                            (Color){155, 75, 0, 255}));

        EntityHelper::addEntity(aiperson);
        EntityHelper::addEntity(player);
        EntityHelper::addEntity(wall);
    }
};
