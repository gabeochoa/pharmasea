
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

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        s.value1b(_enabled);

        s.value4b(max_icon_name_length);
        s.text1b(icon_name, max_icon_name_length);
    }
};
