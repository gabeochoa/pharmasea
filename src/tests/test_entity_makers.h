
#pragma once

#include "../components/model_renderer.h"
#include "../components/simple_colored_box_renderer.h"
#include "../engine/assert.h"
#include "../entities/entity_makers.h"

namespace tests {

inline void test_entity_makers() {
    bool old_models = ENABLE_MODELS;
    ENABLE_MODELS = false;
    {
        auto mop_buddy = std::make_shared<Entity>();
        items::make_item_type(*mop_buddy, EntityType::MopBuddy,
                              raylib::Vector3{0, 0, 0});
        VALIDATE(!mop_buddy->has<ModelRenderer>(),
                 "mop buddy should have no ModelRenderer with -M");
        VALIDATE(mop_buddy->has<SimpleColoredBoxRenderer>(),
                 "mop buddy should have SimpleColoredBoxRenderer with -M");
    }
    ENABLE_MODELS = true;
    {
        auto mop_buddy = std::make_shared<Entity>();
        items::make_item_type(*mop_buddy, EntityType::MopBuddy,
                              raylib::Vector3{0, 0, 0});
        VALIDATE(mop_buddy->has<ModelRenderer>(),
                 "mop buddy should have ModelRenderer with models on");
        VALIDATE(mop_buddy->get<ModelRenderer>().name() == "mop_buddy",
                 "mop buddy model name should be snake_case type");
    }
    ENABLE_MODELS = old_models;
}

}  // namespace tests
