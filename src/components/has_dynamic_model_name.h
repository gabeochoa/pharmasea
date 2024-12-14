

#pragma once

#include "../engine/assert.h"
#include "../engine/model_library.h"
#include "../engine/statemanager.h"
#include "../engine/util.h"
#include "../entity.h"
#include "../vendor_include.h"
#include "base_component.h"

struct HasDynamicModelName : public BaseComponent {
    enum DynamicType {
        NoDynamicType,
        OpenClosed,
        Subtype,
        EmptyFull,
        Ingredients
    };

    virtual ~HasDynamicModelName() {}
    using ModelNameFetcher =
        std::function<std::string(const Entity&, const std::string&)>;

    [[nodiscard]] std::string fetch(const Entity& owner) const;

    void init(EntityType type, DynamicType dyn_type,
              const ModelNameFetcher& fet = nullptr) {
        initialized = true;
        base_name = std::string(util::convertToSnakeCase<EntityType>(type));
        dynamic_type = dyn_type;
        fetcher = fet;
    }

   private:
    DynamicType dynamic_type = DynamicType::NoDynamicType;
    std::string base_name;
    bool initialized = false;
    ModelNameFetcher fetcher;

    friend class cereal::access;
    template<class Archive>
    void serialize(Archive& archive) {
        archive(cereal::base_class<BaseComponent>(this)
                //

                // Not needed because this is only used to change the underlying
                // model inside <ModelRenderer>
                //
                // s.value1b(initialized);
                // s.text1b(base_name, MAX_MODEL_NAME_LENGTH);
        );
    }
};

CEREAL_REGISTER_TYPE(HasDynamicModelName);
