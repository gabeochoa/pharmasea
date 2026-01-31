

#pragma once

#include "../libraries/model_library.h"
#include "../engine/util.h"
#include "../entity.h"
#include "base_component.h"

struct ModelRenderer : public BaseComponent {
    ModelRenderer() : model_name("invalid") {}
    explicit ModelRenderer(const std::string& s) : model_name(s) {}
    explicit ModelRenderer(const EntityType& type) : source_type(type) {
        model_name = util::convertToSnakeCase<EntityType>(type);
    }

    [[nodiscard]] bool missing() const { return !exists(); }
    [[nodiscard]] bool exists() const {
        return ModelInfoLibrary::get().has(model_name);
    }

    [[nodiscard]] ModelInfo& model_info() const {
        return ModelInfoLibrary::get().get(model_name);
    }
    [[nodiscard]] raylib::Model model() const {
        return ModelLibrary::get().get_and_load_if_needed(model_name);
    }
    [[nodiscard]] const std::string& name() const { return model_name; }

    void update_model_name(std::string_view new_name) {
        model_name = new_name;
        source_type = EntityType::Unknown;  // Clear source since name changed
    }

   private:
    std::string model_name;
    EntityType source_type = EntityType::Unknown;

   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        // Optimization: If source_type is set (most models), serialize just the
        // enum (4 bytes). Otherwise serialize the string for custom models.
        using ArchiveKind = std::remove_cvref_t<decltype(archive)>;
        constexpr bool is_reading = ArchiveKind::kind() == zpp::bits::kind::in;
        
        auto result = archive(
            static_cast<BaseComponent&>(self),
            self.source_type
        );
        
        // For custom models (string-based), also serialize the model_name
        if (self.source_type == EntityType::Unknown) {
            result = archive(self.model_name);
        } else if constexpr (is_reading) {
            // Derive model_name from EntityType
            self.model_name = util::convertToSnakeCase<EntityType>(self.source_type);
        }
        
        return result;
    }
};
