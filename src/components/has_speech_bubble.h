
#pragma once

#include "base_component.h"

struct HasSpeechBubble : public BaseComponent {
    virtual ~HasSpeechBubble() {}

    [[nodiscard]] std::string icon() const { return icon_name; }
    [[nodiscard]] bool enabled() const { return _enabled; }
    [[nodiscard]] bool disabled() const { return !_enabled; }

    void off() { _enabled = false; }
    void on() { _enabled = true; }

   private:
    bool _enabled = false;
    int max_icon_name_length = 20;
    std::string icon_name;
    template<class Archive>
    void serialize(Archive& archive) {
        archive(cereal::base_class<BaseComponent>(this),
                //
                _enabled, max_icon_name_length, icon_name);
    }
};

CEREAL_REGISTER_TYPE(HasSpeechBubble);
