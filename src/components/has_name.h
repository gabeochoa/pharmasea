#pragma once

#include <string>
//
#include "base_component.h"

struct HasName : public BaseComponent {
    void update(const std::string& new_name) {
        _name = new_name;
        name_length = (int) _name.size();
    }

    // Mark name as static (AI customers, UI elements) - won't be serialized
    HasName& set_static(bool is_static_name = true) {
        is_static = is_static_name;
        return *this;
    }

    const std::string& name() const { return _name; }

   private:
    int name_length = 1;
    std::string _name;
    bool is_static = false;  // Static names don't need serialization

   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        using ArchiveKind = std::remove_cvref_t<decltype(archive)>;
        constexpr bool is_reading = ArchiveKind::kind() == zpp::bits::kind::in;
        
        // Always serialize the static flag
        auto result = archive(
            static_cast<BaseComponent&>(self),
            self.is_static
        );
        
        // Only serialize the actual name if it's not static
        if (!self.is_static) {
            result = archive(self.name_length, self._name);
        } else if constexpr (is_reading) {
            // For static names, clear on read (will be regenerated)
            self._name.clear();
            self.name_length = 0;
        }
        
        return result;
    }
};
