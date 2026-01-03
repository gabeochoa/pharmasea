
#pragma once

#include "base_component.h"

struct HasSpeechBubble : public BaseComponent {
    [[nodiscard]] std::string icon() const { return icon_name; }
    [[nodiscard]] bool enabled() const { return _enabled; }
    [[nodiscard]] bool disabled() const { return !_enabled; }

    void off() { _enabled = false; }
    void on() { _enabled = true; }

   private:
    bool _enabled = false;
    int max_icon_name_length = 20;
    std::string icon_name;

    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        using archive_type = std::remove_cvref_t<decltype(archive)>;
        if (auto result = archive(                      //
                static_cast<BaseComponent&>(self),       //
                self._enabled,                           //
                self.max_icon_name_length,               //
                self.icon_name                            //
                );
            zpp::bits::failure(result)) {
            return result;
        }
        if constexpr (archive_type::kind() == zpp::bits::kind::in) {
            if (static_cast<int>(self.icon_name.size()) > self.max_icon_name_length) {
                return std::errc::message_size;
            }
        }
        return std::errc{};
    }
};
