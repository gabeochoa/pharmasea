
#pragma once

#include "../engine/log.h"
#include "../engine/statemanager.h"
#include "../strings.h"
#include "../vec_util.h"
#include "base_component.h"

struct CollectsCustomerFeedback : public BaseComponent {
    CollectsCustomerFeedback() {}

   private:
    // we only want to run the checks every second
    float waiting_time = 1.f;
    float waiting_time_reset = 1.f;

   public:
    friend zpp::bits::access;
    constexpr static auto serialize(auto& archive, auto& self) {
        // NOTE: waiting_time fields are intentionally NOT serialized.
        return archive(                      //
            static_cast<BaseComponent&>(self), //
            self.block_state_change_reasons, //
            self.block_state_change_locations //
        );
    }

   public:
    [[nodiscard]] bool waiting_time_pass(float dt) {
        waiting_time -= dt;
        if (waiting_time <= 0) {
            waiting_time += waiting_time_reset;
            return true;
        }
        return false;
    }

    enum WaitingReason {
        None,
        CustomersInStore,
        HoldingFurniture,
        NoPathToRegister,
        RegisterNotInside,
        BarNotClean,
        FurnitureOverlapping,
        ItemInSpawnArea,
        DeletingNeededItem,
        StoreStealingMachine,
        StoreHasGarbage,
        //
        WaitingReasonLast,
    } waiting_reason = None;

    std::bitset<WaitingReason::WaitingReasonLast> block_state_change_reasons;
    std::array<vec2, WaitingReasonLast> block_state_change_locations;
    vec2 invalid_location = vec2{999.f, 999.f};

    [[nodiscard]] TranslatableString text_reason(WaitingReason wr) const {
        switch (wr) {
            case CustomersInStore:
                return TranslatableString(strings::i18n::CUSTOMERS_IN_STORE);
            case HoldingFurniture:
                return TranslatableString(strings::i18n::HOLDING_FURNITURE);
            case NoPathToRegister:
                return TranslatableString(strings::i18n::NO_PATH_TO_REGISTER);
            case BarNotClean:
                return TranslatableString(strings::i18n::BAR_NOT_CLEAN);
            case FurnitureOverlapping:
                return TranslatableString(strings::i18n::FURNITURE_OVERLAPPING);
            case ItemInSpawnArea:
                return TranslatableString(strings::i18n::ITEMS_IN_SPAWN_AREA);
            case DeletingNeededItem:
                return TranslatableString(strings::i18n::DELETING_NEEDED_ITEM);
            case RegisterNotInside:
                return TranslatableString(strings::i18n::REGISTER_NOT_INSIDE);
            case WaitingReasonLast:
            case None:
                return NO_TRANSLATE("");
            case StoreStealingMachine:
                return TranslatableString(strings::i18n::StoreStealingMachine);
                break;
            case StoreHasGarbage:
                return TranslatableString(strings::i18n::StoreHasGarbage);
                break;
        }
    }

    [[nodiscard]] TranslatableString text_reason(int i) const {
        WaitingReason name = magic_enum::enum_value<WaitingReason>(i);
        return text_reason(name);
    }

    [[nodiscard]] std::optional<vec2> reason_location(int i) const {
        vec2 pos = block_state_change_locations[i];
        // Theres some issue with serializing optional<vec> so instead
        // we just put a magic value and when we see that on client
        // we are good to just pretend it was invalid
        if (vec::distance(pos, invalid_location) < 1.f) {
            return {};
        }
        return pos;
    }

    [[nodiscard]] bool read_reason(WaitingReason wr) const {
        return read_reason(magic_enum::enum_integer<WaitingReason>(wr));
    }

    [[nodiscard]] bool read_reason(int i) const {
        return block_state_change_reasons.test(i);
    }

    void write_reason(WaitingReason wr, bool value,
                      std::optional<vec2> location) {
        auto i = magic_enum::enum_integer<WaitingReason>(wr);
        block_state_change_reasons[i] = value;
        block_state_change_locations[i] = location.value_or(invalid_location);
    }
};
