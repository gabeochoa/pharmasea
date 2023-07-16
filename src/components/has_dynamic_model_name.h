

#pragma once

#include "../engine/assert.h"
#include "../engine/model_library.h"
#include "../engine/statemanager.h"
#include "../vendor_include.h"
#include "base_component.h"
#include "has_subtype.h"

struct HasDynamicModelName : public BaseComponent {
    enum DynamicType { OpenClosed, Subtype };

    virtual ~HasDynamicModelName() {}
    typedef std::function<std::string()> ModelNameFetcher;

    [[nodiscard]] std::string fetch() const {
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
            case Subtype: {
                // TODO support fetching subtype based model name
                return base_name;
            } break;
        }
        return base_name;
    }

    void init(const std::string& _base_name, DynamicType type) {
        initialized = true;
        base_name = _base_name;
        dynamic_type = type;
    }

   private:
    DynamicType dynamic_type;
    std::string base_name;
    bool initialized = false;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
        //
        s.value1b(initialized);
        s.text1b(base_name, MAX_MODEL_NAME_LENGTH);
    }
};
