

#pragma once

#include "../engine/model_library.h"
#include "base_component.h"

struct ModelRenderer : public BaseComponent {
    virtual ~ModelRenderer() {}

    [[nodiscard]] bool has_model() const { return model_info().has_value(); }
    [[nodiscard]] std::optional<ModelInfo> model_info() const { return info; }
    [[nodiscard]] raylib::Model model() const {
        return ModelLibrary::get().get(info.value().model_name);
    }

    void update(const ModelInfo& new_info) { info = new_info; }

   private:
    std::optional<ModelInfo> info;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});
        // Only things that need to be rendered, need to be serialized :)
        s.ext(info, bitsery::ext::StdOptional{});
    }
};
