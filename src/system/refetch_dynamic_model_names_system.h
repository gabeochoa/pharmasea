#pragma once

#include "../ah.h"
#include "../components/has_dynamic_model_name.h"
#include "../components/model_renderer.h"

namespace system_manager {

struct RefetchDynamicModelNamesSystem
    : public afterhours::System<HasDynamicModelName, ModelRenderer> {
    virtual void for_each_with(Entity& entity, HasDynamicModelName& hDMN,
                               ModelRenderer& renderer, float) override {
        renderer.update_model_name(hDMN.fetch(entity));
    }
};

}  // namespace system_manager
