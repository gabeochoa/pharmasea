
#include "external_include.h"
#include "entity.h"

struct Wall : public Entity {
    Wall(vec3 p, Color c) : Entity(p, c) {}
};
