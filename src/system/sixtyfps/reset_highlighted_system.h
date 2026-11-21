#pragma once

#include "../../ah.h"
#include "../../components/can_be_highlighted.h"

namespace system_manager {

struct ResetHighlightedSystem : public afterhours::System<CanBeHighlighted> {
    virtual void for_each_with(Entity&, CanBeHighlighted& cbh, float) override {
        cbh.update(false);
    }
};

}  // namespace system_manager
