#pragma once

#include <string>
//
#include "base_component.h"

struct HasName : public BaseComponent {
    void update(const std::string& new_name) {
        _name = new_name;
        name_length = (int) _name.size();
    }

    const std::string& name() const { return _name; }

   private:
    int name_length = 1;
    std::string _name;

    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        using archive_type = std::remove_cvref_t<decltype(archive)>;
        if (auto result = archive(                      //
                static_cast<BaseComponent&>(self),       //
                self.name_length,                        //
                self._name                               //
                );
            zpp::bits::failure(result)) {
            return result;
        }
        if constexpr (archive_type::kind() == zpp::bits::kind::in) {
            if (static_cast<int>(self._name.size()) > self.name_length) {
                return std::errc::message_size;
            }
        }
        return std::errc{};
    }
};
