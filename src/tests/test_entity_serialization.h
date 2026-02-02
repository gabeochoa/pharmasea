
#pragma once

#include "../components/has_name.h"
#include "../components/is_item.h"
#include "../components/transform.h"
#include "../engine/assert.h"
#include "../entities/entity.h"
#include "../entities/entity_helper.h"
#include "../network/serialization.h"
#include "../vec_util.h"

namespace tests {

inline void test_entity_serialization_roundtrip() {
    // Create an entity with various components and tags
    auto entity = std::make_shared<Entity>();
    entity->entity_type = static_cast<int>(EntityType::Item);
    entity->cleanup = false;

    // Add some tags
    entity->enableTag(0);   // Tag 0
    entity->enableTag(5);   // Tag 5
    entity->enableTag(10);  // Tag 10

    // Add some components
    entity->addComponent<Transform>();
    entity->get<Transform>().update(vec3{1.0f, 2.0f, 3.0f});

    entity->addComponent<HasName>();
    entity->get<HasName>().name = "TestEntity";

    entity->addComponent<IsItem>();

    // Store original values for comparison
    const int original_id = entity->id;
    const int original_type = entity->entity_type;
    const bool original_cleanup = entity->cleanup;
    const vec3 original_pos = entity->get<Transform>().position;
    const std::string original_name = entity->get<HasName>().name;
    const bool had_tag_0 = entity->hasTag(0);
    const bool had_tag_5 = entity->hasTag(5);
    const bool had_tag_10 = entity->hasTag(10);
    const bool had_tag_1 = entity->hasTag(1);  // Should be false

    // Serialize
    network::Buffer buffer = network::serialize_to_entity(entity.get());
    VALIDATE(!buffer.empty(), "Serialized buffer should not be empty");

    // Create a new entity for deserialization
    auto deserialized_entity = std::make_shared<Entity>();

    // Deserialize
    network::deserialize_to_entity(deserialized_entity.get(), buffer);

    // Verify all fields match
    VALIDATE(deserialized_entity->id == original_id,
             "Entity ID should match after roundtrip");
    VALIDATE(deserialized_entity->entity_type == original_type,
             "Entity type should match after roundtrip");
    VALIDATE(deserialized_entity->cleanup == original_cleanup,
             "Entity cleanup flag should match after roundtrip");

    // Verify tags
    VALIDATE(deserialized_entity->hasTag(0) == had_tag_0,
             "Tag 0 should be preserved");
    VALIDATE(deserialized_entity->hasTag(5) == had_tag_5,
             "Tag 5 should be preserved");
    VALIDATE(deserialized_entity->hasTag(10) == had_tag_10,
             "Tag 10 should be preserved");
    VALIDATE(deserialized_entity->hasTag(1) == had_tag_1,
             "Tag 1 should remain false");

    // Verify components exist
    VALIDATE(deserialized_entity->has<Transform>(),
             "Transform component should exist after deserialization");
    VALIDATE(deserialized_entity->has<HasName>(),
             "HasName component should exist after deserialization");
    VALIDATE(deserialized_entity->has<IsItem>(),
             "IsItem component should exist after deserialization");

    // Verify component data
    VALIDATE(
        deserialized_entity->get<Transform>().position.x == original_pos.x &&
            deserialized_entity->get<Transform>().position.y ==
                original_pos.y &&
            deserialized_entity->get<Transform>().position.z == original_pos.z,
        "Transform position should match after roundtrip");
    VALIDATE(deserialized_entity->get<HasName>().name == original_name,
             "HasName name should match after roundtrip");
}

inline void test_entity_serialization_empty_tags() {
    // Test entity with no tags set
    auto entity = std::make_shared<Entity>();
    entity->entity_type = static_cast<int>(EntityType::Item);
    entity->addComponent<Transform>();

    // Ensure no tags are set
    for (size_t i = 0; i < 64; ++i) {
        VALIDATE(!entity->hasTag(static_cast<afterhours::TagId>(i)),
                 "Entity should have no tags initially");
    }

    // Serialize and deserialize
    network::Buffer buffer = network::serialize_to_entity(entity.get());
    auto deserialized_entity = std::make_shared<Entity>();
    network::deserialize_to_entity(deserialized_entity.get(), buffer);

    // Verify no tags are set after deserialization
    for (size_t i = 0; i < 64; ++i) {
        VALIDATE(
            !deserialized_entity->hasTag(static_cast<afterhours::TagId>(i)),
            "Entity should have no tags after deserialization");
    }
}

inline void test_entity_serialization_all_tags() {
    // Test entity with many tags set
    auto entity = std::make_shared<Entity>();
    entity->entity_type = static_cast<int>(EntityType::Item);

    // Set many tags
    for (size_t i = 0; i < 32; ++i) {
        entity->enableTag(static_cast<afterhours::TagId>(i));
    }

    // Serialize and deserialize
    network::Buffer buffer = network::serialize_to_entity(entity.get());
    auto deserialized_entity = std::make_shared<Entity>();
    network::deserialize_to_entity(deserialized_entity.get(), buffer);

    // Verify all tags are preserved
    for (size_t i = 0; i < 32; ++i) {
        VALIDATE(deserialized_entity->hasTag(static_cast<afterhours::TagId>(i)),
                 fmt::format("Tag {} should be preserved", i));
    }

    // Verify tags beyond 32 are not set
    for (size_t i = 32; i < 64; ++i) {
        VALIDATE(
            !deserialized_entity->hasTag(static_cast<afterhours::TagId>(i)),
            fmt::format("Tag {} should not be set", i));
    }
}

inline void test_entity_serialization() {
    test_entity_serialization_roundtrip();
    test_entity_serialization_empty_tags();
    test_entity_serialization_all_tags();
}

}  // namespace tests
