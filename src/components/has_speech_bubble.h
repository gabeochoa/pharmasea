
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

   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        return archive(                         //
            static_cast<BaseComponent&>(self),  //
            self._enabled,                      //
            self.max_icon_name_length,          //
            self.icon_name                      //
        );
    }
};
