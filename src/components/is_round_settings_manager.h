
#pragma once

#include "base_component.h"

struct IsRoundSettingsManager : public BaseComponent {
    struct Config {
        float round_length = 100.f;

        friend bitsery::Access;
        template<typename S>
        void serialize(S& s) {
            s.value4b(round_length);
        }
    } config;

    virtual ~IsRoundSettingsManager() {}

    [[nodiscard]] float round_length() const { return config.round_length; }

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        s.object(config);
    }
};
