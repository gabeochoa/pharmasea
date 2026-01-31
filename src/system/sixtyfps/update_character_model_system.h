#pragma once

#include "../../ah.h"
#include "../../components/model_renderer.h"
#include "../../components/uses_character_model.h"

namespace system_manager {

struct UpdateCharacterModelFromIndexSystem
    : public afterhours::System<UsesCharacterModel, ModelRenderer> {
    virtual bool should_run(const float) override { return true; }

    virtual void for_each_with([[maybe_unused]] Entity& entity,
                               UsesCharacterModel& usesCharacterModel,
                               ModelRenderer& renderer, float) override {
        if (usesCharacterModel.value_same_as_last_render()) return;

        // TODO this should be the same as all other rendere updates for players
        renderer.update_model_name(usesCharacterModel.fetch_model_name());
        usesCharacterModel.mark_change_completed();
    }
};

}  // namespace system_manager
