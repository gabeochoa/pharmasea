

#pragma once

#include "../engine/model_library.h"
#include "../engine/util.h"
#include "../entity.h"
#include "base_component.h"

struct ModelRenderer : public BaseComponent {
    ModelRenderer() : model_name("invalid") {}
    ModelRenderer(const std::string& s) : model_name(s) {}
    ModelRenderer(const EntityType& type) {
        model_name = util::convertToSnakeCase<EntityType>(type);
    }

    virtual ~ModelRenderer() {}

    [[nodiscard]] bool missing() const { return !exists(); }
    [[nodiscard]] bool exists() const {
        return ModelInfoLibrary::get().has(model_name);
    }

    [[nodiscard]] ModelInfo& model_info() const {
        return ModelInfoLibrary::get().get(model_name);
    }
    [[nodiscard]] raylib::Model model() const {
        return ModelLibrary::get().get(model_name);
    }
    [[nodiscard]] const std::string name() const { return model_name; }

    void update_model_name(const std::string& new_name) {
        model_name = new_name;
    }

   private:
    std::string model_name;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        s.text1b(model_name, MAX_MODEL_NAME_LENGTH);
    }
};
