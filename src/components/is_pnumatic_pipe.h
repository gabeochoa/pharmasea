

#pragma once

#include "../entities/entity_ref.h"
#include "base_component.h"

struct IsPnumaticPipe : public BaseComponent {
    bool recieving = false;
    int item_id = -1;

    EntityRef paired{};

    [[nodiscard]] bool has_pair() const {
        return paired.id != entity_id::INVALID;
    }

   private:
   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        // s.value4b(item_id);
        return archive(                         //
            static_cast<BaseComponent&>(self),  //
            self.paired                         //
        );
    }
};
