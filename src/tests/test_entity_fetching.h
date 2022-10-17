
#pragma once

#include "../assert.h"
#include "../external_include.h"
//
#include "../entity.h"
#include "../entityhelper.h"

namespace tests {

struct NotMe : public Entity {
    NotMe(vec2 p, Color c) : Entity(p, c) {}
};
struct Me : public Entity {
    Me(vec2 p, Color c) : Entity(p, c) {}
};

void teardown() { entities_DO_NOT_USE.clear(); }

std::shared_ptr<Entity> mk_ent(vec2 v) {
    std::shared_ptr<Entity> entity(new Entity(v, WHITE));
    return entity;
}

std::shared_ptr<NotMe> mk_nm(vec2 v) {
    std::shared_ptr<NotMe> entity(new NotMe(v, WHITE));
    return entity;
}

std::shared_ptr<Me> mk_me(vec2 v) {
    std::shared_ptr<Me> entity(new Me(v, WHITE));
    return entity;
}

void test_loop() {
    int i = 0;
    EntityHelper::addEntity(mk_ent(vec2{0, 0}));
    EntityHelper::forEachEntity([&](auto&&) {
        i++;
        return EntityHelper::ForEachFlow::Continue;
    });

    M_TEST_EQ(i, 1, "there should be one entity");

    i = 0;
    EntityHelper::addEntity(mk_ent(vec2{0, 0}));
    EntityHelper::forEachEntity([&](auto&&) {
        i++;
        return EntityHelper::ForEachFlow::Continue;
    });

    M_TEST_EQ(i, 2, "there should be two entity");
}

void test_get_in_range() {
    int i = 0;
    EntityHelper::addEntity(mk_ent(vec2{0, 0}));
    EntityHelper::addEntity(mk_ent(vec2{1, 1}));
    EntityHelper::addEntity(mk_ent(vec2{2, 2}));
    EntityHelper::addEntity(mk_ent(vec2{3, 3}));
    EntityHelper::addEntity(mk_ent(vec2{4, 4}));

    EntityHelper::forEachEntity([&](auto&&) {
        i++;
        return EntityHelper::ForEachFlow::Continue;
    });
    M_TEST_EQ(i, 5, "there should be 5 entity");

    auto results = EntityHelper::getEntitiesInRange<Entity>(vec2{0, 0}, 1);
    M_TEST_EQ(results.size(), 1, "");

    results = EntityHelper::getEntitiesInRange<Entity>(vec2{0, 0}, 3);
    M_TEST_EQ(results.size(), 3, "");

    results = EntityHelper::getEntitiesInRange<Entity>(vec2{0, 0}, 10);
    M_TEST_EQ(results.size(), 5, "");
}

void test_get_tile_in_front() {
    // me
    std::shared_ptr<Me> me = mk_me(vec2{0, 0});
    EntityHelper::addEntity(me);

    // Facing forward

    auto test_dir = [&](Entity::FrontFaceDirection dir, int i, int j) {
        me->face_direction = dir;

        int step = 0;
        while (step < 10) {
            int a = step * i;
            int b = step * j;
            auto fwd_ = me->tile_infront(step);
            M_TEST_EQ(fwd_.x, me->position.x + a, "");
            M_TEST_EQ(fwd_.y, me->position.y + b, "");
            step += 1;
        }
    };

    test_dir(Entity::FrontFaceDirection::FORWARD, 0, 1);
    test_dir(Entity::FrontFaceDirection::BACK, 0, -1);

    test_dir(Entity::FrontFaceDirection::LEFT , 1, 0);
    test_dir(Entity::FrontFaceDirection::RIGHT , -1, 0);

    test_dir(Entity::FrontFaceDirection::NW, 1, 1);
    test_dir(Entity::FrontFaceDirection::NE, -1, 1);

    test_dir(Entity::FrontFaceDirection::SW, -1, -1);
    test_dir(Entity::FrontFaceDirection::SE, 1,-1);
}

void test_get_matching_in_front() {
    // me
    std::shared_ptr<Me> me = mk_me(vec2{0, 0});
    EntityHelper::addEntity(me);

    int mx_range = 5;
    for (int i = -mx_range; i < mx_range; i++) {
        for (int j = -mx_range; j < mx_range; j++) {
            EntityHelper::addEntity(mk_nm(vec2{i * 1.f, j * 1.f}));
        }
    }
    M_TEST_EQ(entities_DO_NOT_USE.size(), 101, "there should be 101 entity");


    // TODO finish
}

void test_entity_fetching() {
    std::array<std::function<void(void)>, 4> fns = {
        test_loop,
        test_get_in_range,
        test_get_tile_in_front,
        test_get_matching_in_front,
    };

    for (auto fn : fns) {
        fn();
        teardown();
    }
}
}  // namespace tests
