#pragma once

#include "../engine/log.h"
#include "../engine/statemanager.h"
#include "../vec_util.h"
#include "base_component.h"

struct HasTimer : public BaseComponent {
    // TODO using this cause i dont want to make a new component for each
    // renderer type
    enum Renderer {
        Round,
    } type;

    virtual ~HasTimer() {}

    HasTimer() : type(Round), currentRoundTime(10.f), totalRoundTime(10.f) {}

    explicit HasTimer(Renderer _type, float t) {
        type = _type;
        totalRoundTime = t;
        currentRoundTime = totalRoundTime;
    }

    auto& pass_time(float dt) {
        if (currentRoundTime >= 0) currentRoundTime -= dt;
        return *this;
    }

    [[nodiscard]] float pct() const {
        return currentRoundTime / totalRoundTime;
    }

    auto& reset_timer() {
        currentRoundTime = totalRoundTime;
        return *this;
    }

    // TODO make these private
    float currentRoundTime;
    float totalRoundTime;

   private:
    friend bitsery::Access;
    template<typename S>
    void serialize(S& s) {
        s.ext(*this, bitsery::ext::BaseClass<BaseComponent>{});

        s.value4b(type);
        s.value4b(currentRoundTime);
        s.value4b(totalRoundTime);

        // other pcomponent
        s.ext(block_state_change_reasons, bitsery::ext::StdBitset{});
        s.container(block_state_change_locations);
        s.value4b(roundSwitchCountdown);
        s.value4b(roundSwitchCountdownReset);
        s.value4b(dayCount);
    }

   public:
    float roundSwitchCountdownReset = 5.f;
    float roundSwitchCountdown = 5.f;
    int dayCount = 0;

    // TODO move into its own component
    enum WaitingReason {
        None,
        CustomersInStore,
        HoldingFurniture,
        NoPathToRegister,
        BarNotClean,
        FurnitureOverlapping,
        ItemInSpawnArea,
        DeletingNeededItem,
        //
        WaitingReasonLast,
    } waiting_reason = None;

    std::bitset<WaitingReason::WaitingReasonLast> block_state_change_reasons;
    std::array<vec2, WaitingReasonLast> block_state_change_locations;
    vec2 invalid_location = vec2{999.f, 999.f};

    [[nodiscard]] const char* text_reason(WaitingReason wr) const {
        switch (wr) {
            case CustomersInStore:
                return text_lookup(strings::i18n::CUSTOMERS_IN_STORE);
            case HoldingFurniture:
                return text_lookup(strings::i18n::HOLDING_FURNITURE);
            case NoPathToRegister:
                return text_lookup(strings::i18n::NO_PATH_TO_REGISTER);
            case BarNotClean:
                return text_lookup(strings::i18n::BAR_NOT_CLEAN);
            case FurnitureOverlapping:
                return text_lookup(strings::i18n::FURNITURE_OVERLAPPING);
            case ItemInSpawnArea:
                return text_lookup(strings::i18n::ITEMS_IN_SPAWN_AREA);
            case DeletingNeededItem:
                return text_lookup(strings::i18n::DELETING_NEEDED_ITEM);
            default:
                log_warn("got reason {} but dont have a way to render it", wr);
            case WaitingReasonLast:
            case None:
                return "";
        }
    }

    [[nodiscard]] const char* text_reason(int i) const {
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
        if (value) roundSwitchCountdown = roundSwitchCountdownReset;
    }

    [[nodiscard]] bool store_is_closed() const {
        return round_over() && GameState::get().in_round();
    }

    [[nodiscard]] bool round_over() const { return currentRoundTime <= 0; }
    [[nodiscard]] bool round_not_over() const { return !round_over(); }

    auto& pass_time_round_switch(float dt) {
        if (roundSwitchCountdown >= 0) roundSwitchCountdown -= dt;
        return *this;
    }

    [[nodiscard]] bool round_switch_ready() const {
        return roundSwitchCountdown <= 0;
    }
    [[nodiscard]] bool round_switch_not_ready() const {
        return !round_switch_ready();
    }

    auto& reset_round_switch_timer() {
        roundSwitchCountdown = roundSwitchCountdownReset;
        return *this;
    }
};
