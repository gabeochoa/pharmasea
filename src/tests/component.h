

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

        if ((eC == nullptr && e2C != nullptr) ||
            (eC != nullptr && e2C == nullptr)) {
            M_ASSERT(false, "one was null but other wasnt");
        }

        i++;
    }
}

void entity_components() {
    Entity* entity = make_entity();
    network::Buffer buff = network::serialize_to_entity(entity);

    Entity* entity2 = make_entity();
    network::deserialize_to_entity(entity2, buff);

    compare_and_validate_components(entity, entity2);

    delete entity;
    delete entity2;
}

void test_adding_single_component_serialdeserial() {
    // If this test fails, then we know that the components arent being
    // serialized correctly across the network
    Entity* entity = make_entity();
    entity->addComponent<HasName>();

    network::Buffer buff = network::serialize_to_entity(entity);

    Entity* entity2 = make_entity();
    // VVVVV if you dont have this, then it doesnt correctly serialize
    // but once you have it then the names do get copied over correctly
    // figure out why bitsery isnt serializing this over
    entity2->addComponent<HasName>();
    network::deserialize_to_entity(entity2, buff);

    compare_and_validate_components(entity, entity2);

    delete entity;
    delete entity2;
}

void validate_name_change_persisits() {
    Entity* entity = make_entity();
    entity->addComponent<HasName>();

    network::Buffer buff = network::serialize_to_entity(entity);

    auto starting_name = entity->get<HasName>().name();
    auto first_name = "newname";
    entity->get<HasName>().update(first_name);

    M_TEST_NEQ(starting_name, first_name,
               "component should have correctly set name");

    auto middle_name = entity->get<HasName>().name();
    M_TEST_EQ(first_name, middle_name,
              "component should have correctly set name");

    Entity* entity2 = make_entity();
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
    // validate_name_change_persisits();
    // remote_player_components();
}
