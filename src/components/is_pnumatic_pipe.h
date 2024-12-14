

#pragma once

#include "base_component.h"

struct IsPnumaticPipe : public BaseComponent {
    bool recieving = false;
    int item_id = -1;

    int paired_id = -1;

    virtual ~IsPnumaticPipe() {}

    [[nodiscard]] bool has_pair() const { return paired_id != -1; }

   private:
    template<class Archive>
    void serialize(Archive& archive) {
        archive(cereal::base_class<BaseComponent>(this),
                //
                paired_id);
    }
};

CEREAL_REGISTER_TYPE(IsPnumaticPipe);
