

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

    void update_model_name(std::string_view new_name) { model_name = new_name; }

   private:
    std::string model_name;

    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        using archive_type = std::remove_cvref_t<decltype(archive)>;
        if (auto result = archive(                      //
                static_cast<BaseComponent&>(self),       //
                self.model_name                          //
                );
            zpp::bits::failure(result)) {
            return result;
        }
        if constexpr (archive_type::kind() == zpp::bits::kind::in) {
            if (self.model_name.size() > MAX_MODEL_NAME_LENGTH) {
                return std::errc::message_size;
            }
        }
        return std::errc{};
    }
};
