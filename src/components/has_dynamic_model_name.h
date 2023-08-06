

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
    typedef std::function<std::string(const Entity&, const std::string&)>
        ModelNameFetcher;

    [[nodiscard]] std::string fetch(const Entity& owner) const {
        if (!initialized)
            log_warn(
                "calling HasDynamicModelName::fetch() without initializing");

        switch (dynamic_type) {
            case OpenClosed: {
                const bool in_planning =
                    GameState::get().is(game::State::Planning);
                if (in_planning) return base_name;
                return fmt::format("open_{}", base_name);
            } break;
                // TODO eventually id like the logic to live in here assuming we
                // have a ton using these. if its just one for each then fetcher
                // (Custom:) is perfectly fine
            case NoDynamicType:
            case Ingredients:
            case EmptyFull:
            case Subtype: {
                return fetcher(owner, base_name);
            } break;
        }
        return base_name;
    }

    void init(EntityType type, DynamicType dyn_type,
              ModelNameFetcher fet = nullptr) {
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

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        // Not needed because this is only used to change the underlying model
        // inside <ModelRenderer>
        //
        // s.value1b(initialized);
        // s.text1b(base_name, MAX_MODEL_NAME_LENGTH);
    }
};
