#pragma once

#include "../../ah.h"
#include "../../client_server_comm.h"
#include "../../components/can_be_highlighted.h"
#include "../../entities/entity_type.h"

namespace system_manager {

struct ShowMinimapWhenHighlightedSystem
    : public afterhours::System<
          CanBeHighlighted, afterhours::tags::All<EntityType::MapRandomizer>> {
    virtual void for_each_with(Entity&, CanBeHighlighted& cbh, float) override {
        if (cbh.is_set()) {
            server_only::set_show_minimap();
        } else {
            server_only::set_hide_minimap();
        }
    }
};

}  // namespace system_manager
