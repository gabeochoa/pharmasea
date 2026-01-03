

#pragma once

#include "base_component.h"

#include "../entity_ref.h"

struct IsPnumaticPipe : public BaseComponent {
    bool recieving = false;
    int item_id = -1;

    EntityRef paired{};

    [[nodiscard]] bool has_pair() const { return paired.id != entity_id::INVALID; }

   private:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        if (auto result = archive(                      //
                static_cast<BaseComponent&>(self),       //
                self.paired                              //
                );
            zpp::bits::failure(result)) {
            return result;
        }

        // s.value4b(item_id);
        return std::errc{};
    }
};
