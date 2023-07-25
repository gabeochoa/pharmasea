#pragma once

#include "../engine/log.h"
#include "../engine/statemanager.h"
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
        //
        WaitingReasonLast,
    } waiting_reason = None;

    std::bitset<WaitingReason::WaitingReasonLast> block_state_change_reasons;

    [[nodiscard]] const char* text_reason(WaitingReason wr) const {
        switch (wr) {
            case CustomersInStore:
                return text_lookup(strings::i18n::CUSTOMERS_IN_STORE);
            case HoldingFurniture:
                return text_lookup(strings::i18n::HOLDING_FURNITURE);
            case NoPathToRegister:
                return text_lookup(strings::i18n::NO_PATH_TO_REGISTER);
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

    [[nodiscard]] bool read_reason(WaitingReason wr) const {
        return read_reason(magic_enum::enum_integer<WaitingReason>(wr));
    }

    [[nodiscard]] bool read_reason(int i) const {
        return block_state_change_reasons.test(i);
    }

    void write_reason(WaitingReason wr, bool value) {
        auto i = magic_enum::enum_integer<WaitingReason>(wr);
        block_state_change_reasons[i] = value;
        if (value) roundSwitchCountdown = roundSwitchCountdownReset;
    }

    [[nodiscard]] bool store_is_closed() const {
        return currentRoundTime <= 0 && GameState::s_in_round();
    }

    void on_complete(float dt) {
        auto _reset_timer = [&]() { currentRoundTime = totalRoundTime; };

        // Countdown timer
        if (roundSwitchCountdown >= 0) {
            roundSwitchCountdown -= dt;
            return;
        }

        switch (GameState::get().read()) {
            case game::State::Planning: {
                // For this one, we need to wait until everyone drops the things
                if (read_reason(WaitingReason::HoldingFurniture)) return;
                GameState::get().set(game::State::InRound);
            } break;
            case game::State::InRound: {
                // For this one, we need to wait until everyone is done leaving
                if (read_reason(WaitingReason::CustomersInStore)) return;
                GameState::get().set(game::State::Planning);
            } break;

            default:
                log_warn("Completed round switch timer but no state handler {}",
                         GameState::get().read());
                return;
        }

        roundSwitchCountdown = roundSwitchCountdownReset;
        _reset_timer();
    }
    void reset_if_complete(float dt) {
        if (currentRoundTime <= 0.f) on_complete(dt);
    }
};
