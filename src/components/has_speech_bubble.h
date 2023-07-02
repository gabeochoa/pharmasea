
#pragma once

#include "base_component.h"

struct HasSpeechBubble : public BaseComponent {
    virtual ~HasSpeechBubble() {}

    [[nodiscard]] vec3 relative_position() const { return position; }
    [[nodiscard]] std::string icon() const { return icon_name; }

   private:
    int max_icon_name_length = 20;
    std::string icon_name;
    vec3 position;

    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        s.value4b(max_icon_name_length);
        s.text1b(icon_name, max_icon_name_length);

        s.object(position);
    }
};
