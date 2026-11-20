

#pragma once

#include "../entities/entity_ref.h"
#include "base_component.h"

struct IsPnumaticPipe : public BaseComponent {
    enum class PairingState { Unpaired, PairedIdle, Transferring };

    bool recieving = false;
    int item_id = -1;

    EntityRef paired{};

    [[nodiscard]] bool has_pair() const {
        return paired.id != entity_id::INVALID;
    }

    [[nodiscard]] PairingState pairing_state() const {
        if (!has_pair()) return PairingState::Unpaired;
        return recieving ? PairingState::Transferring
                         : PairingState::PairedIdle;
    }

    [[nodiscard]] bool is_pairing_state(PairingState state) const {
        return pairing_state() == state;
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
