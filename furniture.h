
#include "entity.h"
#include "external_include.h"

struct Wall : public Entity {
    Wall(vec3 p, Color c) : Entity(p, c) {}
    Wall(vec2 p, Color c) : Entity(p, c) {}

    void update(float) override {
    }
};
