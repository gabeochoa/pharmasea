

#pragma once

#include "../engine/model_library.h"
#include "../engine/util.h"
#include "../entity.h"
#include "base_component.h"

struct ModelRenderer : public BaseComponent {
    ModelRenderer() : model_name("invalid") {}
    explicit ModelRenderer(const std::string& s) : model_name(s) {}
    explicit ModelRenderer(const EntityType& type) {
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
    [[nodiscard]] const std::string& name() const { return model_name; }

    void update_model_name(const std::string& new_name) {
        model_name = new_name;
    }

   private:
    std::string model_name;

    friend class cereal::access;
    template<class Archive>
    void serialize(Archive& archive) {
        archive(cereal::base_class<BaseComponent>(this),
                //
                model_name);
    }
};
CEREAL_REGISTER_TYPE(ModelRenderer);
