

#include "../entity.h"
//
#include "../engine/defer.h"
#include "../network/shared.h"

void compare_and_validate_components(Entity* a, Entity* b) {
    M_ASSERT(a->componentSet == b->componentSet, "component sets should match");

    int i = 0;
    while (i < max_num_components) {
        auto eC = a->componentArray[i];
        auto e2C = b->componentArray[i];
        if (eC == nullptr && e2C == nullptr) {
            i++;
            continue;
        }

        if (eC != nullptr && e2C != nullptr) {
            i++;
            continue;
        }

        if (eC == nullptr) {
            M_ASSERT(false, "a missing component that b has");
        }

        if (e2C == nullptr) {
            M_ASSERT(false, "b missing component that a has");
        }
        i++;
    }
}

Entity* mk_entity() { return make_entity(DebugOptions{.name = "test"}); }

void entity_components() {
    Entity* entity = mk_entity();
    network::Buffer buff = network::serialize_to_entity(entity);

    Entity* entity2 = mk_entity();
    network::deserialize_to_entity(entity2, buff);

    compare_and_validate_components(entity, entity2);

    delete entity;
    delete entity2;
}

void test_adding_single_component_serialdeserial() {
    // If this test fails, then we know that the components arent being
    // serialized correctly across the network
    Entity* entity = mk_entity();
    entity->addComponent<HasName>();

    network::Buffer buff = network::serialize_to_entity(entity);

    Entity* entity2 = mk_entity();
    // VVVVV if you dont have this, then it doesnt correctly serialize
    // but once you have it then the names do get copied over correctly
    // figure out why bitsery isnt serializing this over
    // entity2->addComponent<HasName>();
    network::deserialize_to_entity(entity2, buff);

    compare_and_validate_components(entity, entity2);

    delete entity;
    delete entity2;
}

void validate_name_change_persisits() {
    Entity* entity = mk_entity();
    entity->addComponent<HasName>();

    auto starting_name = entity->get<HasName>().name();
    auto first_name = "newname";
    entity->get<HasName>().update(first_name);

    M_TEST_NEQ(starting_name, first_name,
               "component should have correctly set name");

    auto middle_name = entity->get<HasName>().name();
    M_TEST_EQ(first_name, middle_name,
              "component should have correctly set name");

    network::Buffer buff = network::serialize_to_entity(entity);
    Entity* entity2 = mk_entity();
    // entity2->addComponent<HasName>();
    network::deserialize_to_entity(entity2, buff);

    compare_and_validate_components(entity, entity2);

    auto last_name = entity2->get<HasName>().name();

    M_TEST_NEQ(starting_name, last_name,
               "component should not have starting name");
    M_TEST_EQ(middle_name, last_name,
              "component before serialize should match deserialize");

    M_TEST_EQ(first_name, last_name, "deserialized should match set name ");

    delete entity;
    delete entity2;
}

void validate_custom_hold_position_persisits() {
    Entity* entity = mk_entity();
    entity->addComponent<CustomHeldItemPosition>();

    auto position = CustomHeldItemPosition::Positioner::Conveyer;

    entity->get<CustomHeldItemPosition>().init(position);

    auto newpos = entity->get<CustomHeldItemPosition>().positioner;

    M_TEST_EQ(position, newpos, "component should have correctly set position");

    network::Buffer buff = network::serialize_to_entity(entity);
    Entity* entity2 = mk_entity();
    network::deserialize_to_entity(entity2, buff);

    compare_and_validate_components(entity, entity2);

    auto post_serialize_position =
        entity2->get<CustomHeldItemPosition>().positioner;

    M_TEST_EQ(position, post_serialize_position,
              "component should have starting position");
    M_TEST_EQ(newpos, post_serialize_position,
              "component before serialize should match deserialize");

    delete entity;
    delete entity2;
}

void validate_held_item_serialized() {
    Entity* entity = mk_entity();
    entity->addComponent<CanHoldItem>();

    M_TEST_EQ(entity->get<CanHoldItem>().item(), nullptr,
              "default should be nullptr");

    std::shared_ptr<Item> item = std::make_shared<Item>(vec2{0, 0}, WHITE);
    entity->get<CanHoldItem>().update(item);

    M_TEST_NEQ(entity->get<CanHoldItem>().item(), nullptr,
               "component should not have null item");

    network::Buffer buff = network::serialize_to_entity(entity);
    Entity* entity2 = mk_entity();
    network::deserialize_to_entity(entity2, buff);

    compare_and_validate_components(entity, entity2);

    auto& post_serialize_item = entity2->get<CanHoldItem>().item();

    M_TEST_NEQ(post_serialize_item, nullptr,
               "component should not have null item");

    M_TEST_EQ(item->color, post_serialize_item->color,
              "component should have item with the same color");

    delete entity;
    delete entity2;
}

void remote_player_components() {
    Entity* entity = make_remote_player({0, 0, 0});

    network::Buffer buff = network::serialize_to_entity(entity);

    Entity* entity2 = make_remote_player({0, 0, 0});
    network::deserialize_to_entity(entity2, buff);

    compare_and_validate_components(entity, entity2);

    delete entity;
    delete entity2;
}

void all_tests() {
    entity_components();
    test_adding_single_component_serialdeserial();
    validate_custom_hold_position_persisits();
    validate_name_change_persisits();
    validate_held_item_serialized();
    // remote_player_components();
}
